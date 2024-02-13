#pragma once

#include "babeltrace2/babeltrace.h"
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <optional>
#include <type_traits>
#include <vector>
#include <stdexcept>

enum backend_e {
  BACKEND_UNKNOWN = 0,
  BACKEND_ZE = 1,
  BACKEND_OPENCL = 2,
  BACKEND_CUDA = 3,
  BACKEND_OMP_TARGET_OPERATIONS = 4,
  BACKEND_OMP = 5,
  BACKEND_HIP = 6,
  BACKEND_MAX,
};

constexpr const char *pretty_backend_name[] = {
    "unknown",
    "ze",
    "cl",
    "cuda",
    "omp_target",
    "omp",
    "hip",
};

constexpr const char *backend_name[] = {
    "BACKEND_UNKNOWN",
    "BACKEND_ZE",
    "BACKEND_OPENCL",
    "BACKEND_CUDA",
    "BACKEND_OMP_TARGET_OPERATIONS",
    "BACKEND_OMP",
    "BACKEND_HIP",
};

typedef enum backend_e backend_t;
typedef unsigned backend_level_t;
typedef std::string thapi_metadata_t;

// Datatypes representing string classes (or messages) common data.
typedef intptr_t process_id_t;
typedef uintptr_t thread_id_t;
typedef std::string hostname_t;
typedef std::string thapi_function_name;
typedef uintptr_t thapi_device_id;
typedef uint32_t thapi_domain_idx;
typedef uint32_t thapi_sdevice_idx;

// Represent a device and a sub device
typedef std::tuple<thapi_device_id, thapi_device_id> dsd_t;
typedef std::tuple<hostname_t, process_id_t> hp_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t> hpt_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_function_name> hpt_function_name_t;
typedef std::tuple<thread_id_t, thapi_function_name> t_function_name_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_device_id, thapi_device_id,
                   thapi_function_name>
    hpt_device_function_name_t;
typedef std::tuple<hostname_t, process_id_t, thapi_device_id> hp_device_t;
typedef std::tuple<hostname_t, process_id_t, thapi_device_id, thapi_device_id> hp_dsd_t;
typedef std::tuple<hostname_t, process_id_t, thapi_device_id, thapi_domain_idx> hp_ddomain_t;
typedef std::tuple<hostname_t, process_id_t, thapi_device_id, thapi_sdevice_idx> hp_dsdev_t;
typedef std::tuple<long, long> sd_t;
typedef std::tuple<thread_id_t, thapi_function_name, long> tfn_ts_t;
typedef std::tuple<thapi_function_name, long> fn_ts_t;

// Most efficient possible access when NDEBUG is set; if not
// set, use .at which can be debugged with stack trace in
// valgrind's memcheck.
template <class M, class K>
inline auto& thapi_at(M& map, K key) {
#ifdef THAPI_DEBUG
  return map.at(key);
#else
  return map[key];
#endif
}

// NOTE: Required to generate the hash of a tuple.
// REFERENCE:
// https://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
namespace std {
namespace {
// Code from boost
// Reciprocal of the golden ratio helps spread entropy
//     and handles duplicates.
// See Mike Seymour in magic-numbers-in-boosthash-combine:
//     https://stackoverflow.com/questions/4948780
template <class T> inline void hash_combine(std::size_t &seed, T const &v) {
  seed ^= hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Recursive template code derived from Matthieu M.
template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1> struct HashValueImpl {
  static void apply(size_t &seed, Tuple const &tuple) {
    HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
    hash_combine(seed, get<Index>(tuple));
  }
};

template <class Tuple> struct HashValueImpl<Tuple, 0> {
  static void apply(size_t &seed, Tuple const &tuple) { hash_combine(seed, get<0>(tuple)); }
};
} // namespace

template <typename... TT> struct hash<std::tuple<TT...>> {
  size_t operator()(std::tuple<TT...> const &tt) const {
    size_t seed = 0;
    HashValueImpl<std::tuple<TT...>>::apply(seed, tt);
    return seed;
  }
};

template <typename... TT> struct hash<std::pair<TT...>> {
  size_t operator()(std::pair<TT...> const &tt) const {
    size_t seed = 0;
    HashValueImpl<std::pair<TT...>>::apply(seed, tt);
    return seed;
  }
};
} // namespace std


// Save Entry State so can be accessed in Exit
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
  std::enable_if_t<std::is_same_v<K, std::string>, K> get_data(hpt_t hpt) {
    auto &v = set_data_impl(hpt);
    const std::string res{(char *)v.value().data()};
    v.reset();
    return res;
  }

  // Timestamp handling
  void set_ts(hpt_t hpt, int64_t ts) { entry_ts[hpt] = ts; }
  int64_t get_ts(hpt_t hpt) { return thapi_at(entry_ts, hpt); }

private:
  std::unordered_map<hpt_t, std::optional<std::vector<std::byte>>> entry_data;
  std::unordered_map<hpt_t, int64_t> entry_ts;

  void set_data_impl(hpt_t hpt, std::vector<std::byte> &res) {
    const auto [kv, inserted] = entry_data.emplace(std::make_pair(hpt, res));
    if (!inserted) {
      auto &v = kv->second;
#ifndef NDEBUG
      if (v)
        throw std::runtime_error("push was not empty");
#endif
      v = res;
    }
  }

  std::optional<std::vector<std::byte>> &set_data_impl(hpt_t hpt) { return thapi_at(entry_data, hpt); }
};

// TODO Delete them as soon as no more metabable

const char *borrow_hostname(const bt_event *);
process_id_t borrow_process_id(const bt_event *);
thread_id_t borrow_thread_id(const bt_event *);

bt_message* create_power_message(const char* hostname, const process_id_t proprocess_id, const thread_id_t thread_id,
                                 const uintptr_t hDevice, const uint32_t domain, const uint64_t power, const uint64_t ts,
                                 bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream, backend_t backend = BACKEND_UNKNOWN);

bt_message* create_frequency_message(const char* hostname, const process_id_t proprocess_id, const thread_id_t thread_id,
                                     const uintptr_t hDevice, const uint32_t domain, const uint64_t ts, const uint64_t frequency,
                                     bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream, backend_t backend = BACKEND_UNKNOWN);

bt_message* create_computeEU_message(const char* hostname, const process_id_t proprocess_id, const thread_id_t thread_id,
                                     const uintptr_t hDevice, const uint32_t subDevice, const float activeTime, const uint64_t ts,
                                     bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream, backend_t backend = BACKEND_UNKNOWN);

bt_message* create_copyEU_message(const char* hostname, const process_id_t proprocess_id, const thread_id_t thread_id,
                                  const uintptr_t hDevice, const uint32_t subDevice, const float activeTime, const uint64_t ts,
                                  bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream, backend_t backend = BACKEND_UNKNOWN);

bt_message *create_host_message(const char *hostname, const process_id_t, const thread_id_t,
                                const char *name, const uint64_t ts, const uint64_t duration,
                                const bool err, bt_event_class *, bt_self_message_iterator *,
                                bt_stream *, backend_t = BACKEND_UNKNOWN);

bt_message *create_device_message(const char *hostname, const process_id_t, const thread_id_t,
                                  const thapi_device_id, const thapi_device_id, const char *name,
                                  const uint64_t ts, const uint64_t duration, const bool err,
                                  const char *metadata, bt_event_class *,
                                  bt_self_message_iterator *, bt_stream *);

bt_message *create_device_name_message(const char *hostname, const process_id_t process_id,
                                       const thapi_device_id device_id, const char *name,
                                       bt_event_class *event_class,
                                       bt_self_message_iterator *message_iterator,
                                       bt_stream *stream);

bt_message *create_traffic_message(const char *hostname, const process_id_t, const thread_id_t,
                                   const char *name, const uint64_t size, bt_event_class *,
                                   bt_self_message_iterator *, bt_stream *,
                                   backend_t = BACKEND_UNKNOWN);
