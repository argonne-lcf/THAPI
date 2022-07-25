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

    perfetto_pruned::Trace trace;

    std::unordered_map<uint64_t, uint64_t> resclate_queue_uuid;
    std::unordered_map<h_d_t, uint64_t> resclate_device_uuid;
    std::unordered_map<h_dsd_t, uint64_t> resclate_subdevice_uuid;

    std::unordered_map<hostname_t,
                       std::unordered_map<thapi_device_id, uint64_t> > m2;


    std::unordered_map<hostname_t,
                       std::unordered_map<thapi_device_id,
                                          std::unordered_map<thapi_device_id, uint64_t> > > m3;


    std::unordered_map<hostname_t,
                       std::unordered_map<thapi_device_id,
                                          std::unordered_map<thapi_device_id, 
                                                             std::unordered_map<uint64_t, uint64_t>> > > m4;
};


// This only work because we never remove a value in the map
template <class T>
uint64_t rescale_uuid(std::unordered_map<T, uint64_t> &m, T uuid) {
  const uint64_t current_uuid = m.size() + 1 ; // Start at 1, 0 is special
  auto ret = m.insert( {uuid, current_uuid } );
  if (!ret.second) {
    return ret.first->second;
  } else {
    return current_uuid;
  }
}

