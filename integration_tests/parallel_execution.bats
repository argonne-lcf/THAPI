bats_require_minimum_version 1.5.0

launch_mpi() {
  # timeout just to avoid burning too much hours when bug are introduced
  timeout 40s $MPIRUN "$@"
}

# THAPI_SYNC_DAEMON=fs Tests

@test "sync_daemon_fs" {
  THAPI_SYNC_DAEMON=fs launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.sh clinfo
}

@test "iprof_fs" {
  trace_dir="${BATS_TEST_TMPDIR}/${BATS_TEST_NAME}"
  THAPI_SYNC_DAEMON=fs launch_mpi -n 2 iprof --backend cl --no-analysis --trace-output ${trace_dir} -- clinfo
  # Count VPID
  [ $(babeltrace_thapi -c ${trace_dir} | awk -F '[ ,]' '{print $6}' | sort | uniq | wc -l) -eq 2 ]
}

@test "sync_daemon_fs_launching_mpi_app" {
  mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
  THAPI_SYNC_DAEMON=fs launch_mpi -n 2 iprof ./mpi_helloworld
}

# THAPI_SYNC_DAEMON=MPI Tests

@test "sync_daemon_mpi" {
  THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.sh clinfo
}

@test "iprof_mpi" {
  trace_dir="${BATS_TEST_TMPDIR}/${BATS_TEST_NAME}"
  THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 iprof --backend cl --no-analysis --trace-output ${trace_dir} -- clinfo
  # Count VPID
  [ $(babeltrace_thapi -c ${trace_dir} | awk -F '[ ,]' '{print $6}' | sort | uniq | wc -l) -eq 2 ]
}

@test "sync_daemon_mpi_launching_mpi_app" {
  mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
  # Current bug in the CI where `mpi_finalize_session` hang
  THAPI_SYNC_DAEMON_MPI_NO_FINALIZE=1 THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 iprof ./mpi_helloworld
}

# Test Traced Rank

@test "iprof_mpi+traced_ranks" {
  trace_dir="${BATS_TEST_TMPDIR}/${BATS_TEST_NAME}"
  run -0 launch_mpi -n 2 iprof --backend cl --traced-ranks 1 -- clinfo
  [[ "$output" =~ "1 Hostnames | 1 Processes | 1 Threads |" ]]
}
