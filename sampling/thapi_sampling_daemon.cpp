#include "thapi_sampling.h"
#include <csignal>
#include <dlfcn.h>
#include <vector>
// LTTng tracepoints heartbeat
#include "sampling.h"
#include <ctime>
#define RT_SIGNAL_READY (SIGRTMIN)
#define RT_SIGNAL_FINISH (SIGRTMIN + 3)

static void thapi_sampling_heartbeat() { do_tracepoint(lttng_ust_sampling, heartbeat, 16); }
static void thapi_sampling_heartbeat2() { do_tracepoint(lttng_ust_sampling, heartbeat2); }

typedef void (*plugin_initialize_func)(void);
typedef void (*plugin_finalize_func)(void);

int main(int argc, char **argv) {

   // Setup signaling, to exit the sampling loop
  int parent_pid = 0;
  parent_pid = atoi(argv[1]);

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
      fprintf(stderr, "Failed to load %s: %s\n", argv[i], dlerror());
      continue;
    }
    plugin_initialize_func init_func =
        reinterpret_cast<plugin_initialize_func>(dlsym(handle, "thapi_initialize_sampling_plugin"));

    plugin_finalize_func fini_func =
        reinterpret_cast<plugin_finalize_func>(dlsym(handle, "thapi_finalize_sampling_plugin"));

    plugins.push_back({handle, init_func, fini_func});
  }

  // Register test sample.
  // TODO: Should be moved in their "sampling_test.so"
  if (getenv("LTTNG_UST_SAMPLING_HEARTBEAT")) {
    struct timespec interval;
    interval.tv_sec = 1;
    interval.tv_nsec = 100000000;
    thapi_register_sampling(&thapi_sampling_heartbeat, &interval);
  }
  if (getenv("LTTNG_UST_SAMPLING_HEARTBEAT2")) {
    struct timespec interval;
    interval.tv_sec = 2;
    interval.tv_nsec = 30000000;
    thapi_register_sampling(&thapi_sampling_heartbeat2, &interval);
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
