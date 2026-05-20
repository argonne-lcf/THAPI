// Wire protocol shared between iprof (parent) and the C/C++ daemons
// (sync_daemon_mpi, thapi_sampling_daemon). Messages are short ASCII
// tokens exchanged over a SOCK_SEQPACKET socketpair; each parent
// command is acknowledged by the daemon with MSG_READY.

#pragma once

#include <cstdio>
#include <iostream>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

namespace daemon_proto {

using namespace std::string_view_literals;

constexpr auto MSG_INIT = "INIT"sv;
constexpr auto MSG_LOCAL_BARRIER = "LOCAL_BARRIER"sv;
constexpr auto MSG_GLOBAL_BARRIER = "GLOBAL_BARRIER"sv;
constexpr auto MSG_FINISH = "FINISH"sv;
constexpr auto MSG_READY = "READY"sv;

inline int send_msg(const char *who, int fd, std::string_view msg) {
  if (write(fd, msg.data(), msg.size()) < 0) {
    std::perror(who);
    return -1;
  }
  return 0;
}

// Read one message from fd and verify it matches `want`. Returns 0 on
// match, -1 on syscall failure, EOF, or mismatch.
inline int recv_expect(const char *who, int fd, std::string_view want) {
  char buf[64];
  const ssize_t n = read(fd, buf, sizeof(buf));
  if (n < 0) {
    std::perror(who);
    return -1;
  }
  if (n == 0) {
    std::cerr << who << ": parent closed socket unexpectedly" << std::endl;
    return -1;
  }
  const std::string_view got(buf, n);
  if (got != want) {
    std::cerr << who << ": expected " << want << ", got " << got << std::endl;
    return -1;
  }
  return 0;
}

} // namespace daemon_proto
