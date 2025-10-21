program hello_world_mpi
  use mpi
  implicit none
  integer world_rank, world_size, ierror, tag

  call mpi_init(ierror)
  call mpi_comm_size(mpi_comm_world, world_size, ierror)
  call mpi_comm_rank(mpi_comm_world, world_rank, ierror)

  print *, 'hello world from process: ', world_rank, 'of ', world_size
  
  call mpi_finalize(ierror)
end program hello_world_mpi
