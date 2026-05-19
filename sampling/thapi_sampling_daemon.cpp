#include "daemon_proto.hpp"
#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <vector>

using namespace daemon_proto;

using plugin_initialize_func = void (*)();
using plugin_finalize_func = void (*)();

constexpr auto WHO = "thapi_sampling_daemon";

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
  if (recv_expect(WHO, fd, MSG_INIT) < 0)
    return 1;
  if (send_msg(WHO, fd, MSG_READY) < 0)
    return 1;

  // Wait for shutdown: parent → FINISH
  if (recv_expect(WHO, fd, MSG_FINISH) < 0)
    return 1;

  // Finalization
  for (const auto &plugin : plugins) {
    if (plugin.finalize)
      plugin.finalize();
    dlclose(plugin.handle);
  }

  if (send_msg(WHO, fd, MSG_READY) < 0)
    return 1;
  close(fd);
  // Will call the destructor, who will finalize all the not unregistered plugin
  return 0;
}
