#!/bin/bash

setup_suite() {
  export THAPI_HOME=${THAPI_HOME:-${PWD}}
  export THAPI_BIN_DIR=${THAPI_BIN_DIR:-${THAPI_HOME}/install/bin}
  export THAPI_TEST_BIN=${THAPI_TEST_BIN:-clinfo}

  export IPROF=${IPROF:-${THAPI_BIN_DIR}/iprof}
  export BBT=${BBT:-${THAPI_BIN_DIR}/babeltrace_thapi}
  export MPIRUN=${MPIRUN:-mpirun}
}
