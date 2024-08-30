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
    char out_value[100] = {0};
    int valuelen = sizeof(out_value);
    int flag;
    CHECK_MPI(MPI_Info_get(tinfo, mt_key, valuelen, out_value, &flag));
    if (flag == 0)
      fprintf(stderr, "THAPI_SYNC_DAEMON_MPI Warning: Could not find key %s\n", mt_key);
    else if (strcmp(out_value, mt_value))
      fprintf(stderr, "THAPI_SYNC_DAEMON_MPI Warning: Did not get %s, got %s\n", mt_value,
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
  // Initialize signal set and add signals
  sigset_t signal_set;
  sigemptyset(&signal_set);
  sigaddset(&signal_set, RT_SIGNAL_GLOBAL_BARRIER);
  sigaddset(&signal_set, RT_SIGNAL_LOCAL_BARRIER);
  sigaddset(&signal_set, RT_SIGNAL_FINISH);
  sigprocmask(SIG_BLOCK, &signal_set, NULL);
  kill(parent_pid, RT_SIGNAL_READY);

  // Processing loop:
  //   Should be only exited when receiving RT_SIGNAL_FINISH
  while (true) {
    int signum;
    sigwait(&signal_set, &signum);
    if (signum == RT_SIGNAL_FINISH) {
      // Ready signal will be sent after cleaning
      return 0;
    } else if (signum == RT_SIGNAL_LOCAL_BARRIER) {
      MPI_Barrier(MPI_COMM_NODE);
    } else if (signum == RT_SIGNAL_GLOBAL_BARRIER) {
      MPI_Barrier(MPI_COMM_WORLD_THAPI);
    } else {
      fprintf(stderr, "Wrong signal received %d. Exiting", signum);
      return 1;
    }
    kill(parent_pid, RT_SIGNAL_READY);
  }

  // Unreachable
  fprintf(stderr, "Wrong signal_loop exit");
  return 1;
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
  if (MPI_COMM_NODE != MPI_COMM_NULL)
    MPI_Comm_free(&MPI_COMM_NODE);
  if (MPI_COMM_WORLD_THAPI != MPI_COMM_NULL)
    MPI_Comm_free(&MPI_COMM_WORLD_THAPI);
  if (lib_shandle != MPI_SESSION_NULL)
    MPI_Session_finalize(&lib_shandle);
  if (parent_pid != 0)
    kill(parent_pid, RT_SIGNAL_READY);
  return ret;
}
