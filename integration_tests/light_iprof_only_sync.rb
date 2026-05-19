#!/usr/bin/env ruby

# Stand-in for the iprof Ruby driver: spawn the configured sync daemon,
# drive the socketpair protocol manually, run the user command in between.
# Used by integration tests to exercise daemon binaries without the rest
# of iprof.

require 'socket'
require 'io/nonblock'

daemon_kind = ENV.fetch('THAPI_SYNC_DAEMON')
daemon = "sync_daemon_#{daemon_kind}"

MSG_INIT           = 'INIT'
MSG_LOCAL_BARRIER  = 'LOCAL_BARRIER'
MSG_GLOBAL_BARRIER = 'GLOBAL_BARRIER'
MSG_FINISH         = 'FINISH'
MSG_READY          = 'READY'

parent, child = UNIXSocket.pair(:SEQPACKET)
# Ruby sets O_NONBLOCK on sockets by default; the daemon uses blocking
# read, so clear it on the inherited fd.
child.nonblock = false
pid = Process.spawn(daemon, child.fileno.to_s, child.fileno => child.fileno)
child.close

send_and_wait = lambda do |msg|
  parent.sendmsg(msg)
  reply, = parent.recvmsg(64)
  raise "expected #{MSG_READY}, got '#{reply}'" unless reply == MSG_READY
end

send_and_wait.call(MSG_INIT)
send_and_wait.call(MSG_LOCAL_BARRIER)

system(*ARGV) || exit($?.exitstatus || 1)

send_and_wait.call(MSG_LOCAL_BARRIER)
send_and_wait.call(MSG_GLOBAL_BARRIER)
send_and_wait.call(MSG_FINISH)
parent.close
Process.wait(pid)
