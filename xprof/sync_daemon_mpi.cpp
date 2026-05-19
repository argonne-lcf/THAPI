#include "daemon_proto.hpp"
#include "mpi.h"
#include <cstdlib>
#include <cstring>

using namespace daemon_proto;

constexpr auto WHO = "sync_daemon_mpi";

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
    else if (strcmp(out_value, "MPI_THREAD_MULTIPLE") == 0) {
    } // Hui: Currently MPI sessions have to use thread multiple because of potential concurrency
      // between sessions.
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

int message_loop(const int fd, MPI_Comm MPI_COMM_WORLD_THAPI, MPI_Comm MPI_COMM_NODE) {
  char buf[64];

  // Processing loop: should only be exited when receiving MSG_FINISH
  while (true) {
    const ssize_t n = read(fd, buf, sizeof(buf));
    if (n < 0) {
      perror(WHO);
      return 1;
    }
    if (n == 0) {
      fprintf(stderr, "%s: parent closed socket unexpectedly\n", WHO);
      return 1;
    }
    const std::string_view msg(buf, n);

    if (msg == MSG_FINISH) {
      return send_msg(WHO, fd, MSG_READY);
    } else if (msg == MSG_LOCAL_BARRIER) {
      MPI_Barrier(MPI_COMM_NODE);
    } else if (msg == MSG_GLOBAL_BARRIER) {
      MPI_Barrier(MPI_COMM_WORLD_THAPI);
    } else if (msg == MSG_INIT) {
      // Initial handshake; no work to do.
    } else {
      fprintf(stderr, "%s: unknown message '%.*s'\n", WHO, static_cast<int>(msg.size()),
              msg.data());
      return 1;
    }

    if (send_msg(WHO, fd, MSG_READY) < 0)
      return 1;
  }
}

int main(int argc, char **argv) {

  // Initialization
  int ret = 0;
  // World Session and Communicator
  MPI_Session lib_shandle = MPI_SESSION_NULL;
  MPI_Comm MPI_COMM_WORLD_THAPI = MPI_COMM_NULL;
  MPI_Comm MPI_COMM_NODE = MPI_COMM_NULL;

  if (argc < 2) {
    fprintf(stderr, "usage: %s <fd>\n", argv[0]);
    return 1;
  }
  const int fd = atoi(argv[1]);

  CHECK_MPI(MPIX_Init_Session(&lib_shandle, &MPI_COMM_WORLD_THAPI));
  CHECK_MPI(MPI_Comm_split_type(MPI_COMM_WORLD_THAPI, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL,
                                &MPI_COMM_NODE));

  ret = message_loop(fd, MPI_COMM_WORLD_THAPI, MPI_COMM_NODE);
  close(fd);

fn_exit:
  if (MPI_COMM_NODE != MPI_COMM_NULL)
    MPI_Comm_free(&MPI_COMM_NODE);
  if (MPI_COMM_WORLD_THAPI != MPI_COMM_NULL)
    MPI_Comm_free(&MPI_COMM_WORLD_THAPI);

  if (lib_shandle != MPI_SESSION_NULL && getenv("THAPI_SYNC_DAEMON_MPI_NO_FINALIZE") == NULL)
    MPI_Session_finalize(&lib_shandle);
  return ret;
}
