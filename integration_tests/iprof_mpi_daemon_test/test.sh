#!/bin/bash

# Get base real-time signal number
SIGRTMIN=$(kill -l SIGRTMIN)

# Set signals as defined in MPI daemon code
RT_SIGNAL_READY=$((SIGRTMIN + 0))
RT_SIGNAL_GLOBAL_BARRIER=$((SIGRTMIN + 1))
RT_SIGNAL_LOCAL_BARRIER=$((SIGRTMIN + 2))
RT_SIGNAL_FINISH=$((SIGRTMIN + 3))

# Signal handler for capturing signals
handle_signal() {
    echo "Received signal $1 from mpi_daemon"
}

# Setup trap for ready signal sent from signal daemon
trap 'handle_signal RT_SIGNAL_READY' $((SIGRTMIN + 0))

# Get the PID of this script
PARENT_PID=$$

# Start mpi_daemon in the background
./mpi_daemon $PARENT_PID &
DAEMON_PID=$!

# Allow the daemon to initialize
sleep 2

# Function to send signals, using adjusted SIGRTMIN corresponding to MPI signal daemon defines
send_signal() {
    kill -$1 $DAEMON_PID
}

# Send signals to mpi_daemon to test synchronization
send_signal $RT_SIGNAL_LOCAL_BARRIER
sleep 1  # Allow local barrier to process

send_signal $RT_SIGNAL_GLOBAL_BARRIER
sleep 1  # Allow global barrier to process

# Run mpi_hello_world
./mpi_hello_world

# Final synchronization after mpi_hello_world execution
send_signal $RT_SIGNAL_LOCAL_BARRIER
sleep 1

send_signal $RT_SIGNAL_GLOBAL_BARRIER
sleep 1

# Signal to terminate the mpi_daemon
send_signal $RT_SIGNAL_FINISH
#wait $DAEMON_PID  # Ensure daemon exits cleanly //FIXME this gives non-zero exit codes
