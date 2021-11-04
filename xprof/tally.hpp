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
    int  display_name_max_size;

    std::unordered_map<hpt_function_name_t, TallyCoreTime> host2;
    std::unordered_map<hpt_device_function_name_t, TallyCoreTime> device2;
    std::unordered_map<hpt_function_name_t, TallyCoreByte> traffic2;

    std::unordered_map<hp_device_t, std::string> device_name;
    std::vector<std::string> metadata;
};

