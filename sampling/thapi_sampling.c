#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <stdio.h>
#include "thapi_sampling.h"
#include "sampling.h"
#include "utarray.h"
#include <signal.h>
#include <time.h>

#define RT_SIGNAL_READY (SIGRTMIN)
#define RT_SIGNAL_FINISH (SIGRTMIN + 3)

struct sampling_entry {
  void (*pfn_run)(void);
  struct timespec interval;
  void (*pfn_final)(void);
  struct timespec next;
};

typedef void (*plugin_init_func)();

static pthread_mutex_t thapi_sampling_mutex = PTHREAD_MUTEX_INITIALIZER;
static UT_array *thapi_sampling_events = NULL;

static pthread_once_t thapi_init_once = PTHREAD_ONCE_INIT;
static volatile int thapi_sampling_finished = 0;
static volatile int thapi_sampling_initialized = 0;

static void signal_handler_finish(int signum) {
  printf("signal_handler_finish\n");
  if (signum == RT_SIGNAL_FINISH) {
    thapi_sampling_finished = 1;
  }
}

static void __attribute__((destructor))
thapi_sampling_cleanup() {
  if (!thapi_sampling_initialized)
    return;
  thapi_sampling_finished = 1;
  pthread_mutex_lock(&thapi_sampling_mutex);
  struct sampling_entry **entry = NULL;
  while ((entry = (struct sampling_entry **)utarray_next(thapi_sampling_events, entry))) {
    if ( (*entry)->pfn_final )
        (*entry)->pfn_final();
    free(*entry);
  }
  utarray_free(thapi_sampling_events);
  pthread_mutex_unlock(&thapi_sampling_mutex);
}

static inline int time_cmp(const struct timespec * t1, const struct timespec * t2) {
  if (t1->tv_sec < t2->tv_sec)
    return -1;
  if (t1->tv_sec > t2->tv_sec)
    return 1;
  if (t1->tv_nsec < t2->tv_nsec)
    return -1;
  if (t1->tv_nsec > t2->tv_nsec)
    return 1;
  return 0;
}

static inline int sampling_entry_cmp(const struct sampling_entry **e1, const struct sampling_entry **e2) {
  return time_cmp(&(*e1)->next, &(*e2)->next);
}

static inline int sampling_entry_cmpw(const void * t1, const void * t2) {
  return sampling_entry_cmp((const struct sampling_entry **)t1, (const struct sampling_entry **)t2);
}

static inline void time_add(struct timespec *dest, const struct timespec *t, const struct timespec *d) {
  dest->tv_nsec = t->tv_nsec + d->tv_nsec;
  dest->tv_sec = t->tv_sec + d->tv_sec;
  while (dest->tv_nsec > 999999999) {
    dest->tv_sec += 1;
    dest->tv_nsec -= 1000000000;
  }
}

static void * thapi_sampling_loop(void *args) {
  (void)args;
  while(!thapi_sampling_finished) {
    struct timespec now;
    struct sampling_entry **entry = NULL;

    pthread_mutex_lock(&thapi_sampling_mutex);
    clock_gettime(CLOCK_REALTIME, &now);
    while ((entry = (struct sampling_entry **)utarray_next(thapi_sampling_events, entry)) &&
           time_cmp(&(*entry)->next, &now) < 0) {
      (*entry)->pfn_run();
      time_add(&(*entry)->next, &(*entry)->next, &(*entry)->interval);
      if(time_cmp(&(*entry)->next, &now) < 0)
	      time_add(&(*entry)->next, &now, &(*entry)->interval);
    }
    utarray_sort(thapi_sampling_events, sampling_entry_cmpw);
    entry = (struct sampling_entry **)utarray_front(thapi_sampling_events);
    pthread_mutex_unlock(&thapi_sampling_mutex);
    if (entry)
      while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &(*entry)->next, NULL) && !thapi_sampling_finished) 
        ;
  }
  return NULL;
}

static void thapi_sampling_heartbeat() {
  printf("thapi_sampling\n");
  do_tracepoint(lttng_ust_sampling, heartbeat, 16);
}

static void thapi_sampling_heartbeat_finalize() {
   printf("thapi_sampling_finalize\n");
   do_tracepoint(lttng_ust_sampling, heartbeat, 32);
}

static void thapi_sampling_heartbeat2() {
  printf("thapi_sampling_heartbeat2\n");
  do_tracepoint(lttng_ust_sampling, heartbeat2);
}


static void thapi_sampling_init_once() {
  utarray_new(thapi_sampling_events, &ut_ptr_icd);
  if (!thapi_sampling_events)
    return;
  thapi_sampling_initialized = 1;
}

static int thapi_sampling_init() {
  pthread_once(&thapi_init_once, &thapi_sampling_init_once);
  return 0;
}

void thapi_register_sampling(void (*pfn_run)(void), struct timespec *interval,
					        void (*pfn_final)(void) ) {
  struct sampling_entry *entry = NULL;
  struct timespec now, next;
  if(clock_gettime(CLOCK_REALTIME, &now))
    return;
  time_add(&next, &now, interval);

  pthread_mutex_lock(&thapi_sampling_mutex);
  if (!thapi_sampling_events)
    goto end;
  entry = (struct sampling_entry *)malloc(sizeof(struct sampling_entry));
  if (!entry)
    goto end;
  entry->pfn_run = pfn_run;
  entry->interval = *interval;
  entry->pfn_final = pfn_final;

  entry->next = next;
  utarray_push_back(thapi_sampling_events, &entry);
  utarray_sort(thapi_sampling_events, sampling_entry_cmpw);
end:
  pthread_mutex_unlock(&thapi_sampling_mutex);
  return;
}

int main(int argc, char **argv) {

  int parent_pid = 0;
  parent_pid = atoi(argv[1]);
   
  // Setup signaling, to exist the sampling loop
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, RT_SIGNAL_FINISH);
  signal(RT_SIGNAL_FINISH, signal_handler_finish);

  // Initlixaztion
  thapi_sampling_init();

  // Register test sample    
  if (getenv("LTTNG_UST_SAMPLING_HEARTBEAT")) {
    struct timespec interval;
    interval.tv_sec = 1;
    interval.tv_nsec = 100000000;
    thapi_register_sampling(&thapi_sampling_heartbeat, &interval, &thapi_sampling_heartbeat_finalize);
  }
  if (getenv("LTTNG_UST_SAMPLING_HEARTBEAT2")) {
    struct timespec interval;
    interval.tv_sec = 2;
    interval.tv_nsec = 30000000;
    thapi_register_sampling(&thapi_sampling_heartbeat2, &interval, NULL);
  }

  // Regiser user sample
  for (int i=2 ; i < argc; i++) {
     void *handle = dlopen(argv[i], RTLD_LAZY | RTLD_LOCAL | RTLD_DEEPBIND);
     plugin_init_func f = (plugin_init_func)(intptr_t)dlsym(handle, "thapi_register_sampling_plugin");
     f();
  }

  // Signal Ready to manager
  kill(parent_pid, RT_SIGNAL_READY);

  // Run until signal is comming
  thapi_sampling_loop(NULL);
  // Call destructor
  kill(parent_pid, RT_SIGNAL_READY);

    return 0;
}
