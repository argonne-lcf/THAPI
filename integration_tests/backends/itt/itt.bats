bats_require_minimum_version 1.5.0

setup() {
  # Useful paths
  ITT_TEST_ROOT="${BATS_TEST_DIRNAME}"
  ITT_SRC_DIR="${ITT_TEST_ROOT}/src"
  ITT_TMP_DIR="${ITT_TEST_ROOT}/tmp"
  mkdir -p "${ITT_TMP_DIR}"
}

teardown_file() {
  # Clean up any traces we created in this file
  rm -rf "${ITT_TMP_DIR}"
}

# Prefer babeltrace2; fall back to THAPI's babeltrace wrapper if available
_bt_pretty() {
  if command -v babeltrace2 >/dev/null 2>&1; then
    babeltrace2 -c sink.text.pretty "$@"
  else
    local bbt_bin="${BBT:-babeltrace_thapi}"
    if command -v "${bbt_bin}" >/dev/null 2>&1; then
      "${bbt_bin}" --no-restrict "$@"
    else
      echo "SKIP: Neither 'babeltrace2' nor '${bbt_bin}' available." >&2
      return 127
    fi
  fi
}

# Precise resolvers for the two Python example scripts
_py_script_path() {
  case "$1" in
    context-manager) printf "%s" "${ITT_SRC_DIR}/itt_example_py/itt_example_context-manager.py" ;;
    c-style)         printf "%s" "${ITT_SRC_DIR}/itt_example_py/itt_example_c-style.py" ;;
    *) return 1 ;;
  esac
}

_require_python_ittapi() {
  if ! command -v python3 >/dev/null 2>&1; then
    skip "python3 not found in PATH; skipping Python ITT tests."
  fi
  python3 - <<'PY'
import sys
try:
    import ittapi  # must be installed via pip
except Exception:
    sys.exit(2)
sys.exit(0)
PY
  case $? in
    0) return 0 ;;
    2) skip "Python package 'ittapi' not installed; skipping Python ITT tests." ;;
    *) skip "Unable to check Python 'ittapi'; skipping Python ITT tests." ;;
  esac
}

# Ensure (or opportunistically build) the C example binary.
_ensure_c_example_exe() {
  local exe="${ITT_SRC_DIR}/itt_example_c/itt_example"
  if [[ -x "${exe}" ]]; then
    printf "%s" "${exe}"
    return 0
  fi

  local ittapi_root="${ITTAPI_ROOT:-}"
  local src_c="${ITT_SRC_DIR}/itt_example_c/itt_example.c"
  if [[ -z "${ittapi_root}" || ! -f "${src_c}" ]]; then
    return 1
  fi

  local cc=""
  for cc in ${CC:-icx clang gcc cc}; do
    if command -v "$cc" >/dev/null 2>&1; then
      break
    fi
    cc=""
  done
  [[ -n "${cc}" ]] || return 1

  local inc="${ittapi_root}/include"
  local libdir=""
  if [[ -d "${ittapi_root}/lib64" ]]; then
    libdir="${ittapi_root}/lib64"
  elif [[ -d "${ittapi_root}/lib" ]]; then
    libdir="${ittapi_root}/lib"
  else
    return 1
  fi

  (
    cd "${ITT_SRC_DIR}/itt_example_c" || exit 1
    "${cc}" itt_example.c -O2 -I"${inc}" -L"${libdir}" -littnotify -o itt_example >/dev/null 2>&1
  )
  [[ -x "${exe}" ]] && printf "%s" "${exe}" && return 0
  return 1
}

@test "ITT (C): trace contains __itt_task_begin / __itt_task_end events" {
  local c_exe
  if ! c_exe="$(_ensure_c_example_exe)"; then
    skip "C ITT example binary not found (and not buildable). Expected at: ${ITT_SRC_DIR}/itt_example_c/itt_example"
  fi

  local trace_dir="${ITT_TMP_DIR}/itt_example_c_CTF"
  rm -rf "${trace_dir}"

  run "${IPROF:-iprof}" --no-analysis --backends itt --trace-output "${trace_dir}" -- "${c_exe}"
  echo "$output"
  [ "$status" -eq 0 ]

  run _bt_pretty "${trace_dir}"
  echo "$output"
  [ "$status" -eq 0 ]
  [ "$(echo "$output" | grep -c 'lttng_ust_itt:__itt_task_begin')" -ge 2 ]
  echo "$output" | grep -q 'Task 1'
}

@test "ITT (Python, context manager): trace contains ITT task events" {
  _require_python_ittapi

  local script
  script="$(_py_script_path 'context-manager')" || skip "Missing context-manager example."
  [[ -f "${script}" ]] || skip "File not found: ${script}"

  local trace_dir="${ITT_TMP_DIR}/itt_example_py_context-manager_CTF"
  rm -rf "${trace_dir}"

  run "${IPROF:-iprof}" --no-analysis --backends itt --trace-output "${trace_dir}" -- python3 "${script}"
  echo "$output"
  [ "$status" -eq 0 ]

  run _bt_pretty "${trace_dir}"
  echo "$output"
  [ "$status" -eq 0 ]
  [ "$(echo "$output" | grep -c 'lttng_ust_itt:__itt_task_begin')" -ge 2 ]
  echo "$output" | grep -q 'Task 2'
}

@test "ITT (Python, C-style): trace contains ITT task events" {
  _require_python_ittapi

  local script
  script="$(_py_script_path 'c-style')" || skip "Missing C-style example."
  [[ -f "${script}" ]] || skip "File not found: ${script}"

  local trace_dir="${ITT_TMP_DIR}/itt_example_py_c-style_CTF"
  rm -rf "${trace_dir}"

  run "${IPROF:-iprof}" --no-analysis --backends itt --trace-output "${trace_dir}" -- python3 "${script}"
  echo "$output"
  [ "$status" -eq 0 ]

  run _bt_pretty "${trace_dir}"
  echo "$output"
  [ "$status" -eq 0 ]
  [ "$(echo "$output" | grep -c 'lttng_ust_itt:__itt_task_begin')" -ge 2 ]
  echo "$output" | grep -q 'Task 2'
}

