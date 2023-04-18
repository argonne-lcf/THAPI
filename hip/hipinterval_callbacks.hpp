#pragma once

#include "xprof_utils.hpp"
#include <queue>
#include <unordered_map>
#include <babeltrace2/babeltrace.h>

struct hipinterval_callbacks_state {

    std::unordered_map<hpt_function_name_t, uint64_t> host_start;

    std::queue<const bt_message*> downstream_message_queue;

};
