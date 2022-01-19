#pragma once

#include "xprof_utils.hpp" // typedef
#include <unordered_map>

/* Sink component's private data */
struct timeline_dispatch {
    bt_message_iterator *message_iterator;
    std::unordered_map<hp_t,int> s_gtf_pid;
    std::unordered_map<hp_dsd_t,int> s_gtf_pid_gpu;
};
