#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using namespace std::string_view_literals;

constexpr auto MSG_INIT = "INIT"sv;
constexpr auto MSG_FINISH = "FINISH"sv;
constexpr auto MSG_READY = "READY"sv;

using plugin_initialize_func = void (*)();
using plugin_finalize_func = void (*)();

static int recv_expect(const int fd, const std::string_view want) {
  char buf[64];
  const ssize_t n = read(fd, buf, sizeof(buf));
  if (n < 0) {
    std::perror("thapi_sampling_daemon: read");
    return -1;
  }
  if (n == 0) {
    std::cerr << "thapi_sampling_daemon: parent closed socket unexpectedly" << std::endl;
    return -1;
  }
  const std::string_view got(buf, n);
  if (got != want) {
    std::cerr << "thapi_sampling_daemon: expected " << want << ", got " << got << std::endl;
    return -1;
  }
  return 0;
}

static int send_msg(const int fd, const std::string_view msg) {
  if (write(fd, msg.data(), msg.size()) < 0) {
    std::perror("thapi_sampling_daemon: write");
    return -1;
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <fd> [plugin.so ...]" << std::endl;
    return 1;
  }
  const int fd = std::atoi(argv[1]);

  // DL Open
  struct Plugin {
    void *handle;
    plugin_initialize_func initialize;
    plugin_finalize_func finalize;
  };

  std::vector<Plugin> plugins;

  for (int i = 2; i < argc; ++i) {
    void *handle = dlopen(argv[i], RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
    if (!handle) {
      std::cerr << "Failed to load " << argv[i] << ": " << dlerror() << std::endl;
      continue;
    }
    auto init_func =
        reinterpret_cast<plugin_initialize_func>(dlsym(handle, "thapi_initialize_sampling_plugin"));
    auto fini_func =
        reinterpret_cast<plugin_finalize_func>(dlsym(handle, "thapi_finalize_sampling_plugin"));
    plugins.push_back({handle, init_func, fini_func});
  }

  // User pluging
  for (const auto &plugin : plugins)
    plugin.initialize();

  // Handshake: parent → INIT, daemon → READY
  if (recv_expect(fd, MSG_INIT) < 0)
    return 1;
  if (send_msg(fd, MSG_READY) < 0)
    return 1;

  // Wait for shutdown: parent → FINISH
  if (recv_expect(fd, MSG_FINISH) < 0)
    return 1;

  // Finalization
  for (const auto &plugin : plugins) {
    if (plugin.finalize)
      plugin.finalize();
    dlclose(plugin.handle);
  }

  if (send_msg(fd, MSG_READY) < 0)
    return 1;
  close(fd);
  // Will call the destructor, who will finalize all the not unregistered plugin
  return 0;
}
