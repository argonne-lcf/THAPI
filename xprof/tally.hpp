#pragma once

#include "xprof_utils.hpp"
#include "tally_utils.hpp"

/* Sink component's private data */
struct tally_dispatch {
    bt_message_iterator *message_iterator;
    std::unordered_map<hpt_function_name_t, StatTime> host;
    std::unordered_map<hpt_device_function_name_t, StatTime> device;
};

