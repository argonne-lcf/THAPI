#!/bin/bash

setup_suite() {
  export THAPI_HOME=${THAPI_HOME:-${PWD}}
  export THAPI_BIN_DIR=${THAPI_BIN_DIR:-${THAPI_HOME}/install/bin}
  export THAPI_TEST_BIN=${THAPI_TEST_BIN:-clinfo}

  export IPROF=${IPROF:-${THAPI_BIN_DIR}/iprof}
  export BBT=${BBT:-${THAPI_BIN_DIR}/babeltrace_thapi}
  export MPIRUN=${MPIRUN:-mpirun}
  export JQ=${JQ:-jq}

  missing_tools=()

  # Check for clinfo
  if ! command -v "${THAPI_TEST_BIN}" >/dev/null 2>&1; then
    echo "Error: '${THAPI_TEST_BIN}' not found in PATH."
    echo "-> Please install 'clinfo' or ensure it is in your PATH."
    missing_tools+=("clinfo")
  fi

  # Check for iprof
  if ! command -v "${IPROF}" >/dev/null 2>&1; then
    echo "Error: 'iprof' not found in PATH or at \$IPROF."
    echo "-> Please set IPROF, set THAPI_BIN_DIR, or add it to your PATH."
    missing_tools+=("iprof")
  fi

  # Check for babeltrace_thapi
  if ! command -v "${BBT}" >/dev/null 2>&1; then
    echo "Error: 'babeltrace_thapi' not found in PATH or at \$BBT."
    echo "-> Please set BBT, set THAPI_BIN_DIR, or add it to your PATH."
    missing_tools+=("babeltrace_thapi")
  fi

  # Check for mpirun
  if ! command -v "${MPIRUN}" >/dev/null 2>&1; then
    echo "Error: 'mpirun' not found in PATH or at \$MPIRUN."
    echo "-> Please set MPIRUN or add it to your PATH."
    missing_tools+=("mpirun")
  fi

  # Check for jq
  if ! command -v "${JQ}" >/dev/null 2>&1; then
    echo "Error: 'jq' not found in PATH or at \$JQ."
    echo "-> Please set JQ or add it to your PATH."
    missing_tools+=("jq")
  fi

  # Final summary
  if [ ${#missing_tools[@]} -eq 0 ]; then
    echo "All required tools are available."
  else
    echo "Missing tools: ${missing_tools[*]}"
    exit 1
  fi
}
