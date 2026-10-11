#!/bin/bash

setup_suite() {
  export MPIRUN=${MPIRUN:-mpirun}
  export PATH=$(pkg-config --variable=bindir thapi):${PATH}

  # Check for ${MPIRUN}, iprof, babeltrace_thapi, jq, and clinfo
  missing_tools=()
  for tool in "${MPIRUN}" iprof babeltrace_thapi jq clinfo; do
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
    echo "(Set MPIRUN to use an MPI launcher other than '${MPIRUN}'.)"
    exit 1
  fi
}
