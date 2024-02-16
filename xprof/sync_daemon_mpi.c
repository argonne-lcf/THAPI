#include <mpi.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>

// Define real-time signals
#define RT_SIGNAL_READY SIGRTMIN
#define RT_SIGNAL_GLOBAL_BARRIER SIGRTMIN + 1
#define RT_SIGNAL_LOCAL_BARRIER SIGRTMIN + 2
#define RT_SIGNAL_FINISH SIGRTMIN + 3

int main(int argc, char **argv) {

  // Initialization
  MPI_Init(&argc, &argv);
  MPI_Comm MPI_COMM_NODE;
  MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL,
                      &MPI_COMM_NODE);

  // Initialize signal set and add signals
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, RT_SIGNAL_GLOBAL_BARRIER);
  sigaddset(&signal_set, RT_SIGNAL_LOCAL_BARRIER);
  sigaddset(&signal_set, RT_SIGNAL_FINISH);

  sigprocmask(SIG_BLOCK, &signal_set, NULL);

  const int parent_pid = atoi(argv[1]);
  kill(parent_pid, RT_SIGNAL_READY);

  // Main loop
  // Non blocked signal will be handled as usual
  while (true) {
    int signum;
    sigwait(&signal_set, &signum);
    if (signum == RT_SIGNAL_FINISH) {
      break;
    } else if (signum == RT_SIGNAL_GLOBAL_BARRIER) {
      // Only local masters should receiv the signal
      int local_size;
      MPI_Comm_size(MPI_COMM_NODE, &local_size);
      int global_rank;
      MPI_Comm_rank(MPI_COMM_WORLD, &global_rank);

      if (global_rank != 0) {
        MPI_Send(&local_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      } else {
        int global_size;
        MPI_Comm_size(MPI_COMM_WORLD, &global_size);
        int sum_local_size_recv = local_size;
        while (sum_local_size_recv != global_size) {
          int local_size_recv;
          MPI_Recv(&local_size_recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
                   MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          sum_local_size_recv += local_size_recv;
        }
      }
    } else if (signum == RT_SIGNAL_LOCAL_BARRIER) {
      MPI_Barrier(MPI_COMM_NODE);
    }
    kill(parent_pid, RT_SIGNAL_READY);
  }

  MPI_Comm_free(&MPI_COMM_NODE);
  MPI_Finalize();
  kill(parent_pid, RT_SIGNAL_READY);
  return 0;
}
