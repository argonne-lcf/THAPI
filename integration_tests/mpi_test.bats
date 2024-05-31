#!/usr/bin/env bats

setup_file() {
   export THAPI_HOME=$PWD
   export IPROF=$THAPI_BIN_DIR/iprof
   export MPIRUN=${MPIRUN:-mpirun}
   export THAPI_SYNC_DAEMON=${THAPI_SYNC_DAEMON:-mpi}
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "default_mpi_sync_${THAPI_SYNC_DAEMON}" {
   run THAPI_JOBID=0 $MPIRUN -n 12 $IPROF --debug 0 -- $THAPI_TEST_BIN
   run THAPI_JOBID=1 $MPIRUN -n 12 $IPROF --debug 0 -- $THAPI_TEST_BIN
   run THAPI_JOBID=2 $MPIRUN -n 12 $IPROF --debug 0 -l -- $THAPI_TEST_BIN
   rm out.pftrace
}
