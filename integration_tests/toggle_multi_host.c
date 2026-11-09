#include <mpi.h>

#include <thapi.h>

/* The barrier makes every rank call thapi_start() before any rank calls
 * MPI_Comm_rank, so each MPI_Comm_rank comes after another rank's
 * thapi_start(). The toggle filter must still match it against its own rank's
 * hostname, not the hostname of the last thapi_start() seen. */

int main(int argc, char *argv[]) {
  int rank;

  MPI_Init(&argc, &argv);

  thapi_start();
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  thapi_stop();

  MPI_Finalize();

  return 0;
}
