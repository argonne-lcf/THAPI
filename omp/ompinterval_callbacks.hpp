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
/*
template <class K,
          typename = std::enable_if_t<std::is_trivially_copyable_v<K> || std::is_same_v<K, std::string>>>
static inline void save_start(ompinterval_callbacks_state* state, hpt_t hpt, K v){
    const auto b = (std::byte*) &v;
    state->last_command[hpt] = std::vector<std::byte>(b, b+sizeof(K));
}

template <>
void save_start(ompinterval_callbacks_state* state, hpt_t hpt, const std::string s){
    const auto b = (std::byte*) s.data();
    state->last_command[hpt] = std::vector<std::byte>(b, b + s.size() + 1);
}

template <class K,
         typename = std::enable_if_t<std::is_trivially_copyable_v<K> || std::is_same_v<K, std::string>>>
static inline K retrieve_start(ompinterval_callbacks_state* state, hpt_t hpt){
    return *(K*)(state->last_command[hpt].data());
}

template <>
std::string retrieve_start(ompinterval_callbacks_state* state, hpt_t hpt){
    return std::string( (char*) state->last_command[hpt].data());
}
*/
