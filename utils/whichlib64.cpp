#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
struct find_lib_request {
  const char *libname;
  int soversion;
};

struct find_lib_result {
  char path[4096];
  int status;
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
    auto real = fs::canonical(fpath, ec);
    if (ec || real.filename().string() == libname)
      return {};
    return real.filename().string();
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

struct batch_entry {
  std::string_view libname;
  std::string_view needed_name;
};

// access(full_path) instead of open(dir)+faccessat: on Lustre, open() costs
// 10ms-900ms per directory while access() is ~10us.
static void search_pathlist_batch(std::string_view pathlist,
                                  const std::vector<batch_entry> &entries,
                                  find_lib_result *results,
                                  int &pending) {
  if (pathlist.empty() || pending == 0)
    return;
  char buf[4096];
  for_each_dir(pathlist, [&](std::string_view dir) {
    for (size_t i = 0; i < entries.size(); i++) {
      if (results[i].path[0])
        continue;
      bool found = false;
      if (!entries[i].needed_name.empty()) {
        build_path(buf, sizeof(buf), dir, entries[i].needed_name);
        found = access(buf, F_OK) == 0;
      }
      if (!found) {
        if (entries[i].libname.empty())
          continue;
        build_path(buf, sizeof(buf), dir, entries[i].libname);
        found = access(buf, F_OK) == 0;
      }
      if (found) {
        copy_to_buf(results[i].path, buf);
        if (--pending == 0)
          return true;
      }
    }
    return false;
  });
}

// Single mmap, single scan for all pending libraries
static void search_cache_batch(const std::vector<batch_entry> &entries,
                               find_lib_result *results,
                               int &pending) {
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
        if (access(val, F_OK) == 0) {
          copy_to_buf(results[j].path, val);
          --pending;
        }
      } else if (fallbacks[j].empty() && !entries[j].libname.empty() &&
                 std::strcmp(key, entries[j].libname.data()) == 0) {
        if (access(val, F_OK) == 0)
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

static constexpr std::string_view DEFAULT_PATHS[] = {"/lib64", "/usr/lib64", "/lib", "/usr/lib"};

static void search_default_paths_batch(const std::vector<batch_entry> &entries,
                                       find_lib_result *results,
                                       int &pending) {
  if (pending == 0)
    return;
  char buf[4096];
  for (auto dir : DEFAULT_PATHS) {
    for (size_t i = 0; i < entries.size(); i++) {
      if (results[i].path[0])
        continue;
      bool found = false;
      if (!entries[i].needed_name.empty()) {
        build_path(buf, sizeof(buf), dir, entries[i].needed_name);
        found = access(buf, F_OK) == 0;
      }
      if (!found) {
        if (entries[i].libname.empty())
          continue;
        build_path(buf, sizeof(buf), dir, entries[i].libname);
        found = access(buf, F_OK) == 0;
      }
      if (found) {
        copy_to_buf(results[i].path, buf);
        if (--pending == 0)
          return;
      }
    }
  }
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

static void
parse_binary(const char *binary, std::string &rpath_raw, std::vector<std::string> &needed_names) {
  auto resolved = resolve_binary(binary);
  if (resolved.empty())
    return;

  MappedFile mf(resolved.c_str());
  if (!mf)
    return;

  auto *ehdr = mf.at<Elf64_Ehdr>(0);
  if (!ehdr || std::memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0)
    return;

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
      needed_names.emplace_back(reinterpret_cast<const char *>(mf.data() + noff));
    }
  }

  if (has_rpath) {
    size_t str_offset = strtab_foff + rpath_off;
    assert(str_offset < mf.size());

    std::string rpath_str(reinterpret_cast<const char *>(mf.data() + str_offset));
    assert(!rpath_str.empty());

    auto origin = fs::path(resolved).parent_path().string();

    rpath_raw = rpath_str;
    size_t pos = 0;
    while ((pos = rpath_raw.find(ORIGIN_VAR, pos)) != std::string::npos) {
      rpath_raw.replace(pos, sizeof(ORIGIN_VAR) - 1, origin);
      pos += origin.size();
    }
  }
}

static void find_lib_batch(const char *binary,
                           const find_lib_request *requests,
                           int count,
                           find_lib_result *results) {
  for (int i = 0; i < count; i++) {
    results[i].path[0] = '\0';
    results[i].status = 1;
  }
  if (count == 0)
    return;

  std::string rpath_raw;
  std::string ldpath_raw;
  std::vector<std::string> needed_names;

  if (const char *ldp = std::getenv("LD_LIBRARY_PATH"))
    ldpath_raw = ldp;

  parse_binary(binary, rpath_raw, needed_names);

  std::vector<batch_entry> entries(count);
  for (int i = 0; i < count; i++) {
    entries[i].libname = requests[i].libname;
    for (const auto &name : needed_names) {
      if (name.size() > entries[i].libname.size() &&
          name.compare(0, entries[i].libname.size(), entries[i].libname) == 0 &&
          name[entries[i].libname.size()] == '.') {
        entries[i].needed_name = name;
        break;
      }
    }
  }

  int pending = count;

  search_pathlist_batch(rpath_raw, entries, results, pending);
  search_pathlist_batch(ldpath_raw, entries, results, pending);
  search_cache_batch(entries, results, pending);
  search_default_paths_batch(entries, results, pending);

  for (int i = 0; i < count; i++) {
    if (results[i].path[0] && !is_elf64(results[i].path))
      results[i].path[0] = '\0';
    if (!results[i].path[0])
      results[i].status = 1;
    else
      results[i].status =
          version_check(results[i].path, requests[i].libname, requests[i].soversion) ? 2 : 0;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <binary> [<libname>[:<soversion>]...]\n";
    return 1;
  }

  int nlibs = argc - 2;
  std::vector<std::string> names(nlibs);
  std::vector<find_lib_request> requests(nlibs);
  for (int i = 0; i < nlibs; i++) {
    std::string arg(argv[i + 2]);
    auto colon = arg.rfind(':');
    if (colon != std::string::npos) {
      names[i] = arg.substr(0, colon);
      requests[i] = {names[i].c_str(), std::atoi(arg.c_str() + colon + 1)};
    } else {
      names[i] = arg;
      requests[i] = {names[i].c_str(), -1};
    }
  }

  std::vector<find_lib_result> results(nlibs);
  find_lib_batch(argv[1], requests.data(), nlibs, results.data());

  int ret = 0;
  for (int i = 0; i < nlibs; i++) {
    if (results[i].path[0])
      std::cout << results[i].path << "\n";
    if (results[i].status == 1) {
      std::cout << names[i] << " not found\n";
      ret = 1;
    } else if (results[i].status == 2 && ret == 0)
      ret = 2;
  }

  return ret;
}
