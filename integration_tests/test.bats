#!/usr/bin/env bats

setup_file() {
   export THAPI_HOME=$PWD
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "default" {
   $IPROF $THAPI_TEST_BIN
   $IPROF -t $THAPI_TEST_BIN | wc -l
   $IPROF -l -- $THAPI_TEST_BIN
   rm out.pftrace
}

@test "default_mpi" {
   run type mpirun
   if [ "$status" != 0 ]
   then
      skip
   fi
   mpirun -n 12 -- $IPROF $THAPI_TEST_BIN
   mpirun -n 12 -- $IPROF -t $THAPI_TEST_BIN | wc -l
   mpirun -n 12 -- $IPROF -l -- $THAPI_TEST_BIN
   rm out.pftrace
}

@test "iprof_mpi_daemon" {
   run type mpiexec mpicc mpicxx
   if [ "$status" != 0 ]
   then
      skip
   fi

   mpicxx ./integration_tests/iprof_mpi_daemon_test/mpi_hello_world.cpp -o mpi_hello_world
   mpicc ./xprof/sync_daemon_mpi.c -o mpi_daemon

   mpiexec -n2 ./integration_tests/iprof_mpi_daemon_test/test.sh
   rm mpi_hello_world
   rm mpi_daemon
}


@test "replay" {
   $IPROF $THAPI_TEST_BIN
   $IPROF -r
}

@test "no-analsysis" {
   $IPROF --no-analysis -- $THAPI_TEST_BIN
   $IPROF -r 
   $IPROF -t -r | wc -l
   $IPROF -l -r
   rm out.pftrace 
}

@test "trace-output" {
   $IPROF --trace-output trace_1 -- $THAPI_TEST_BIN
   $IPROF -r trace_1
   rm -rf trace_1

   $IPROF --trace-output trace_2 -t -- $THAPI_TEST_BIN | wc -l
   $IPROF -t -r trace_2 | wc -l
   rm -rf trace_2

   $IPROF --trace-output trace_3 -l -- $THAPI_TEST_BIN
   $IPROF -l -r trace_3
   rm -rf trace_3 out.pftrace
}

@test "timeline output" {
   $IPROF -l roger -- $THAPI_TEST_BIN
   rm roger
}

# Assert Failure
@test "failure of replay" {
   $IPROF  -- $THAPI_TEST_BIN
   run $IPROF -t -r
   [ "$status" != 0 ]
   run $IPROF -l -r
   [ "$status" != 0 ]

   $IPROF -l -- $THAPI_TEST_BIN
   run $IPROF -t -r
   [ "$status" != 0 ]
   rm out.pftrace
}
