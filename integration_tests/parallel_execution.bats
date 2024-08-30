#!/usr/bin/env bats

setup_file() {
   export THAPI_HOME=$PWD
   export IPROF=$THAPI_BIN_DIR/iprof
   export MPIRUN=${MPIRUN:-mpirun}
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

# To avoid race condition, the HAPI_SYNC_DAEMON=f tests need to be run:
#   - on one node,
#   - or tracing a MPI appplication
@test "sync_daemon_fs" {
   THAPI_SYNC_DAEMON=fs THAPI_JOBID=0 timeout 20s $MPIRUN -n 2 ./integration_tests/light_iprof_only_sync.sh $THAPI_TEST_BIN
}

@test "iprof_fs" {
   THAPI_SYNC_DAEMON=fs THAPI_JOBID=0  timeout 40s $MPIRUN -n 2 $IPROF --debug 0 -- $THAPI_TEST_BIN
}

@test "sync_daemon_fs_launching_mpi_app" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   THAPI_SYNC_DAEMON=fs THAPI_JOBID=0 timeout 20s $MPIRUN -n 2 ./integration_tests/light_iprof_only_sync.sh ./mpi_helloworld
}

@test "sync_daemon_mpi" {
   THAPI_SYNC_DAEMON=mpi THAPI_JOBID=0 timeout 20s $MPIRUN -n 2 ./integration_tests/light_iprof_only_sync.sh $THAPI_TEST_BIN
}

@test "iprof_mpi" {
   THAPI_SYNC_DAEMON=mpi THAPI_JOBID=0  timeout 40s $MPIRUN -n 2 $IPROF --debug 0 -- $THAPI_TEST_BIN
}

@test "sync_daemon_mpi_launching_mpi_app" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   THAPI_SYNC_DAEMON=mpi THAPI_JOBID=0 timeout 20s $MPIRUN -n 2 ./integration_tests/light_iprof_only_sync.sh ./mpi_helloworld
}
