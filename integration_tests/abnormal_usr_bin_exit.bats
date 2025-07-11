bats_require_minimum_version 1.5.0

load test_env_vars.bash

@test "exit_code_propagated" {
   run -55 $IPROF -- bash -c "exit 55"
   run -55 $IPROF --no-analysis -- bash -c "exit 55"
}

@test "signaling_propagated_mpi" {
   run -2 $IPROF --analysis-output out.txt -- bash -c "$THAPI_TEST_BIN &&  echo \$BASHPID > lock.tmp && sleep 100" &
   until [ -f lock.tmp ]; do sleep 1; done
   kill -2 $(cat lock.tmp)
   until [ -s out.txt ]; do sleep 1; done
   grep clGetPlatformIDs out.txt
}
