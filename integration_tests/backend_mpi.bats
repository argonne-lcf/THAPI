#!/usr/bin/env bats

@test "backend_mpi_sanity_check" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   $IPROF --backends mpi --analysis-output out.txt -- ./mpi_helloworld
   grep MPI_Init out.txt
   grep MPI_Finalize out.txt
}

