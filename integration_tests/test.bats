#!/usr/bin/env bats

setup_file() {
   export IPROF=$HOME/THAPI/build/ici/bin/iprof
   export THAPI_HOME=$PWD
}

teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

@test "default" {
   $IPROF sycl-ls
   $IPROF -t sycl-ls | wc -l
   $IPROF -l -- sycl-ls
   rm out.pftrace
}

@test "default_mpi" {
   mpirun -n 12 -- $IPROF sycl-ls
   mpirun -n 12 -- $IPROF -t sycl-ls | wc -l
   mpirun -n 12 -- $IPROF -l -- sycl-ls
   rm out.pftrace
}

@test "replay" {
   $IPROF sycl-ls
   $IPROF -r
}

@test "no-analsysis" {
   $IPROF --no-analysis -- sycl-ls
   $IPROF -r 
   $IPROF -t -r | wc -l
   $IPROF -l -r
   rm out.pftrace 
}

@test "trace-output" {
   $IPROF --trace-output trace_1 -- sycl-ls
   $IPROF -r trace_1
   rm -rf trace_1

   $IPROF --trace-output trace_2 -t -- sycl-ls | wc -l
   $IPROF -t -r trace_2 | wc -l
   rm -rf trace_2

   $IPROF --trace-output trace_3 -l -- sycl-ls
   $IPROF -l -r trace_3
   rm -rf trace_3 out.pftrace
}

@test "timeline output" {
   $IPROF -l roger -- sycl-ls
   rm roger
}

# Assert Failure
@test "failure of replay" {
   $IPROF  -- sycl-ls
   run $IPROF -t -r
   [ "$status" != 0 ]
   run $IPROF -l -r
   [ "$status" != 0 ]

   $IPROF -l -- sycl-ls
   run $IPROF -t -r
   [ "$status" != 0 ]
   rm out.pftrace
}


