#!/usr/bin/env bats

setup_file() {
   export MPIRUN=${MPIRUN:-mpirun}
   export MPICXX=${MPICXX:-mpicxx}
   export THAPI_SYNC_DAEMON=${THAPI_SYNC_DAEMON:-mpi}

   cd ./integration_tests/sync_daemon_test
   $MPICXX mpi_hello_world.cpp -o mpi_hello_world
   cd -
}

teardown_file() {
   rm ./integration_tests/sync_daemon_test/mpi_hello_world
}

@test "sync_daemon_${THAPI_SYNC_DAEMON}" {
   $MPIRUN -n 2 ./integration_tests/sync_daemon_test/test.sh
}
