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

int MPIX_Init_Session(MPI_Session *lib_shandle, MPI_Comm *lib_comm) {

  int rc, flag, valuelen;
  int ret = MPI_SUCCESS;
  const char pset_name[] = "mpi://WORLD";
  const char mt_key[] = "thread_level";
  const char mt_value[] = "MPI_THREAD_SINGLE";
  char out_value[100]; /* large enough */
  MPI_Group wgroup = MPI_GROUP_NULL;
  MPI_Info sinfo = MPI_INFO_NULL;
  MPI_Info tinfo = MPI_INFO_NULL;
  MPI_Info_create(&sinfo);
  MPI_Info_set(sinfo, mt_key, mt_value);
  rc = MPI_Session_init(sinfo, MPI_ERRORS_RETURN, lib_shandle);
  if (rc != MPI_SUCCESS) {
    ret = -1;
    goto fn_exit;
  }
  /*
   * check we got thread support level foo library needs
   */
  rc = MPI_Session_get_info(*lib_shandle, &tinfo);
  if (rc != MPI_SUCCESS) {
    ret = -1;
    goto fn_exit;
  }
  valuelen = sizeof(out_value);
  MPI_Info_get_string(tinfo, mt_key, &valuelen, out_value, &flag);
  if (flag == 0) {
    fprintf(stderr, "THAPI_SYNC_DAEMON_MPI Warning: Could not find key %s\n", mt_key);
    //ret = -1;
    //goto fn_exit;
  }
  if (strcmp(out_value, mt_value)) {
    fprintf(stderr, "THAPI_SYNC_DAEMON_MPI Warning: Did not get MPI_THREAD_SINGLE, got %s\n", out_value);
    //ret = -1;
    //goto fn_exit;
  }
  /*
   * create a group from the WORLD process set
   */
  rc = MPI_Group_from_session_pset(*lib_shandle, pset_name, &wgroup);

  if (rc != MPI_SUCCESS) {
    ret = -1;
    goto fn_exit;
  }
  /*
   * get a communicator
   */
  rc = MPI_Comm_create_from_group(wgroup, "thapi_sync_daemon_mpi", MPI_INFO_NULL, MPI_ERRORS_RETURN,
                                  lib_comm);
  if (rc != MPI_SUCCESS) {
    ret = -1;
    goto fn_exit;
  }
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
  int rc;
  int parent_pid = 0;
  // World Session and Communicator
  MPI_Session lib_shandle = MPI_SESSION_NULL;
  MPI_Comm MPI_COMM_WORLD_THAPI = MPI_COMM_NULL;
  MPI_Comm MPI_COMM_NODE = MPI_COMM_NULL;

  rc = MPIX_Init_Session(&lib_shandle, &MPI_COMM_WORLD_THAPI);
  if (rc != MPI_SUCCESS)
    goto fn_exit;
  rc = MPI_Comm_split_type(MPI_COMM_WORLD_THAPI, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL,
                           &MPI_COMM_NODE);
  if (rc != MPI_SUCCESS)
    goto fn_exit;

  parent_pid = atoi(argv[1]);
  rc = signal_loop(parent_pid, MPI_COMM_WORLD_THAPI, MPI_COMM_NODE);

fn_exit:
  if (MPI_COMM_NODE != MPI_COMM_NULL)
    MPI_Comm_free(&MPI_COMM_NODE);
  if (MPI_COMM_WORLD_THAPI != MPI_COMM_NULL)
    MPI_Comm_free(&MPI_COMM_WORLD_THAPI);
  if (lib_shandle != MPI_SESSION_NULL)
    MPI_Session_finalize(&lib_shandle);
  if (parent_pid != 0)
    kill(parent_pid, RT_SIGNAL_READY);
  return rc;
}
