#include "mpi.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define real-time signals
#define RT_SIGNAL_READY SIGRTMIN
#define RT_SIGNAL_GLOBAL_BARRIER SIGRTMIN + 1
#define RT_SIGNAL_LOCAL_BARRIER SIGRTMIN + 2
#define RT_SIGNAL_FINISH SIGRTMIN + 3

#define CHECK_MPI(x)                                                                               \
  do {                                                                                             \
    int retval = (x);                                                                              \
    if (retval != MPI_SUCCESS) {                                                                   \
      fprintf(stderr, "Runtime error: %s returned %d at %s:%d", #x, retval, __FILE__, __LINE__);   \
      ret = -1;                                                                                    \
      goto fn_exit;                                                                                \
    }                                                                                              \
  } while (0)

int MPIX_Init_Session(MPI_Session *lib_shandle, MPI_Comm *lib_comm) {
  /*
   * Create session
   */
  int ret = 0;
  const char mt_key[] = "thread_level";
  const char mt_value[] = "MPI_THREAD_SINGLE";
  MPI_Group wgroup = MPI_GROUP_NULL;
  MPI_Info sinfo = MPI_INFO_NULL;
  MPI_Info tinfo = MPI_INFO_NULL;
  MPI_Info_create(&sinfo);
  MPI_Info_set(sinfo, mt_key, mt_value);
  CHECK_MPI(MPI_Session_init(sinfo, MPI_ERRORS_RETURN, lib_shandle));
  /*
   * check we got thread support level foo library needs
   */
  CHECK_MPI(MPI_Session_get_info(*lib_shandle, &tinfo));
  {
    char out_value[100]; /* large enough */
    int valuelen = sizeof(out_value);
    int flag;
    CHECK_MPI(MPI_Info_get_string(tinfo, mt_key, &valuelen, out_value, &flag));
    if (flag == 0)
      fprintf(stderr, "THAPI_SYNC_DAEMON_MPI Warning: Could not find key %s\n", mt_key);
    if (strcmp(out_value, mt_value))
      fprintf(stderr, "THAPI_SYNC_DAEMON_MPI Warning: Did not get MPI_THREAD_SINGLE, got %s\n",
              out_value);
  }
  /*
   * create a group from the WORLD process set
   */
  {
    const char pset_name[] = "mpi://WORLD";
    CHECK_MPI(MPI_Group_from_session_pset(*lib_shandle, pset_name, &wgroup));
  }
  /*
   * get a communicator
   */
  CHECK_MPI(MPI_Comm_create_from_group(wgroup, "thapi_sync_daemon_mpi", MPI_INFO_NULL,
                                       MPI_ERRORS_RETURN, lib_comm));
/*
 * free group, library doesn’t need it.
 */
fn_exit:
  if (wgroup != MPI_GROUP_NULL)
    MPI_Group_free(&wgroup);
  if (sinfo != MPI_INFO_NULL)
    MPI_Info_free(&sinfo);
  if (tinfo != MPI_INFO_NULL)
    MPI_Info_free(&tinfo);
  if (ret != 0)
    MPI_Session_finalize(lib_shandle);
  return ret;
}

int signal_loop(int parent_pid, MPI_Comm MPI_COMM_WORLD_THAPI, MPI_Comm MPI_COMM_NODE) {

  // Required MPI info
  int global_rank;
  MPI_Comm_rank(MPI_COMM_WORLD_THAPI, &global_rank);
  int global_size;
  MPI_Comm_size(MPI_COMM_WORLD_THAPI, &global_size);
  int local_size;
  MPI_Comm_size(MPI_COMM_NODE, &local_size);

  // Initialize signal set and add signals
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, RT_SIGNAL_GLOBAL_BARRIER);
  sigaddset(&signal_set, RT_SIGNAL_LOCAL_BARRIER);
  sigaddset(&signal_set, RT_SIGNAL_FINISH);

  sigprocmask(SIG_BLOCK, &signal_set, NULL);

  // Send ready to parent
  kill(parent_pid, RT_SIGNAL_READY);
  // Main loop
  // Non blocked signal will be handled as usual
  while (true) {
    int signum;
    sigwait(&signal_set, &signum);
    if (signum == RT_SIGNAL_FINISH) {
      break;
    } else if (signum == RT_SIGNAL_GLOBAL_BARRIER) {
      // Only local master should send signal
      if (global_rank != 0) {
        MPI_Send(&local_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD_THAPI);
      } else {
        int sum_local_size_recv = local_size;
        while (sum_local_size_recv != global_size) {
          int local_size_recv;
          MPI_Recv(&local_size_recv, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD_THAPI,
                   MPI_STATUS_IGNORE);
          sum_local_size_recv += local_size_recv;
        }
      }
    } else if (signum == RT_SIGNAL_LOCAL_BARRIER) {
      MPI_Barrier(MPI_COMM_NODE);
    }
    kill(parent_pid, RT_SIGNAL_READY);
  }
  return 0;
}

int main(int argc, char **argv) {

  // Initialization
  int ret = 0;
  int parent_pid = 0;
  // World Session and Communicator
  MPI_Session lib_shandle = MPI_SESSION_NULL;
  MPI_Comm MPI_COMM_WORLD_THAPI = MPI_COMM_NULL;
  MPI_Comm MPI_COMM_NODE = MPI_COMM_NULL;

  CHECK_MPI(MPIX_Init_Session(&lib_shandle, &MPI_COMM_WORLD_THAPI));
  CHECK_MPI(MPI_Comm_split_type(MPI_COMM_WORLD_THAPI, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL,
                                &MPI_COMM_NODE));

  parent_pid = atoi(argv[1]);
  ret = signal_loop(parent_pid, MPI_COMM_WORLD_THAPI, MPI_COMM_NODE);

fn_exit:
  printf("MPI_Comm_free: MPI_COMM_NODE\n");
  if (MPI_COMM_NODE != MPI_COMM_NULL)
    MPI_Comm_free(&MPI_COMM_NODE);
  printf("MPI_Comm_free: MPI_COMM_WORLD_THAPI\n");
  if (MPI_COMM_WORLD_THAPI != MPI_COMM_NULL)
    MPI_Comm_free(&MPI_COMM_WORLD_THAPI);
  printf("MPI_Session_finalize\n");
  if (lib_shandle != MPI_SESSION_NULL)
    MPI_Session_finalize(&lib_shandle);
  printf("Signal RT_SIGNAL_READY \n");
  if (parent_pid != 0)
    kill(parent_pid, RT_SIGNAL_READY);
  return ret;
}
