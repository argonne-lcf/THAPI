#!/bin/bash

export THAPI_HOME=${THAPI_HOME:-${PWD}}
export THAPI_INSTALL_DIR=${THAPI_INSTALL_DIR:-${PWD}/build/ici}
export THAPI_BIN_DIR=${THAPI_BIN_DIR:-${THAPI_INSTALL_DIR}/bin}
export THAPI_TEST_BIN=${THAPI_TEST_BIN:-clinfo}

export IPROF=${IPROF:-${THAPI_BIN_DIR}/iprof}
export BBT=${BBT:-${THAPI_BIN_DIR}/babeltrace_thapi}
export MPIRUN=${MPIRUN:-mpirun}
