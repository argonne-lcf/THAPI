teardown_file() {
   rm -rf $THAPI_HOME/thapi-traces
}

launch_mpi() {
  # timeout just to avoid burning too much hours when bug are introduced 
  timeout 40s $MPIRUN "$@"
}

# THAPI_SYNC_DAEMON=fs Tests

@test "sync_daemon_fs" {
   THAPI_SYNC_DAEMON=fs launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.sh $CLINFO
}

@test "iprof_fs" {
   THAPI_SYNC_DAEMON=fs launch_mpi -n 2 $IPROF --no-analysis --trace-output "${BATS_TEST_NAME}" -- $CLINFO
   # Count VPID
   [ $($BBT -c "${BATS_TEST_NAME}" | awk -F '[ ,]' '{print $6}' | sort | uniq | wc -l) -eq 2 ]
}

@test "sync_daemon_fs_launching_mpi_app" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   THAPI_SYNC_DAEMON=fs launch_mpi -n 2 $IPROF ./mpi_helloworld
}

# THAPI_SYNC_DAEMON=MPI Tests

@test "sync_daemon_mpi" {
   THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 ./integration_tests/light_iprof_only_sync.sh $CLINFO
}

@test "iprof_mpi" {
   THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 $IPROF --no-analysis --trace-output "${BATS_TEST_NAME}" -- $CLINFO
   # Count VPID
   [ $($BBT -c "${BATS_TEST_NAME}" | awk -F '[ ,]' '{print $6}' | sort | uniq | wc -l) -eq 2 ]
}

@test "sync_daemon_mpi_launching_mpi_app" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   # Current bug in the CI where `mpi_finalize_session` hang
   THAPI_SYNC_DAEMON_MPI_NO_FINALIZE=1 THAPI_SYNC_DAEMON=mpi launch_mpi -n 2 $IPROF ./mpi_helloworld
}
