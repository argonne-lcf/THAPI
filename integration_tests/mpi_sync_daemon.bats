#!/usr/bin/env bats

setup_file() {
   export MPIRUN=${MPIRUN:-mpirun}
   export MPICXX=${MPICXX:-mpicxx}
   export THAPI_SYNC_DAEMON=${THAPI_SYNC_DAEMON:-mpi}
   export TEST_EXE=${TEST_EXE:-clinfo}
}

@test "sync_daemon_${THAPI_SYNC_DAEMON}" {
   THAPI_JOBID=0 $MPIRUN -n 2 ./integration_tests/sync_daemon_test/test.sh
}
