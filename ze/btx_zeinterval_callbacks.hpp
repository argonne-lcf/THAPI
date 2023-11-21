#pragma once

#include <metababel/metababel.h>
#include "xprof_utils.hpp"
#include <cstddef> // Bytes
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

typedef std::tuple<hostname_t, process_id_t, ze_event_handle_t> hp_event_t;
typedef std::tuple<hostname_t, process_id_t, ze_kernel_handle_t> hp_kernel_t;
typedef std::tuple<hostname_t, process_id_t, ze_command_list_handle_t>
    hp_command_list_t;
typedef std::tuple<hostname_t, process_id_t, ze_module_handle_t> hp_module_t;

typedef std::map<uintptr_t, uintptr_t> memory_interval_t;

typedef std::tuple<uint64_t, uint64_t> clock_lttng_device_t;
typedef bool to_ignore_t;

struct data_s {
  /* Host */
  std::unordered_map<hpt_t, int64_t> entry_ts;

  /* Useful State */
  std::unordered_map<hpt_t, std::vector<std::byte>> last_command;

  /* Handle memory copy */
  std::unordered_map<hp_module_t, std::unordered_set<uint64_t>>
      module_global_pointer;
  std::unordered_map<hp_t, memory_interval_t> rangeset_memory_device;
  std::unordered_map<hp_t, memory_interval_t> rangeset_memory_host;
  std::unordered_map<hp_t, memory_interval_t> rangeset_memory_shared;

  std::unordered_map<hp_device_t, ze_device_properties_t> device_property;
  std::unordered_map<hp_device_t, thapi_device_id> subdevice_parent;

  std::unordered_map<hp_kernel_t, thapi_function_name> kernel_name;
  std::unordered_map<hp_kernel_t, std::string> kernel_groupsize_str;
  std::unordered_map<hp_kernel_t, std::string> kernel_simdsize_str;

  std::unordered_map<hp_command_list_t, thapi_device_id> command_list_device;

  std::unordered_map<hpt_t,
                     std::tuple<ze_command_list_handle_t, thapi_function_name,
                                std::string, thapi_device_id, uint64_t>>
      command_partial_payload;

  std::unordered_map<hp_device_t, clock_lttng_device_t>
      device_timestamps_pair_ref;
  std::unordered_map<
      hp_event_t, std::tuple<thread_id_t, thapi_function_name, std::string,
                             thapi_device_id, uint64_t, clock_lttng_device_t,
			     to_ignore_t>>
      event_payload;
};
typedef struct data_s data_t;

// Push pop entry
template <class K, class S,
          typename = std::enable_if_t<std::is_trivially_copyable_v<K> ||
                                      std::is_same_v<K, std::string>>>
static inline void push_entry(S *state, hpt_t hpt, K v) {
  const auto *b = (std::byte *)&v;
  state->last_command[hpt] = std::vector<std::byte>(b, b + sizeof(K));
}

template <class K, class S,
          typename = std::enable_if_t<std::is_trivially_copyable_v<K> ||
                                      std::is_same_v<K, std::string>>>
inline K pop_entry(S *state, hpt_t hpt) {
  return *(K *)(state->last_command[hpt].data());
}

// String specialization
template <class S>
void inline push_entry(S *state, hpt_t hpt, const std::string s) {
  const auto *b = (std::byte *)s.data();
  state->last_command[hpt] = std::vector<std::byte>(b, b + s.size() + 1);
}

// Cannot have `class S`...
template <> inline std::string pop_entry(data_t *state, hpt_t hpt) {
  return std::string{(char *)state->last_command[hpt].data()};
}
