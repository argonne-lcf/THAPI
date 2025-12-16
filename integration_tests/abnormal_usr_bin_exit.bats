bats_require_minimum_version 1.5.0

@test "exit_code_propagated" {
   run -55 iprof -- bash -c "exit 55"
   run -55 iprof --no-analysis -- bash -c "exit 55"
}

@test "signaling_propagated_mpi" {
  lock_tmp=${BATS_TEST_TMPDIR}/lock.tmp
  out_txt=${BATS_TEST_TMPDIR}/out.txt
  run -2 iprof --analysis-output ${out_txt} -- bash -c "clinfo &&  echo \$BASHPID > ${lock_tmp} && sleep 100" &
  until [ -f ${lock_tmp} ]; do sleep 1; done
  kill -2 $(cat ${lock_tmp})
  until [ -s ${out_txt} ]; do sleep 1; done
  grep clGetPlatformIDs ${out_txt}
}
