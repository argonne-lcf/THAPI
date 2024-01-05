#pragma once

#include "xprof_utils.hpp"
#include <cstddef> // Bytes
#include <metababel/metababel.h>
#include <optional>
#include <stdexcept>
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

struct data_s {
  /* Host */
  std::unordered_map<hpt_t, int64_t> entry_ts;

  /* Useful State */
  std::unordered_map<hpt_t, std::optional<std::vector<std::byte>>> last_command;

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
                             thapi_device_id, uint64_t, clock_lttng_device_t>>
      event_payload;
};
typedef struct data_s data_t;

// Push Pop. Verify that we never misuse them
template <class S>
static inline void push_entry_impl(S *state, hpt_t hpt,
                                   std::vector<std::byte> &res) {
  const auto [kv, inserted] =
      state->last_command.emplace(std::make_pair(hpt, res));
  if (!inserted) {
    auto &v = kv->second;
    if (v)
      throw std::runtime_error("push was not empty");
    v = res;
  }
}

template <class S> static inline auto &pop_entry_impl(S *state, hpt_t hpt) {
  const auto it = state->last_command.find(hpt);
  auto &v = it->second;
  if (it == state->last_command.cend() || !v)
    throw std::runtime_error("pop was empty");
  return v;
}

template <class K, class S,
          typename = std::enable_if_t<std::is_trivially_copyable_v<K>>>
static inline void push_entry(S *state, hpt_t hpt, K v) {
  const auto *b = (std::byte *)&v;
  std::vector<std::byte> res{b, b + sizeof(K)};
  push_entry_impl(state, hpt, res);
}

template <class K, class S,
          typename = std::enable_if_t<std::is_trivially_copyable_v<K>>>
static inline K pop_entry(S *state, hpt_t hpt) {
  auto &v = pop_entry_impl(state, hpt);
  const K res{*(K *)(v.value().data())};
  v.reset();
  return res;
}

//-- String specialization
template <class S>
static inline void push_entry(S *state, hpt_t hpt, const std::string s) {
  const auto *b = (std::byte *)s.data();
  std::vector<std::byte> res{b, b + s.size() + 1};
  push_entry_impl(state, hpt, res);
}

template <class K, class S,
          typename = std::enable_if_t<std::is_same_v<K, std::string>>>
inline std::enable_if_t<std::is_same_v<K, std::string>, K>
pop_entry(S *state, hpt_t hpt) {
  auto &v = pop_entry_impl(state, hpt);
  const std::string res{(char *)v.value().data()};
  v.reset();
  return res;
}
