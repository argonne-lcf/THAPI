#!/usr/bin/env bats

setup_file() {
   export THAPI_HOME=$PWD
   export IPROF=$THAPI_BIN_DIR/iprof
   export MPIRUN=${MPIRUN:-mpirun}
}

@test "sync_daemon_mpi_launching_mpi_app" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   THAPI_JOBID=0 $MPIRUN -n 1 $IPROF --backends mpi --analysis-output out.txt -- ./mpi_helloworld
   grep MPI_Init out.txt
   grep MPI_Finalize out.txt
}

