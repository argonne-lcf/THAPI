#include <csignal>
#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <vector>
#define RT_SIGNAL_READY (SIGRTMIN)
#define RT_SIGNAL_FINISH (SIGRTMIN + 3)

typedef void (*plugin_initialize_func)(void);
typedef void (*plugin_finalize_func)(void);

int main(int argc, char **argv) {

  // Setup signaling, to exit the sampling loop
  int parent_pid = std::atoi(argv[1]);
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, RT_SIGNAL_FINISH);
  sigprocmask(SIG_BLOCK, &signal_set, NULL);

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
    plugin_initialize_func init_func =
        reinterpret_cast<plugin_initialize_func>(dlsym(handle, "thapi_initialize_sampling_plugin"));

    plugin_finalize_func fini_func =
        reinterpret_cast<plugin_finalize_func>(dlsym(handle, "thapi_finalize_sampling_plugin"));

    plugins.push_back({handle, init_func, fini_func});
  }

  // User pluging
  for (const auto &plugin : plugins) {
    plugin.initialize();
  }

  // Signal Ready to manager
  kill(parent_pid, RT_SIGNAL_READY);

  // Wait for to finish
  while (true) {
    int signum;
    sigwait(&signal_set, &signum);
    if (signum == RT_SIGNAL_FINISH)
      break;
  }

  // Finalization
  for (const auto &plugin : plugins) {
    if (plugin.finalize)
      plugin.finalize();
    dlclose(plugin.handle);
  }

  kill(parent_pid, RT_SIGNAL_READY);

  // Will call the destructor, who will finalize all the not unregistered plugin
  return 0;
}
