#!/usr/bin/env bats

setup_file() {
  export THAPI_INCFLAGS="-I$(pkg-config --variable=includedir thapi)"
  export THAPI_LDFLAGS="-Wl,-rpath,$(pkg-config --variable=libdir thapi) $(pkg-config --libs thapi)"
  # needed for toggle api dlopen test.
  export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(pkg-config --variable=libdir thapi)
}

get_unique_jobid() {
  echo ${BATS_TEST_NAME}.${RANDOM}
}

@test "toggle_api" {
  rm -rf toggle_traces 2>/dev/null

  cc ${THAPI_INCFLAGS} ./integration_tests/toggle.c -o toggle ${THAPI_LDFLAGS}
  iprof --trace-output toggle_traces --no-analysis -- ./toggle

  # Make sure auto_stop comes before stop.
  babeltrace_thapi ./toggle_traces | awk 'BEGIN { seen_auto = 0 }
  $0 ~ /lttng_ust_toggle:auto_stop/ { seen_auto = 1 }
  $0 ~ /lttng_ust_toggle:stop/ { if (seen_auto == 1) { exit 0 } else { exit 1 } }
  '

  # Check expected trace counts.
  dir=$(ls -d -1 ./toggle_traces/*/)
  start_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:start | wc -l)
  [ "$start_count" -eq 1 ]

  stop_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:stop | wc -l)
  [ "$stop_count" -eq 1 ]

  auto_stop_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:auto_stop | wc -l)
  [ "$auto_stop_count" -eq 1 ]
}

@test "toggle_api_dlopen" {
  rm -rf toggle_traces 2>/dev/null

  cc ./integration_tests/toggle_dlopen.c -o toggle_dlopen -ldl
  iprof --trace-output toggle_traces --no-analysis -- ./toggle_dlopen

  # Check expected trace counts.
  dir=$(ls -d -1 ./toggle_traces/*/)
  start_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:start | wc -l)
  [ "$start_count" -eq 2 ]

  stop_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:stop | wc -l)
  [ "$stop_count" -eq 2 ]

  auto_stop_count=$(babeltrace_thapi -c $dir | grep lttng_ust_toggle:auto_stop | wc -l)
  [ "$auto_stop_count" -eq 2 ]
}

# Trace and analyse in a single iprof call: `--toggle-on` gates the events on the
# lttng_ust_toggle start/stop tracepoints, and iprof prints the tally to stdout.
count_base() {
  rm -rf toggle_traces 2>/dev/null

  mpicc ${THAPI_INCFLAGS} ./integration_tests/toggle_mpi.c -o toggle_mpi ${THAPI_LDFLAGS}

  THAPI_SYNC_DAEMON=fs THAPI_JOBID=$(get_unique_jobid) timeout 40s mpirun -n $1 \
    iprof --toggle-on --trace-output toggle_traces -- ./toggle_mpi $2
}

# Count the number of traced API calls: the tally's `Total` row, whose 4th
# `|`-separated column is the call count (see general.bats `default_summary`).
count_traces() {
  count_base $1 $2 | awk -F'|' '/Total/ {c = int($4)} END {print c + 0}'
}

@test "toggle_plugin_mpi_np_1" {
  count_0=$(count_traces 1 0)
  count_1=$(count_traces 1 1)
  count_2=$(count_traces 1 2)

  [ "$count_0" -eq 2 ]
  [ "$count_1" -eq 1 ]
  [ "$count_2" -eq 0 ]
}

# Count the number of processes that emitted traced API calls: the `Processes`
# field of the tally header.
count_processors() {
  count_base $1 $2 | grep -oP '\d+(?= Processes)' || echo 0
}

@test "toggle_plugin_mpi_np_2" {
  count_0=$(count_processors 2 0)
  count_1=$(count_processors 2 1)
  count_2=$(count_processors 2 2)

  [ "$count_0" -eq 2 ]
  [ "$count_1" -eq 1 ]
  [ "$count_2" -eq 0 ]
}

# Count the calls of the API function $1 in the tally read on stdin.
count_calls() {
  awk -F'|' -v name="$1" '$1 ~ name {c = int($4)} END {print c + 0}'
}

# The tests below trace with hand-made LTTng sessions instead of iprof, to
# control what iprof always sets up. Session name ($1, MPI rank id in these
# tests) gets its own session daemon, with LTTNG_HOME set to
# ${BATS_TEST_TMPDIR}/lttng_home/$1, and writes its trace to
# ${BATS_TEST_TMPDIR}/lttng_home/$1/trace. $2 is context or no-context: whether
# events carry the vpid/vtid context. Prints the session's LTTNG_HOME, which
# the traced application and lttng_stop need to reach its session daemon, and
# babeltrace_thapi to find its trace.
#
# The body is a subshell with `set -e`: callers capture the output with $(...),
# where bats would otherwise not stop on a failing command.
lttng_start() (
  set -e
  local home=${BATS_TEST_TMPDIR}/lttng_home/$1
  local out=${home}/trace
  local context=$2

  # Store pipe's write end (fd 1) to fd 3 and point fd 1 (stdout) to
  # fd 2 (stderr).
  exec 3>&1 1>&2

  local fake_hostname_so=${BATS_TEST_TMPDIR}/toggle_fake_hostname.so
  if [ ! -f "${fake_hostname_so}" ]; then
    cc -shared -fPIC ./integration_tests/toggle_fake_hostname.c -o \
      "${fake_hostname_so}"
  fi

  mkdir -p "${home}"
  # The session daemon takes the trace's hostname from gethostname() when
  # the session is created, and LTTng has no option to set it. Preloading
  # toggle_fake_hostname.c overrides gethostname() in this daemon to return
  # the value of THAPI_FAKE_HOSTNAME when it is set.
  LTTNG_HOME="${home}" LD_PRELOAD="${fake_hostname_so}" \
    THAPI_FAKE_HOSTNAME="host$1" lttng-sessiond --daemonize --quiet
  LTTNG_HOME="${home}" lttng create toggle_test -o "${out}"
  LTTNG_HOME="${home}" lttng enable-channel --userspace --blocking-timeout=inf \
    chan
  if [ "${context}" = context ]; then
    LTTNG_HOME="${home}" lttng add-context --userspace --channel=chan \
      -t vpid -t vtid
  fi
  LTTNG_HOME="${home}" lttng enable-event --userspace --channel=chan \
    'lttng_ust_mpi:*,lttng_ust_toggle:*'
  LTTNG_HOME="${home}" lttng start

  # echo lttng_home to pipe's write end.
  echo "${home}" >&3
)

# Stop and destroy the session of the LTTNG_HOME $1 printed by lttng_start.
lttng_stop() {
  LTTNG_HOME="$1" lttng stop
  LTTNG_HOME="$1" lttng destroy
}

# Kill the session daemons started by lttng_start, even if the test failed.
teardown() {
  for pid_file in "${BATS_TEST_TMPDIR}"/lttng_home/*/.lttng/lttng-sessiond.pid; do
    if [ -f "${pid_file}" ]; then
      kill "$(cat "${pid_file}")" || true
    fi
  done
}

# Environment preloading THAPI's MPI tracer into binary $1 (as tracer_mpi.sh
# does).
mpi_tracer_env() {
  local pkglibdir
  pkglibdir=$(pkg-config --variable=libdir thapi)/thapi
  echo LTTNG_UST_ALLOW_BLOCKING=1 \
    LTTNG_UST_MPI_LIBMPI="$(ldd "$1" | awk '/libmpi\.so/ {print $3; exit}')" \
    LD_LIBRARY_PATH="${pkglibdir}/mpi:${LD_LIBRARY_PATH}" \
    LD_PRELOAD="${pkglibdir}/mpi/libmpi.so"
}

# Each rank must be matched against its own hostname, not the hostname of the
# last thapi_start() seen. Fake a two-host run: each rank registers with its own
# session daemon, which records its own hostname (see lttng_start).
@test "toggle_plugin_multi_host" {
  mpicc ${THAPI_INCFLAGS} ./integration_tests/toggle_multi_host.c -o \
    toggle_multi_host ${THAPI_LDFLAGS}

  home0=$(lttng_start 0 context)
  home1=$(lttng_start 1 context)

  # We launch two programs below and each registers with the session daemon based
  # on its own LTTNG_HOME (set using -env).
  local prog=(env $(mpi_tracer_env ./toggle_multi_host) ./toggle_multi_host)
  timeout 40s ${MPIRUN} -n 1 -env LTTNG_HOME "${home0}" "${prog[@]}" : \
    -n 1 -env LTTNG_HOME "${home1}" "${prog[@]}"

  lttng_stop "${home0}"
  lttng_stop "${home1}"

  run babeltrace_thapi tally --toggle-on --backends mpi "${home0}" "${home1}"
  [ "$status" -eq 0 ]
  [[ "$output" =~ "2 Hostnames | 2 Processes" ]]
  # Both ranks called thapi_start() before MPI_Comm_rank.
  [ "$(count_calls MPI_Comm_rank <<<"$output")" -eq 2 ]
}
