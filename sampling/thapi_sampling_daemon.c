#include "thapi_sampling.h"
#include "thapi_sampling_register.h"
#include <dlfcn.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
// Lttng tracepoint for heartbeat
#include "sampling.h"

#define RT_SIGNAL_READY (SIGRTMIN)
#define RT_SIGNAL_FINISH (SIGRTMIN + 3)

static void thapi_sampling_heartbeat() {
  do_tracepoint(lttng_ust_sampling, heartbeat, 16);
}

static void thapi_sampling_heartbeat_finalize() {
  do_tracepoint(lttng_ust_sampling, heartbeat, 32);
}

static void thapi_sampling_heartbeat2() {
  do_tracepoint(lttng_ust_sampling, heartbeat2);
}

typedef void (*plugin_init_func)();

static void signal_handler_finish(int signum) {
  if (signum == RT_SIGNAL_FINISH) {
    thapi_sampling_stop();
  }
}

int main(int argc, char **argv) {

  int parent_pid = 0;
  parent_pid = atoi(argv[1]);

  // Setup signaling, to exit the sampling loop
  struct sigaction sa = { .sa_handler = signal_handler_finish };
  sigemptyset(&sa.sa_mask); // Do not block other signal
  sigaction(RT_SIGNAL_FINISH, &sa, NULL);

  // Initialization
  thapi_sampling_init();

  // Register test sample.
  // TODO: Should be moved in their "sampling_test.so"
  if (getenv("LTTNG_UST_SAMPLING_HEARTBEAT")) {
    struct timespec interval;
    interval.tv_sec = 1;
    interval.tv_nsec = 100000000;
    thapi_register_sampling(&thapi_sampling_heartbeat, &interval,
                            &thapi_sampling_heartbeat_finalize);
  }
  if (getenv("LTTNG_UST_SAMPLING_HEARTBEAT2")) {
    struct timespec interval;
    interval.tv_sec = 2;
    interval.tv_nsec = 30000000;
    thapi_register_sampling(&thapi_sampling_heartbeat2, &interval, NULL);
  }
  // Register user sampling plugin
  for (int i = 2; i < argc; i++) {
    void *handle = dlopen(argv[i], RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
    plugin_init_func f =
        (plugin_init_func)(intptr_t)dlsym(handle, "thapi_register_sampling_plugin");
    f();
  }
  // Signal Ready to manager
  kill(parent_pid, RT_SIGNAL_READY);
  // Run until signal is coming
  thapi_sampling_start_loop();
  // Call destructor
  kill(parent_pid, RT_SIGNAL_READY);

  return 0;
}
