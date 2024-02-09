#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "xprof_utils.hpp"


class EntryState {
public:
  template <class K,
            typename = std::enable_if_t<std::is_trivially_copyable_v<K>>>
  void set_data(hpt_t hpt, K v) {
    const auto *b = (std::byte *)&v;
    std::vector<std::byte> res{b, b + sizeof(K)};
    set_data_impl(hpt, res);
  }

  template <class K,
            typename = std::enable_if_t<std::is_trivially_copyable_v<K>>>
  K get_data(hpt_t hpt) {
    auto &v = set_data_impl(hpt);
    const K res{*(K *)(v.value().data())};
    v.reset();
    return res;
  }

  //-- String specialization
  void set_data(hpt_t hpt, const std::string s) {
    const auto *b = (std::byte *)s.data();
    std::vector<std::byte> res{b, b + s.size() + 1};
    set_data_impl(hpt, res);
  }

  template <class K,
            typename = std::enable_if_t<std::is_same_v<K, std::string>>>
  std::enable_if_t<std::is_same_v<K, std::string>, K>
  get_data(hpt_t hpt) {
    auto &v = set_data_impl(hpt);
    const std::string res{(char *)v.value().data()};
    v.reset();
    return res;
  }

  void set_ts(hpt_t hpt, int64_t ts) {
    entry_ts[hpt] = ts;
  }

  int64_t get_ts(hpt_t hpt) {
    return THAPI_AT(entry_ts, hpt);
  }

private:
  std::unordered_map<hpt_t, std::optional<std::vector<std::byte>>> entry_data;
  std::unordered_map<hpt_t, int64_t> entry_ts;

  void set_data_impl(hpt_t hpt, std::vector<std::byte> &res) {
    const auto [kv, inserted] =
        entry_data.emplace(std::make_pair(hpt, res));
    if (!inserted) {
      auto &v = kv->second;
#ifndef NDEBUG
      if (v)
        throw std::runtime_error("push was not empty");
#endif
      v = res;
    }
  }

  auto &set_data_impl(hpt_t hpt) {
    return THAPI_AT(entry_data, hpt);
  }
};
