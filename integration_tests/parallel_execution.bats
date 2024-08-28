#!/usr/bin/env bats

setup_file() {
   export THAPI_HOME=$PWD
   export IPROF=$THAPI_BIN_DIR/iprof
   export MPIRUN=${MPIRUN:-mpirun}
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "sync_daemon_fs" {
   skip "THAPI_SYNC_DAEMON=FS is racy if the apps traced is not calling a global barrier"
   THAPI_SYNC_DAEMON=fs THAPI_JOBID=0 timeout 20s $MPIRUN -n 2 ./integration_tests/light_iprof_only_sync.sh $THAPI_TEST_BIN
}

@test "iprof_fs_launching_mpi_app" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   THAPI_SYNC_DAEMON=fs THAPI_JOBID=0  timeout 40s $MPIRUN -n 2 $IPROF --debug 0 -- ./mpi_helloworld
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
