#pragma once

#include "xprof_utils.hpp"
#include "omp-tools.h"
#include <queue>
#include <babeltrace2/babeltrace.h>
#include <unordered_map>
#include <cstddef>
 
struct ompinterval_callbacks_state {
    std::queue<const bt_message*>   downstream_message_queue;
    /* Stack to get begin end */
    std::unordered_map<hpt_t, std::vector<std::byte>> last_command;
    std::unordered_map<hpt_function_name_t, uint64_t>       host_start;
};
