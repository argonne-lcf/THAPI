bats_require_minimum_version 1.5.0

setup() {
  # Useful paths
  ITT_TEST_ROOT="${BATS_TEST_DIRNAME}"
  ITT_SRC_DIR="${ITT_TEST_ROOT}"
  ITT_TMP_DIR="${ITT_TEST_ROOT}/tmp"
  mkdir -p "${ITT_TMP_DIR}"
}

teardown_file() {
  # Clean up any traces we created in this file
  rm -rf "${ITT_TMP_DIR}"
}

@test "ITT (C): trace contains __itt_task_begin events" {
  gcc ${ITT_SRC_DIR}/itt_example.c -O2 -I${ITTAPI_ROOT}/include -L${ITTAPI_ROOT}/lib64 -littnotify -o itt_example >/dev/null 2>&1

  local trace_dir="${ITT_TMP_DIR}/itt_example_c_CTF"
  rm -rf "${trace_dir}"

  run "${IPROF:-iprof}" --no-analysis --backends itt --trace-output "${trace_dir}" -- "${c_exe}"
  echo "$output"
  [ "$status" -eq 0 ]

  run "${BBT:-babeltrace_thapi}" "${trace_dir}"
  echo "$output"
  [ "$status" -eq 0 ]
  [ "$(echo "$output" | grep -c 'lttng_ust_itt:__itt_task_begin')" -ge 2 ]
  echo "$output" | grep -q 'Task 1'
}

@test "ITT (Python, context manager): trace contains __itt_task_begin events" {
  local script="${ITT_SRC_DIR}/itt_example_context-manager.py"
  local trace_dir="${ITT_TMP_DIR}/itt_example_py_context-manager_CTF"
  rm -rf "${trace_dir}"

  run "${IPROF:-iprof}" --no-analysis --backends itt --trace-output "${trace_dir}" -- python3 "${script}"
  echo "$output"
  [ "$status" -eq 0 ]

  run "${BBT:-babeltrace_thapi}" "${trace_dir}"
  echo "$output"
  [ "$status" -eq 0 ]
  [ "$(echo "$output" | grep -c 'lttng_ust_itt:__itt_task_begin')" -ge 2 ]
  echo "$output" | grep -q 'Task 2'
}

@test "ITT (Python, C-style): trace contains __itt_task_begin events" {
  local script="${ITT_SRC_DIR}/itt_example_c-style.py"
  local trace_dir="${ITT_TMP_DIR}/itt_example_py_c-style_CTF"
  rm -rf "${trace_dir}"

  run "${IPROF:-iprof}" --no-analysis --backends itt --trace-output "${trace_dir}" -- python3 "${script}"
  echo "$output"
  [ "$status" -eq 0 ]

  run "${BBT:-babeltrace_thapi}" "${trace_dir}"
  echo "$output"
  [ "$status" -eq 0 ]
  [ "$(echo "$output" | grep -c 'lttng_ust_itt:__itt_task_begin')" -ge 2 ]
  echo "$output" | grep -q 'Task 2'
}

