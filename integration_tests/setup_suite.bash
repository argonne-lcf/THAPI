#!/bin/bash

setup_suite() {
  export MPIRUN=${MPIRUN:-mpirun}
  export THAPI_TEST_BIN=${THAPI_TEST_BIN:-clinfo}
  export IPROF=iprof
  export BBT=babeltrace_thapi
  export JQ=jq

  missing_tools=()

  # Check for clinfo
  if ! command -v "${THAPI_TEST_BIN}" >/dev/null 2>&1; then
    echo "Error: '${THAPI_TEST_BIN}' not found in PATH."
    echo "-> Please install 'clinfo' or ensure it is in your PATH."
    missing_tools+=("${THAPI_TEST_BIN}")
  fi

  # Check for mpirun
  if ! command -v "${MPIRUN}" >/dev/null 2>&1; then
    echo "Error: 'mpirun' not found in PATH or at \$MPIRUN."
    echo "-> Please set MPIRUN or add it to your PATH."
    missing_tools+=("mpirun")
  fi

  # Check for iprof, babeltrace_thapi, and jq
  for tool in $IPROF $BBT $JQ; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
      echo "Error: '${tool}' not found in PATH -> Please add it to your PATH."
      missing_tools+=("${tool}")
    fi
  done

  # Final summary
  if [ ${#missing_tools[@]} -eq 0 ]; then
    echo "All required tools are available."
  else
    echo "Missing tools: ${missing_tools[*]}"
    exit 1
  fi
}
