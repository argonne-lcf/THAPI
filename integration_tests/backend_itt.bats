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
  gcc ${ITT_SRC_DIR}/itt_example.c -O2 -I${ITTAPI_ROOT}/include -L${ITTAPI_ROOT}/lib64 -littnotify -o ${ITT_TMP_DIR}/itt_example 2>&1
  local out_file="${ITT_TMP_DIR}/itt_out_c.txt"
  $IPROF --backends itt --analysis-output ${out_file} -- ${ITT_TMP_DIR}/itt_example
  grep "Example.Domain:Task 2" ${out_file}
  grep "Example.Domain:Task 1" ${out_file}
}

@test "ITT (Python, context manager): trace contains ITT task events" {
  local script=${ITT_SRC_DIR}/itt_example_context-manager.py
  local out_file="${ITT_TMP_DIR}/itt_out_py_context-manager.txt"
  $IPROF --backends itt --analysis-output ${out_file} -- python3 ${script}
  grep "Example.Domain:Task 2" ${out_file}
  grep "Example.Domain:Task 1" ${out_file}
}

@test "ITT (Python, C-style): trace contains ITT task events" {
  local script="${ITT_SRC_DIR}/itt_example_c-style.py"
  local out_file="${ITT_TMP_DIR}/itt_out_py_c-style.txt"
  $IPROF --backends itt --analysis-output ${out_file} -- python3 ${script}
  grep "Example.Domain:Task 2" ${out_file}
  grep "Example.Domain:Task 1" ${out_file}
}

