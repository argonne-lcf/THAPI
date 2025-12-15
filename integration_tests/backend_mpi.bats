@test "backend_mpi_sanity_check" {
   mpicc ./integration_tests/mpi_helloworld.c -o mpi_helloworld
   iprof --backends mpi --analysis-output out.txt -- ./mpi_helloworld
   grep MPI_Init out.txt
   grep MPI_Finalize out.txt
}

# TODO: Working CI MPICH for Fortran IntelMPI tracer
# @test "backend_mpi_sanity_check_fortran" {
#    mpifort ./integration_tests/mpi_helloworld.f90 -o mpi_helloworld_f90
#    iprof --backends mpi --analysis-output out_f90.txt -- ./mpi_helloworld_f90
#    grep MPI_Init out_f90.txt
#    grep MPI_Finalize out_f90.txt
# }

