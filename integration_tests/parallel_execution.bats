bats_require_minimum_version 1.5.0

launch_mpi() {
  # timeout just to avoid burning too much hours when bug are introduced
  timeout 40s $MPIRUN "$@"
}

# THAPI_SYNC_DAEMON=fs Tests

@test "sync_daemon_fs" {
  THAPI_SYNC_DAEMON=fs launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.sh clinfo
}

# bats test_tags=issue_489
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

# bats test_tags=mpi_sync_daemon
@test "sync_daemon_mpi" {
  THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.sh clinfo
}

# bats test_tags=mpi_sync_daemon
@test "iprof_mpi" {
  trace_dir="${BATS_TEST_TMPDIR}/${BATS_TEST_NAME}"
  THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 iprof --backend cl --no-analysis --trace-output ${trace_dir} -- clinfo
  # Count VPID
  [ $(babeltrace_thapi -c ${trace_dir} | awk -F '[ ,]' '{print $6}' | sort | uniq | wc -l) -eq 2 ]
}

# bats test_tags=mpi_sync_daemon
@test "sync_daemon_mpi_launching_mpi_app" {
  mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
  # Current bug in the CI where `mpi_finalize_session` hang
  THAPI_SYNC_DAEMON_MPI_NO_FINALIZE=1 THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 iprof ./mpi_helloworld
}

# Test Traced Rank

# bats test_tags=issue_489
@test "iprof_mpi+traced_ranks" {
  trace_dir="${BATS_TEST_TMPDIR}/${BATS_TEST_NAME}"
  run -0 launch_mpi -n 2 iprof --backend cl --traced-ranks 1 -- clinfo
  [[ "$output" =~ "1 Hostnames | 1 Processes | 1 Threads |" ]]
}

# Regression: launch_usr_bin used to set sync on the read end of the popen
# pipe (a no-op) instead of $stdout, so under mpiexec the per-rank app output
# was buffered inside iprof and printed *after* the aggregated summary.
@test "stdout_ordering_under_mpiexec" {
  mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
  run -0 launch_mpi -n 12 iprof --backend cl -- ./mpi_helloworld
  hello_line=$(echo "$output" | grep -n "Hello world from processor" | tail -1 | cut -d: -f1)
  total_line=$(echo "$output" | grep -n "^[[:space:]]*Total" | head -1 | cut -d: -f1)
  [ -n "$hello_line" ]
  [ -n "$total_line" ]
  [ "$hello_line" -lt "$total_line" ]
}
