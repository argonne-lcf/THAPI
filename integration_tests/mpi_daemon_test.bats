@test "iprof_mpi_daemon" {
   mpicxx ./integration_tests/iprof_mpi_daemon_test/mpi_hello_world.cpp -o mpi_hello_world
   mpicc ./xprof/sync_daemon_mpi.c -o mpi_daemon

   mpiexec -n 2 ./integration_tests/iprof_mpi_daemon_test/test.sh
   rm mpi_hello_world
   rm mpi_daemon
}
