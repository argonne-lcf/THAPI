#!/bin/bash
# Usage: IPROF_BIN_DIR=/path/to/iprof/bin THAPI_SYNC_DAEMON=mpi|fs test.sh

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Get base real-time signal number
SIGRTMIN=$(kill -l SIGRTMIN)

# Set signals as defined in MPI daemon code
RT_SIGNAL_READY=$((SIGRTMIN + 0))
RT_SIGNAL_GLOBAL_BARRIER=$((SIGRTMIN + 1))
RT_SIGNAL_LOCAL_BARRIER=$((SIGRTMIN + 2))
RT_SIGNAL_FINISH=$((SIGRTMIN + 3))

SENT_COUNT=0
READY_RECV=0

# Initialize a variable to track signal reception
SIGNAL_RECEIVED=0

# Signal handler for capturing signals
handle_ready_signal() {
    SIGNAL_RECEIVED=$((SIGNAL_RECEIVED + 1))
}

log() {
    echo "$OMPI_COMM_WORLD_RANK$PMI_RANK:" "$@"
}

# Setup trap for ready signal sent from signal daemon
trap 'handle_ready_signal' $RT_SIGNAL_READY

# Function to wait for RT_SIGNAL_READY
wait_for_signal() {
    log "waiting for signal..."
    while [[ "$SIGNAL_RECEIVED" = 0 ]]; do
        sleep 0.1  # Small sleep to prevent busy looping
    done
    SIGNAL_RECEIVED=$((SIGNAL_RECEIVED - 1))
    READY_RECV=$((READY_RECV + 1))
}

# Function to send signals, using adjusted SIGRTMIN corresponding to MPI signal daemon defines
send_signal_blocking() {
    log "send signal $1"
    kill -$1 $DAEMON_PID
    SENT_COUNT=$((SENT_COUNT + 1))
    wait_for_signal
}

# Get the PID of this script
PARENT_PID=$$
# Start sync daemon in the background
${THAPI_BIN_DIR}/sync_daemon_${THAPI_SYNC_DAEMON} $PARENT_PID &
DAEMON_PID=$!
# Wait for daemon to be ready
wait_for_signal
log "Daemon ready"

# Send signals to mpi_daemon to test synchronization
send_signal_blocking $RT_SIGNAL_LOCAL_BARRIER
send_signal_blocking $RT_SIGNAL_GLOBAL_BARRIER
# Run mpi_hello_world
log "Running mpi hello..."
${SCRIPT_DIR}/mpi_hello_world
log "done"

# Final synchronization after mpi_hello_world execution
send_signal_blocking $RT_SIGNAL_LOCAL_BARRIER
send_signal_blocking $RT_SIGNAL_GLOBAL_BARRIER
# Signal to terminate the mpi_daemon
send_signal_blocking $RT_SIGNAL_FINISH

log SENT_COUNT=$SENT_COUNT
log READY_RECV=$READY_RECV

wait $DAEMON_PID  # Ensure daemon exits cleanly
