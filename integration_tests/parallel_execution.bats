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

@test "iprof_mpi+traced_ranks" {
  trace_dir="${BATS_TEST_TMPDIR}/${BATS_TEST_NAME}"
  run -0 launch_mpi -n 2 iprof --backend cl --traced-ranks 1 -- clinfo
  [[ "$output" =~ "1 Hostnames | 1 Processes | 1 Threads |" ]]
}

@test "launch_usr_bin_streams_child_stdout" {
  # Verify iprof streams the user binary's stdout line-by-line rather than
  # buffering it. The child sleeps 4s between two prints, so under proper
  # streaming the two lines reach us ~4s apart; under buffering they arrive
  # together at child exit. We assert the gap is at least 2s.
  ts_first=""
  ts_second=""
  while IFS= read -r line; do
    case "$line" in
      THAPI_STREAM_FIRST) ts_first=$(date +%s%N) ;;
      THAPI_STREAM_SECOND)
        ts_second=$(date +%s%N)
        break
        ;;
    esac
  done < <(iprof -- bash -c 'echo THAPI_STREAM_FIRST; sleep 4; echo THAPI_STREAM_SECOND' 2>/dev/null)
  [ -n "$ts_first" ]
  [ -n "$ts_second" ]
  delta_ms=$(((ts_second - ts_first) / 1000000))
  echo "delta_ms=$delta_ms"
  [ "$delta_ms" -gt 2000 ]
}
