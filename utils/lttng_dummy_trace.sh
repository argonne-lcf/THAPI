#!/bin/sh
lttng-sessiond --daemonize --quiet
lttng create thapi-session --output=./thapi-session
lttng enable-channel --userspace --blocking-timeout=inf blocking-channel
lttng add-context --userspace --channel=blocking-channel -t vpid -t vtid
lttng enable-event --channel=blocking-channel --userspace $2
lttng start
LD_PRELOAD=$1 rm
lttng stop
lttng destroy
