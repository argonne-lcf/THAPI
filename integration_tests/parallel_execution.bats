bats_require_minimum_version 1.5.0

launch_mpi() {
  # timeout just to avoid burning too much hours when bug are introduced
  timeout 40s $MPIRUN "$@"
}

# THAPI_SYNC_DAEMON=fs Tests

@test "sync_daemon_fs" {
  THAPI_SYNC_DAEMON=fs launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.rb clinfo
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
  THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.rb clinfo
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

# Non-MPI runs should ignore THAPI_SYNC_DAEMON entirely (no validation, no spawn).
@test "sync_daemon_ignored_without_mpi" {
  THAPI_SYNC_DAEMON=whatever-not-in-list iprof -- true
}

# Test Traced Rank

@test "iprof_mpi+traced_ranks" {
  trace_dir="${BATS_TEST_TMPDIR}/${BATS_TEST_NAME}"
  run -0 launch_mpi -n 2 iprof --backend cl --traced-ranks 1 -- clinfo
  [[ "$output" =~ "1 Hostnames | 1 Processes | 1 Threads |" ]]
}

@test "launch_usr_bin_streams_child_stdout" {
  # If iprof's input is not buffered, iprof should not buffer it. We use `script`
  # to give iprof a tty (an unbuffered pipe); the helper sleeps 4s between two
  # prints, so under streaming the two lines reach us ~4s apart. `script` runs the
  # child under a PTY, whose line discipline appends a CR to each '\n', hence the
  # trailing `*` in the patterns below.
  helper="${BATS_TEST_TMPDIR}/buffering_helper"
  gcc "${BATS_TEST_DIRNAME}/buffering_helper.c" -o "${helper}"

  ts_first=""
  ts_second=""
  while IFS= read -r line; do
    case "$line" in
      THAPI_STREAM_FIRST*) ts_first=$(date +%s%N) ;;
      THAPI_STREAM_SECOND*)
        ts_second=$(date +%s%N)
        break
        ;;
    esac
  done < <(script -qec "iprof -- '${helper}'" /dev/null)
  [ -n "$ts_first" ]
  [ -n "$ts_second" ]
  delta_ms=$(((ts_second - ts_first) / 1000000))
  echo "delta_ms=$delta_ms"
  [ "$delta_ms" -gt 2000 ]
}

@test "launch_usr_bin_keeps_stderr_separate" {
  # The user binary's stdout and stderr must stay on their respective streams.
  helper="${BATS_TEST_TMPDIR}/buffering_helper"
  gcc "${BATS_TEST_DIRNAME}/buffering_helper.c" -o "${helper}"

  run --separate-stderr iprof --no-analysis -- "${helper}"
  [[ "$output" =~ $'THAPI_STREAM_FIRST\nTHAPI_STREAM_SECOND' ]]
  [[ "$stderr" =~ THAPI_STREAM_STDERR ]]
}
