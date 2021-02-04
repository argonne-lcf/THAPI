#pragma once

#include "xprof_utils.hpp"
#include <queue>
#include <babeltrace2/babeltrace.h>
#include <unordered_map>

#include <ze_api.h>

typedef std::tuple<hostname_t, process_id_t, ze_event_handle_t> hp_event_t;
typedef std::tuple<hostname_t, process_id_t, ze_kernel_handle_t> hp_kernel_t;

typedef std::tuple<hostname_t, process_id_t, ze_command_list_handle_t> hp_command_list_t;
typedef std::tuple<hostname_t, process_id_t, ze_command_queue_handle_t> hp_command_queue_t;

typedef std::tuple<uint64_t, uint64_t> timerResolution_kernelTimestampValidBits_t;


struct zeinterval_callbacks_state {
	std::queue<const bt_message*> downstream_message_queue;
};

