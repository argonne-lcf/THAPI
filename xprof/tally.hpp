#pragma once

#include "xprof_utils.hpp"
#include "tally_utils.hpp"

/* Sink component's private data */
struct tally_dispatch {
    bt_message_iterator *message_iterator;
    bool display_compact;
    bool demangle_name;
    bool display_human;
    bool display_metadata;
    std::unordered_map<hpt_function_name_t, StatTime> host;
    std::unordered_map<hpt_device_function_name_t, StatTime> device;
    std::unordered_map<hpt_function_name_t, StatByte> traffic;
    std::unordered_map<hp_device_t, std::string> device_name;
    std::vector<std::string> metadata;
};

