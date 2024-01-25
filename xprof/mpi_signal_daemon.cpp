#include <mpi.h>
#include <iostream>
#include <vector>
#include <atomic>
#include <signal.h>
#include <cstring>

// Define real-time signals
const int RT_SIGNAL_GLOBAL = SIGRTMIN;
const int RT_SIGNAL_NODE = SIGRTMIN + 1;
const int RT_SIGNAL_RANK = SIGRTMIN + 2;
const int RT_SIGNAL_SET_SPECIAL_FLAG = SIGRTMIN + 3;
const int RT_SIGNAL_SPECIAL_GROUP = SIGRTMIN + 4;

// Define signal handler flags
std::atomic<bool> special_group_flag(false);
std::atomic<bool> exit_flag(false);

// Task functions
void performGlobalTask(int world_rank, MPI_Comm comm) {
    std::cout << "Global rank " << world_rank << " performing global task: waiting for barrier" << std::endl;
    MPI_Barrier(comm);
    std::cout << "Global rank " << world_rank << " performing global task: barrier complete" << std::endl;
}

void performNodeSpecificTask(int world_rank, int local_rank, MPI_Comm comm) {
    std::cout << "Global rank " << world_rank << ", Local rank " << local_rank << " performing node-specific task: waiting for barrier" << std::endl;
    MPI_Barrier(comm);
    std::cout << "Global rank " << world_rank << ", Local rank " << local_rank << " performing node-specific task: barrier complete" << std::endl;
}

void performRankSpecificTask(int world_rank) {
    std::cout << "Global rank " << world_rank << " performing rank-specific task" << std::endl;
}

void performSpecialTask(int world_rank, MPI_Comm world_comm) {
    std::cout << "Global rank " << world_rank << " in special task: waiting for global barrier" << std::endl;
    MPI_Barrier(world_comm);
    std::cout << "Global rank " << world_rank << " in special task: global barrier complete" << std::endl;

    int world_size;
    MPI_Comm_size(world_comm, &world_size);

    // Gather flags from all processes
    int *flags = new int[world_size]();
    int special_group_flag_value = special_group_flag.load() ? 1 : 0;
    MPI_Allgather(&special_group_flag_value, 1, MPI_INT, flags, 1, MPI_INT, world_comm);

    // Determine the ranks that are part of the new group
    std::vector<int> group_ranks;
    for (int i = 0; i < world_size; ++i) {
        if (flags[i] == 1) {
            group_ranks.push_back(i);
        }
    }

    MPI_Group world_group, special_group;
    MPI_Comm special_comm;

    if (special_group_flag) {
        // Can do stuff right away
        std::cout << "Rank " << world_rank << " performing special task" << std::endl;

        // Or can create a new group and communicator if this rank is part of the special task
        MPI_Comm_group(world_comm, &world_group);
        MPI_Group_incl(world_group, group_ranks.size(), group_ranks.data(), &special_group);
        MPI_Comm_create(world_comm, special_group, &special_comm);

        if (special_comm != MPI_COMM_NULL) {
            // Do stuff with the new communicator here
            std::cout << "Rank " << world_rank << ": waiting at barrier for others in special group" << std::endl;
            MPI_Barrier(special_comm);
            std::cout << "Rank " << world_rank << ": barrier complete in special group" << std::endl;

            // Free the communicator
            MPI_Comm_free(&special_comm);
        }
        // Free the group
        MPI_Group_free(&special_group);
    }

    delete[] flags;
    special_group_flag = false;
}

// Signal handling function
void handleSignal(int signum, int world_rank, MPI_Comm world_comm, MPI_Comm local_comm, int local_rank) {
    switch (signum) {
        case SIGTERM:
        case SIGINT:
            exit_flag = true;
            break;
        case RT_SIGNAL_GLOBAL:
            performGlobalTask(world_rank, world_comm);
            break;
        case RT_SIGNAL_NODE:
            performNodeSpecificTask(world_rank, local_rank, local_comm);
            break;
        case RT_SIGNAL_RANK:
            performRankSpecificTask(world_rank);
            break;
        case RT_SIGNAL_SET_SPECIAL_FLAG:
            special_group_flag = true;
            break;
        case RT_SIGNAL_SPECIAL_GROUP:
            performSpecialTask(world_rank, world_comm);
            break;
        default:
            std::cerr << "Unhandled signal: " << signum << std::endl;
            break;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Split world communitcator based on memory shared (N.B. should create communicator for all ranks on the same node)
    MPI_Comm local_comm;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &local_comm);

    int local_rank;
    MPI_Comm_rank(local_comm, &local_rank);

    // Initialize signal handling variables
    sigset_t signal_set;
    int sig;
    int signal_result;

    // Initialize signal set and add signals
    sigemptyset(&set);
    sigaddset(&set, RT_SIGNAL_GLOBAL);
    sigaddset(&set, RT_SIGNAL_NODE);
    sigaddset(&set, RT_SIGNAL_RANK);
    sigaddset(&set, RT_SIGNAL_SET_SPECIAL_FLAG);
    sigaddset(&set, RT_SIGNAL_SPECIAL_GROUP);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGINT);

    // Main loop
    while (!exit_flag) {
        // Wait for a signal
        signal_result = sigwait(&signal_set, &sig);
        if (signal_result == 0) {
            handleSignal(sig, world_rank, MPI_COMM_WORLD, local_comm, local_rank);
        } else {
            std::cerr << "sigwait error: " << strerror(signal_result) << std::endl;
        }
    }

    MPI_Comm_free(&local_comm);
    MPI_Finalize();
    return 0;
}
