#include <mpi.h>
#include <iostream>
#include <vector>
#include <atomic>
#include <signal.h>

// Define real-time signals
const int RT_SIGNAL_GLOBAL = SIGRTMIN;
const int RT_SIGNAL_NODE = SIGRTMIN + 1;
const int RT_SIGNAL_RANK = SIGRTMIN + 2;
const int RT_SIGNAL_SET_SPECIAL_FLAG = SIGRTMIN + 3;
const int RT_SIGNAL_SPECIAL_GROUP = SIGRTMIN + 4;

// Define signal handler flags
std::atomic<bool> perform_global_task(false);
std::atomic<bool> perform_node_task(false);
std::atomic<bool> perform_rank_task(false);
std::atomic<bool> special_group_flag(false);
std::atomic<bool> perform_special_task(false);

// Signal handlers
void globalTaskSignalHandler(int signum) {
    perform_global_task = true;
}

void nodeSpecificTaskSignalHandler(int signum) {
    perform_node_task = true;
}

void rankSpecificTaskSignalHandler(int signum) {
    perform_rank_task = true;
}

void setFlagSignalHandler(int signum) {
    special_group_flag = true;
}

void executeSpecialTaskSignalHandler(int signum) {
    perform_special_task = true;
}

// Task functions
void performGlobalTask(int world_rank, MPI_Comm comm) {
    std::cout << "Global rank " << world_rank << " performing global task: waiting for barrier" << std::endl;
    MPI_Barrier(comm);
    std::cout << "Global rank " << world_rank << " performing global task: barrier complete" << std::endl;
    perform_global_task = false;
}

void performNodeSpecificTask(int world_rank, int local_rank, MPI_Comm comm) {
    std::cout << "Global rank " << world_rank << ", Local rank " << local_rank << " performing node-specific task: waiting for barrier" << std::endl;
    MPI_Barrier(comm);
    std::cout << "Global rank " << world_rank << ", Local rank " << local_rank << " performing node-specific task: barrier complete" << std::endl;
    perform_node_task = false;
}

void performRankSpecificTask(int world_rank) {
    std::cout << "Global rank " << world_rank << " performing rank-specific task" << std::endl;
    perform_rank_task = false;
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
    perform_special_task = false;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    MPI_Comm local_comm;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &local_comm);

    int local_rank;
    MPI_Comm_rank(local_comm, &local_rank);

    // Setup signal handlers
    signal(RT_SIGNAL_GLOBAL, globalTaskSignalHandler);
    signal(RT_SIGNAL_NODE, nodeSpecificTaskSignalHandler);
    signal(RT_SIGNAL_RANK, rankSpecificTaskSignalHandler);
    signal(RT_SIGNAL_SET_SPECIAL_FLAG, setFlagSignalHandler);
    signal(RT_SIGNAL_SPECIAL_GROUP, executeSpecialTaskSignalHandler);


    // Main loop
    while (true) {
        if (perform_global_task) {
            performGlobalTask(world_rank, MPI_COMM_WORLD);
        }
        if (perform_node_task) {
            performNodeSpecificTask(world_rank, local_rank, local_comm);
        }
        if (perform_rank_task) {
            performRankSpecificTask(world_rank);
        }
        if (perform_special_task) {
            performSpecialTask(world_rank, MPI_COMM_WORLD);
        }
    }

    MPI_Comm_free(&local_comm);
    MPI_Finalize();
    return 0;
}
