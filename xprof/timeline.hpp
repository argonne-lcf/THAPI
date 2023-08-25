#pragma once

#include "xprof_utils.hpp" // typedef
#include <unordered_map>
#include <stack>
#include <utility> // pair
#include <babeltrace2/babeltrace.h>
#include "perfetto_prunned.pb.h"

typedef uint64_t perfetto_uuid_t;
typedef uint64_t timestamp_t;

/* Sink component's private data */
struct timeline_dispatch {
    bt_message_iterator *message_iterator;
    // Perfetto
    std::unordered_map<hp_dsd_t, perfetto_uuid_t> hp2uuid;
    std::unordered_map<std::pair<perfetto_uuid_t, thread_id_t>, perfetto_uuid_t> hpt2uuid;
    std::map<perfetto_uuid_t, std::map<timestamp_t,perfetto_uuid_t>> parents2tracks;
    std::map<perfetto_uuid_t, std::stack<timestamp_t>>  uuid2stack;

    std::unordered_map<hp_t, perfetto_uuid_t> hp2frqtracks;
    std::unordered_map<hp_t, perfetto_uuid_t> hp2pwrtracks;
    std::unordered_map<hp_device_t, perfetto_uuid_t> hp_devs2frqtracks;
    std::unordered_map<hp_device_t, perfetto_uuid_t> hp_devs2pwrtracks;

    perfetto_pruned::Trace trace;
};
