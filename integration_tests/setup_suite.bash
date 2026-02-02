#!/bin/bash

setup_suite() {
  export MPIRUN=${MPIRUN:-mpirun}

  export PATH=$(pkg-config --variable=bindir thapi):${PATH}

  missing_tools=()

  # Check for mpirun
  if ! command -v "${MPIRUN}" >/dev/null 2>&1; then
    if ! command -v "mpirun" >/dev/null 2>&1; then
      echo "Error: 'mpirun' not found in PATH or at \$MPIRUN."
      echo "-> Please set MPIRUN or add it to your PATH."
      missing_tools+=("mpirun")
    fi
  fi

  # Check for iprof, babeltrace_thapi, and jq
  for tool in babeltrace_thapi clinfo iprof jq; do
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
