// whichlib64 — locate 64-bit shared libraries using the dynamic linker's search order.
//
// For each search phase (in order: DT_RPATH/DT_RUNPATH, LD_LIBRARY_PATH,
// /etc/ld.so.cache, default paths from /etc/ld.so.conf), for each directory
// in that phase, for each unfound library:
//
//   1. access("dir/<needed_name>") — exact versioned name from DT_NEEDED (e.g. libfoo.so.1)
//   2. access("dir/<libname>")     — base name (e.g. libfoo.so)
//   3. glob("dir/<libname>.*")     — versioned fallback
//
// On any access() hit, is_elf64() verifies it is a 64-bit ELF. If not,
// the candidate is skipped and the search continues in the next directory/phase.
//
// Lustre-aware: uses access() (not opendir/stat) for probing to avoid
// expensive directory opens (~10ms-900ms on Lustre vs ~10us for access).

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <elf.h>
#include <fcntl.h>
#include <glob.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
enum class FindStatus { Found = 0, NotFound = 1, VersionMismatch = 2 };

struct lib_query {
  std::string_view libname;
  std::string_view needed_name;
  int soversion;
};

struct find_lib_result {
  char path[4096];
  FindStatus status;
};

struct binary_info {
  std::string rpath;
  std::vector<std::string> needed;
};

namespace fs = std::filesystem;

static constexpr char CACHEMAGIC_NEW[] = "glibc-ld.so.cache1.1";
static constexpr char ORIGIN_VAR[] = "$ORIGIN";

struct file_entry_new {
  int32_t flags;
  uint32_t key;
  uint32_t value;
  uint32_t osversion;
  uint64_t hwcap;
};

struct cache_file_new {
  char magic[sizeof CACHEMAGIC_NEW - 1];
  uint32_t nlibs;
  uint32_t len_strings;
  uint32_t unused[5];
};

static bool is_elf64(const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0)
    return false;
  unsigned char ident[EI_NIDENT];
  bool ok = read(fd, ident, EI_NIDENT) == EI_NIDENT && std::memcmp(ident, ELFMAG, SELFMAG) == 0 &&
            ident[EI_CLASS] == ELFCLASS64;
  close(fd);
  return ok;
}

static std::string resolve_versioned_name(std::string_view found_path, std::string_view libname) {
  auto fpath = fs::path(found_path);
  auto fname = fpath.filename().string();

  if (fname == libname) {
    std::error_code ec;
    auto target = fs::read_symlink(fpath, ec);
    if (ec || target.filename().string() == libname)
      return {};
    return target.filename().string();
  }

  assert(fname.size() > libname.size() && fname.compare(0, libname.size(), libname) == 0 &&
         fname[libname.size()] == '.');
  return fname;
}

static bool version_check(std::string_view found_path, std::string_view libname, int soversion) {
  auto fname = resolve_versioned_name(found_path, libname);
  if (fname.empty())
    return false;

  if (soversion < 0) {
    std::cerr << "Warning: no soversion specified for " << libname << " but found versioned "
              << fname << "\n";
    return true;
  }

  int found_ver = std::stoi(fname.substr(libname.size() + 1));
  if (found_ver != soversion) {
    std::cerr << "Warning: expected " << libname << "." << soversion << " but found " << fname
              << "\n";
    return true;
  }
  return false;
}

class MappedFile {
  void *addr_ = MAP_FAILED;
  size_t size_ = 0;

public:
  explicit MappedFile(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0)
      return;
    struct stat st;
    if (fstat(fd, &st) == 0 && st.st_size > 0) {
      size_ = st.st_size;
      addr_ = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd, 0);
    }
    close(fd);
  }
  ~MappedFile() {
    if (addr_ != MAP_FAILED)
      munmap(addr_, size_);
  }
  MappedFile(const MappedFile &) = delete;
  MappedFile &operator=(const MappedFile &) = delete;

  explicit operator bool() const { return addr_ != MAP_FAILED; }
  const uint8_t *data() const { return static_cast<const uint8_t *>(addr_); }
  size_t size() const { return size_; }

  template <typename T> const T *at(size_t offset) const {
    if (offset + sizeof(T) > size_)
      return nullptr;
    return reinterpret_cast<const T *>(data() + offset);
  }

  template <typename T> const T *array_at(size_t offset, size_t count) const {
    if (offset + count * sizeof(T) > size_)
      return nullptr;
    return reinterpret_cast<const T *>(data() + offset);
  }
};

template <size_t N> void copy_to_buf(char (&dest)[N], std::string_view src) {
  size_t to_copy = std::min(src.size(), N - 1);
  std::memcpy(dest, src.data(), to_copy);
  dest[to_copy] = '\0';
}

template <typename Fn> static bool for_each_dir(std::string_view pathlist, Fn &&fn) {
  std::string_view sv(pathlist);
  while (!sv.empty()) {
    auto sep = sv.find(':');
    auto dir = sv.substr(0, sep);
    if (!dir.empty() && fn(dir))
      return true;
    if (sep == std::string_view::npos)
      break;
    sv.remove_prefix(sep + 1);
  }
  return false;
}

static bool build_path(char *buf, size_t buf_size, std::string_view dir, std::string_view name) {
  if (dir.size() + name.size() + 2 > buf_size)
    return false;

  char *out = buf;
  out = std::copy(dir.begin(), dir.end(), out);
  *out++ = '/';
  out = std::copy(name.begin(), name.end(), out);
  *out = '\0';

  return true;
}

// Try to find a library in dir: needed_name first, then libname, then glob libname.*.
// access(full_path) instead of open(dir)+faccessat: on Lustre, open() costs
// 10ms-900ms per directory while access() is ~10us.
static bool
try_find_in_dir(char *buf, size_t buf_size, std::string_view dir, const lib_query &entry) {
  if (!entry.needed_name.empty()) {
    build_path(buf, buf_size, dir, entry.needed_name);
    if (access(buf, F_OK) == 0 && is_elf64(buf))
      return true;
  }
  if (entry.libname.empty())
    return false;
  build_path(buf, buf_size, dir, entry.libname);
  if (access(buf, F_OK) == 0 && is_elf64(buf))
    return true;
  // Versioned fallback: glob dir/libname.* (e.g. libOpenCL.so.1)
  std::string pattern;
  pattern.reserve(dir.size() + 1 + entry.libname.size() + 2);
  pattern.append(dir);
  pattern += '/';
  pattern.append(entry.libname);
  pattern += ".*";
  glob_t gl;
  if (glob(pattern.c_str(), 0, nullptr, &gl) == 0 && gl.gl_pathc > 0) {
    std::snprintf(buf, buf_size, "%s", gl.gl_pathv[0]);
    globfree(&gl);
    return is_elf64(buf);
  }
  return false;
}

template <typename DirRange>
static void search_dirs_batch(DirRange &&dirs,
                              const std::vector<lib_query> &entries,
                              find_lib_result *results,
                              int &pending) {
  char buf[4096];
  for (auto &&dir : dirs) {
    std::string_view dir_sv(dir);
    for (size_t i = 0; i < entries.size(); i++) {
      if (results[i].path[0])
        continue;
      if (try_find_in_dir(buf, sizeof(buf), dir_sv, entries[i])) {
        copy_to_buf(results[i].path, buf);
        if (--pending == 0)
          return;
      }
    }
  }
}

static std::vector<std::string_view> split_pathlist(std::string_view pathlist) {
  std::vector<std::string_view> dirs;
  while (!pathlist.empty()) {
    auto sep = pathlist.find(':');
    auto dir = pathlist.substr(0, sep);
    if (!dir.empty())
      dirs.push_back(dir);
    if (sep == std::string_view::npos)
      break;
    pathlist.remove_prefix(sep + 1);
  }
  return dirs;
}

static void search_pathlist_batch(std::string_view pathlist,
                                  const std::vector<lib_query> &entries,
                                  find_lib_result *results,
                                  int &pending) {
  if (pathlist.empty() || pending == 0)
    return;
  search_dirs_batch(split_pathlist(pathlist), entries, results, pending);
}

// Single mmap, single scan for all pending libraries
static void
search_cache_batch(const std::vector<lib_query> &entries, find_lib_result *results, int &pending) {
  if (pending == 0)
    return;

  MappedFile mf("/etc/ld.so.cache");
  if (!mf)
    return;

  std::string_view view(reinterpret_cast<const char *>(mf.data()), mf.size());
  size_t pos = view.find(CACHEMAGIC_NEW);
  if (pos == std::string_view::npos)
    return;
  if (pos + sizeof(cache_file_new) > mf.size())
    return;

  auto *cache = reinterpret_cast<const cache_file_new *>(mf.data() + pos);
  const char *cache_data = reinterpret_cast<const char *>(mf.data() + pos);

  size_t libs_offset = pos + sizeof(cache_file_new);
  size_t cache_data_size = mf.size() - pos;

  auto *libs = reinterpret_cast<const file_entry_new *>(mf.data() + libs_offset);

  if (sizeof(cache_file_new) + cache->nlibs * sizeof(file_entry_new) > cache_data_size)
    return;

  std::vector<std::string> fallbacks(entries.size());

  for (uint32_t i = 0; i < cache->nlibs && pending > 0; i++) {
    if (libs[i].key >= cache_data_size || libs[i].value >= cache_data_size)
      continue;
    const char *key = cache_data + libs[i].key;
    const char *val = cache_data + libs[i].value;
    for (size_t j = 0; j < entries.size(); j++) {
      if (results[j].path[0])
        continue;
      if (!entries[j].needed_name.empty() && std::strcmp(key, entries[j].needed_name.data()) == 0) {
        if (access(val, F_OK) == 0 && is_elf64(val)) {
          copy_to_buf(results[j].path, val);
          --pending;
        }
      } else if (fallbacks[j].empty() && !entries[j].libname.empty() &&
                 std::strcmp(key, entries[j].libname.data()) == 0) {
        if (access(val, F_OK) == 0 && is_elf64(val))
          fallbacks[j] = val;
      }
    }
  }

  for (size_t j = 0; j < entries.size(); j++) {
    if (!results[j].path[0] && !fallbacks[j].empty()) {
      copy_to_buf(results[j].path, fallbacks[j]);
      --pending;
    }
  }
}

// Parse /etc/ld.so.conf and its includes to collect library search directories.
//
// The format is one directive per line:
//   - A directory path:          /usr/local/lib64
//   - An include with glob:      include /etc/ld.so.conf.d/*.conf
//   - Comments (#) and blank lines are ignored.
//
// Example /etc/ld.so.conf:
//   /usr/local/lib64
//   include /etc/ld.so.conf.d/*.conf
//
// Example /etc/ld.so.conf.d/x86_64-linux-gnu.conf:
//   /usr/local/lib/x86_64-linux-gnu
//   /lib/x86_64-linux-gnu
//   /usr/lib/x86_64-linux-gnu
//
static std::string_view strip(std::string_view sv) {
  if (auto p = sv.find_first_not_of(" \t"); p != sv.npos)
    sv.remove_prefix(p);
  else
    return {};
  if (auto p = sv.find_last_not_of(" \t\r\n"); p != sv.npos)
    sv = sv.substr(0, p + 1);
  return sv;
}

static constexpr std::string_view INCLUDE_PREFIX = "include";

static void parse_ldconfig_conf(const char *path, std::vector<std::string> &dirs) {
  std::ifstream file(path);
  if (!file)
    return;

  std::string line;
  while (std::getline(file, line)) {
    auto sv = strip(line);
    if (sv.empty() || sv.front() == '#')
      continue;

    if (sv.substr(0, INCLUDE_PREFIX.size()) == INCLUDE_PREFIX &&
        sv.size() > INCLUDE_PREFIX.size() &&
        (sv[INCLUDE_PREFIX.size()] == ' ' || sv[INCLUDE_PREFIX.size()] == '\t')) {
      auto pattern = strip(sv.substr(INCLUDE_PREFIX.size()));
      glob_t gl;
      if (glob(std::string(pattern).c_str(), 0, nullptr, &gl) == 0) {
        for (size_t i = 0; i < gl.gl_pathc; i++)
          parse_ldconfig_conf(gl.gl_pathv[i], dirs);
        globfree(&gl);
      }
    } else {
      dirs.emplace_back(sv);
    }
  }
}

// Fallback when ld.so.cache is stale (e.g. CI restoring packages without running ldconfig).
// Mirrors ld.so search order: /etc/ld.so.conf paths first (e.g. multiarch dirs like
// /lib/x86_64-linux-gnu), then the trusted defaults that ldconfig adds implicitly.
// Override conf path with WHICHLIB64_CONF env var for testing.
static constexpr std::string_view DEFAULT_PATHS[] = {"/lib64", "/usr/lib64", "/lib", "/usr/lib"};

static void search_default_paths_batch(const std::vector<lib_query> &entries,
                                       find_lib_result *results,
                                       int &pending) {
  if (pending == 0)
    return;

  const char *conf = std::getenv("WHICHLIB64_CONF");
  if (!conf)
    conf = "/etc/ld.so.conf";

  std::vector<std::string> dirs;
  parse_ldconfig_conf(conf, dirs);
  for (auto dp : DEFAULT_PATHS)
    dirs.emplace_back(dp);

  search_dirs_batch(dirs, entries, results, pending);
}

static std::string resolve_binary(const char *binary) {
  if (!binary || binary[0] == '\0')
    return {};
  if (std::strchr(binary, '/'))
    return binary;

  const char *path_env = std::getenv("PATH");
  assert(path_env);

  std::string_view cmd(binary);
  char buf[4096];
  std::string result;
  for_each_dir(path_env, [&](std::string_view dir) {
    [[maybe_unused]] bool ok = build_path(buf, sizeof(buf), dir, cmd);
    assert(ok);
    struct stat st;
    if (stat(buf, &st) == 0 && S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
      result = buf;
      return true;
    }
    return false;
  });
  return result;
}

static binary_info parse_binary(const char *binary) {
  binary_info info;
  auto resolved = resolve_binary(binary);
  if (resolved.empty())
    return info;

  MappedFile mf(resolved.c_str());
  if (!mf)
    return info;

  auto *ehdr = mf.at<Elf64_Ehdr>(0);
  if (!ehdr || std::memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    return info;

  auto *phdrs = mf.array_at<Elf64_Phdr>(ehdr->e_phoff, ehdr->e_phnum);
  assert(phdrs);

  const Elf64_Phdr *dyn_phdr = nullptr;
  for (int i = 0; i < ehdr->e_phnum; i++) {
    if (phdrs[i].p_type == PT_DYNAMIC) {
      dyn_phdr = &phdrs[i];
      break;
    }
  }
  assert(dyn_phdr);

  size_t dyn_count = dyn_phdr->p_filesz / sizeof(Elf64_Dyn);
  auto *dyns = mf.array_at<Elf64_Dyn>(dyn_phdr->p_offset, dyn_count);
  assert(dyns);

  Elf64_Addr strtab_vaddr = 0;
  Elf64_Xword rpath_off = 0;
  bool has_rpath = false;

  for (size_t i = 0; i < dyn_count && dyns[i].d_tag != DT_NULL; i++) {
    switch (dyns[i].d_tag) {
    case DT_STRTAB:
      strtab_vaddr = dyns[i].d_un.d_ptr;
      break;
    case DT_RUNPATH:
      rpath_off = dyns[i].d_un.d_val;
      has_rpath = true;
      break;
    case DT_RPATH:
      if (!has_rpath) {
        rpath_off = dyns[i].d_un.d_val;
        has_rpath = true;
      }
      break;
    default:
      break;
    }
  }

  assert(strtab_vaddr);

  Elf64_Off strtab_foff = 0;
  bool found_load = false;
  for (int i = 0; i < ehdr->e_phnum; i++) {
    if (phdrs[i].p_type == PT_LOAD && strtab_vaddr >= phdrs[i].p_vaddr &&
        strtab_vaddr < phdrs[i].p_vaddr + phdrs[i].p_filesz) {
      strtab_foff = phdrs[i].p_offset + (strtab_vaddr - phdrs[i].p_vaddr);
      found_load = true;
      break;
    }
  }
  assert(found_load);

  for (size_t i = 0; i < dyn_count && dyns[i].d_tag != DT_NULL; i++) {
    if (dyns[i].d_tag == DT_NEEDED) {
      size_t noff = strtab_foff + dyns[i].d_un.d_val;
      assert(noff < mf.size());
      info.needed.emplace_back(reinterpret_cast<const char *>(mf.data() + noff));
    }
  }

  if (has_rpath) {
    size_t str_offset = strtab_foff + rpath_off;
    assert(str_offset < mf.size());

    std::string rpath_str(reinterpret_cast<const char *>(mf.data() + str_offset));
    assert(!rpath_str.empty());

    auto origin = fs::path(resolved).parent_path().string();

    info.rpath = rpath_str;
    size_t pos = 0;
    while ((pos = info.rpath.find(ORIGIN_VAR, pos)) != std::string::npos) {
      info.rpath.replace(pos, sizeof(ORIGIN_VAR) - 1, origin);
      pos += origin.size();
    }
  }
  return info;
}

static void
find_lib_batch(const char *binary, std::vector<lib_query> &queries, find_lib_result *results) {
  int count = queries.size();
  for (int i = 0; i < count; i++) {
    results[i].path[0] = '\0';
    results[i].status = FindStatus::NotFound;
  }
  if (count == 0)
    return;

  std::string ldpath_raw;
  if (const char *ldp = std::getenv("LD_LIBRARY_PATH"))
    ldpath_raw = ldp;

  auto info = parse_binary(binary);

  for (int i = 0; i < count; i++) {
    for (const auto &name : info.needed) {
      if (name.size() > queries[i].libname.size() &&
          name.compare(0, queries[i].libname.size(), queries[i].libname) == 0 &&
          name[queries[i].libname.size()] == '.') {
        queries[i].needed_name = name;
        break;
      }
    }
  }

  int pending = count;

  search_pathlist_batch(info.rpath, queries, results, pending);
  search_pathlist_batch(ldpath_raw, queries, results, pending);
  search_cache_batch(queries, results, pending);
  search_default_paths_batch(queries, results, pending);

  for (int i = 0; i < count; i++) {
    if (!results[i].path[0])
      results[i].status = FindStatus::NotFound;
    else
      results[i].status = version_check(results[i].path, queries[i].libname, queries[i].soversion)
                              ? FindStatus::VersionMismatch
                              : FindStatus::Found;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <binary> [<libname>[:<soversion>]...]\n";
    return 1;
  }

  int nlibs = argc - 2;
  std::vector<std::string> names(nlibs);
  std::vector<lib_query> queries(nlibs);
  for (int i = 0; i < nlibs; i++) {
    std::string arg(argv[i + 2]);
    auto colon = arg.rfind(':');
    if (colon != std::string::npos) {
      names[i] = arg.substr(0, colon);
      queries[i] = {names[i], {}, std::atoi(arg.c_str() + colon + 1)};
    } else {
      names[i] = arg;
      queries[i] = {names[i], {}, -1};
    }
  }

  std::vector<find_lib_result> results(nlibs);
  find_lib_batch(argv[1], queries, results.data());

  int ret = 0;
  for (int i = 0; i < nlibs; i++) {
    if (results[i].path[0])
      std::cout << results[i].path << "\n";
    if (results[i].status == FindStatus::NotFound) {
      std::cout << names[i] << " not found\n";
      ret = 1;
    } else if (results[i].status == FindStatus::VersionMismatch && ret == 0)
      ret = 2;
  }

  return ret;
}
