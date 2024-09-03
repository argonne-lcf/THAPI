#!/bin/bash
set -euo pipefail

# Get base real-time signal number
SIGRTMIN=$(kill -l SIGRTMIN)

# Set signals as defined in MPI daemon code
RT_SIGNAL_READY=$((SIGRTMIN + 0))
RT_SIGNAL_GLOBAL_BARRIER=$((SIGRTMIN + 1))
RT_SIGNAL_LOCAL_BARRIER=$((SIGRTMIN + 2))
RT_SIGNAL_FINISH=$((SIGRTMIN + 3))

# Signal handler for capturing signals
handle_signal() {
    echo "$PARENT_PID $(date) |   Received signal $1 from mpi_daemon"
    if [ "$1" == "RT_SIGNAL_READY" ]; then
        SIGNAL_RECEIVED="true"
    fi
}

# Setup trap for ready signal sent from signal daemon
trap 'handle_signal RT_SIGNAL_READY' $RT_SIGNAL_READY

# Function to wait for RT_SIGNAL_READY
wait_for_signal() {
    while [[ "$SIGNAL_RECEIVED" == "false" ]]; do
        sleep 0.1  # Small sleep to prevent busy looping
    done
}

# To avoid race condition, `SIGNAL_RECEIVED` need to be set
#   before spawning or signaling the daemon
spawn_daemon_blocking() {
    local parent_pid=$$
    SIGNAL_RECEIVED="false"
    "${THAPI_BIN_DIR}"/sync_daemon_"${THAPI_SYNC_DAEMON}" parent_pid &
    DAEMON_PID=$!
    wait_for_signal
}

send_signal_blocking() {
    SIGNAL_RECEIVED="false"
    kill -"$1" $DAEMON_PID
    wait_for_signal
}

echo "$PARENT_PID $(date) | Spawn Daemon"
spawn_daemon_blocking
echo "$PARENT_PID $(date) | Send Local Barrier signal"
send_signal_blocking $RT_SIGNAL_LOCAL_BARRIER
# Run test program
"$@"

# Final synchronization after mpi_hello_world execution
echo "$PARENT_PID $(date) | Send Local Barrier signal"
send_signal_blocking $RT_SIGNAL_LOCAL_BARRIER
echo "$PARENT_PID $(date) | Send Global Barrier signal"
send_signal_blocking $RT_SIGNAL_GLOBAL_BARRIER
echo "$PARENT_PID $(date) | Send Termination signal"
send_signal_blocking $RT_SIGNAL_FINISH
echo "$PARENT_PID $(date) | Wait for daemon to quit"
wait $DAEMON_PID
