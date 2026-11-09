#include <mpi.h>
#include <pthread.h>

#include <thapi.h>

/* The main thread turns tracing on, then a second thread makes an MPI call and
 * turns tracing off. Finally, main thread turns tracing off as well. As toggling
 * is per thread, only the main thread's MPI_Comm_rank must be traced: the second
 * thread never called thapi_start(), and its thapi_stop() must not affect the
 * main thread. */

static pthread_barrier_t barrier;

static void *worker(void *arg) {
  (void)arg;

  /* sync1: wait till main thread is done calling thapi_start() */
  pthread_barrier_wait(&barrier);

  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  thapi_stop();

  /* sync2: wait till worker is done calling MPI_Comm_size and thapi_stop() */
  pthread_barrier_wait(&barrier);

  return NULL;
}

int main(int argc, char *argv[]) {
  int provided;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  if (provided < MPI_THREAD_MULTIPLE)
    MPI_Abort(MPI_COMM_WORLD, 1);

  pthread_barrier_init(&barrier, NULL, 2);

  pthread_t thread;
  pthread_create(&thread, NULL /* attr */, worker /* startr_routine */, NULL /* arg */);

  /* main thread calls thapi_start() */
  thapi_start();

  /* sync1: wait till main thread is done calling thapi_start() */
  pthread_barrier_wait(&barrier);

  /* sync2: wait till worker is done calling MPI_Comm_size and thapi_stop() */
  pthread_barrier_wait(&barrier);

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  thapi_stop();

  pthread_join(thread, NULL);
  pthread_barrier_destroy(&barrier);

  MPI_Finalize();

  return 0;
}
