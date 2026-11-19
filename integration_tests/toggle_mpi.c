#include <mpi.h>
#include <stdlib.h>

#include <thapi.h>

/* Variants, checked by the toggle_mpi_np_* tests in toggle.bats:
 * 0: every rank turns tracing on first, so MPI_Comm_rank and MPI_Comm_size are
 *    traced on every rank.
 * 1: only rank 0 turns tracing on, after MPI_Comm_rank, so only its
 *    MPI_Comm_size is traced.
 * Any other value: tracing is never turned on, so nothing is traced. */

int main(int argc, char *argv[]) {
  int variant = (argc > 1) ? atoi(argv[1]) : 0;

  MPI_Init(&argc, &argv);

  int rank, size;

  switch (variant) {
  case 0:
    thapi_start();
    /* Variant 0 makes the same calls as variant 1, with tracing already on.
     * Rank 0's second thapi_start() leaves it on. */
    /* fall through */
  case 1:
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0)
      thapi_start();
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    break;
  default:
    break;
  }

  thapi_stop();

  MPI_Finalize();

  return 0;
}
