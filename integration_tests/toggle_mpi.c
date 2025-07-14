#include <stdlib.h>
#include <mpi.h>

#include <thapi.h>

int main(int argc, char *argv[]) {
  int variant = (argc > 1) ? atoi(argv[1]) : 0;

  MPI_Init(&argc, &argv);

  int rank, size;

  switch (variant) {
  case 0:
    thapi_start();
  case 1:
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) thapi_start();
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    break;
  default:
    break;
  }

  thapi_stop();

  MPI_Finalize();

  return 0;
}
