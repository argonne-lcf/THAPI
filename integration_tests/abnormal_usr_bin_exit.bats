setup_file() {
   export THAPI_HOME=$PWD
   export IPROF=$THAPI_BIN_DIR/iprof
   export MPIRUN=${MPIRUN:-mpirun}
}

@test "exit_code_propagated" {
   run $IPROF -- bash -c "exit 55"
   [ "$status" == 55 ]

   run $IPROF --no-analysis -- bash -c "exit 55"
   [ "$status" == 55 ]
}

@test "signaling_propagated" {
   run $IPROF --analysis-output out.txt -- bash -c "$THAPI_TEST_BIN && touch lock.tmp && sleep 100" &
   while [ ! -f lock.tmp ]; do sleep 1; done
   kill -2 %   
   [ "$status" == 2 ]
   grep clGetDeviceInfo out.txt
}
