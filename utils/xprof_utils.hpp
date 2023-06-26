#pragma once

#include <map>
#include <tuple>
#include <string>
#include "babeltrace2/babeltrace.h"

enum backend_e{
  BACKEND_UNKNOWN = 0,
  BACKEND_ZE = 1,
  BACKEND_OPENCL = 2,
  BACKEND_CUDA = 3,
  BACKEND_OMP_TARGET_OPERATIONS = 4,
  BACKEND_OMP = 5,
  BACKEND_HIP = 6,
  BACKEND_MAX = 7,
};

constexpr const char* backend_name[] = {
  "BACKEND_UNKNOWN",
  "BACKEND_ZE",
  "BACKEND_OPENCL",
  "BACKEND_CUDA",
  "BACKEND_OMP_TARGET_OPERATIONS",
  "BACKEND_OMP",
  "BACKEND_HIP",
};

typedef enum        backend_e backend_t;
typedef unsigned    backend_level_t;
typedef std::string thapi_metadata_t;

// Datatypes representing string classes (or messages) common data. 
typedef intptr_t      process_id_t;
typedef uintptr_t     thread_id_t;
typedef std::string   hostname_t;
typedef std::string   thapi_function_name;
typedef uintptr_t     thapi_device_id;

// Represent a device and a sub device
typedef std::tuple<thapi_device_id, thapi_device_id> dsd_t;
typedef std::tuple<hostname_t, process_id_t> hp_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t> hpt_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_function_name> hpt_function_name_t;
typedef std::tuple<thread_id_t, thapi_function_name> t_function_name_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_device_id, thapi_device_id, thapi_function_name> hpt_device_function_name_t;
typedef std::tuple<hostname_t, process_id_t, thapi_device_id> hp_device_t;
typedef std::tuple<hostname_t, process_id_t, thapi_device_id, thapi_device_id> hp_dsd_t;
typedef std::tuple<long,long> sd_t;
typedef std::tuple<thread_id_t, thapi_function_name, long> tfn_ts_t;
typedef std::tuple<thapi_function_name, long> fn_ts_t;

// NOTE: Required to generate the hash of a tuple.
// REFERENCE: https://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
namespace std{
  namespace
  {
    // Code from boost
    // Reciprocal of the golden ratio helps spread entropy
    //     and handles duplicates.
    // See Mike Seymour in magic-numbers-in-boosthash-combine:
    //     https://stackoverflow.com/questions/4948780
    template <class T>
    inline void hash_combine(std::size_t& seed, T const& v)
    {
        seed ^= hash<T>()(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    // Recursive template code derived from Matthieu M.
    template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
    struct HashValueImpl
    {
      static void apply(size_t& seed, Tuple const& tuple)
      {
        HashValueImpl<Tuple, Index-1>::apply(seed, tuple);
        hash_combine(seed, get<Index>(tuple));
      }
    };

    template <class Tuple>
    struct HashValueImpl<Tuple,0>
    {
      static void apply(size_t& seed, Tuple const& tuple)
      {
        hash_combine(seed, get<0>(tuple));
      }
    };
  }

  template <typename ... TT>
  struct hash<std::tuple<TT...>>
  {
    size_t
    operator()(std::tuple<TT...> const& tt) const
    {
        size_t seed = 0;
        HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
        return seed;
    }
  };

  template <typename ... TT>
  struct hash<std::pair<TT...>>
  {
    size_t
    operator()(std::pair<TT...> const& tt) const
    {
        size_t seed = 0;
        HashValueImpl<std::pair<TT...> >::apply(seed, tt);
        return seed;
    }
  };
}

const char* borrow_hostname(const bt_event*);
process_id_t borrow_process_id(const bt_event*);
thread_id_t borrow_thread_id(const bt_event*);

bt_message* create_host_message(const char *hostname, const process_id_t, const thread_id_t,
                                const char *name, const uint64_t ts, const uint64_t duration, const bool err,
                                bt_event_class*, bt_self_message_iterator*, bt_stream*, backend_t = BACKEND_UNKNOWN);

bt_message* create_device_message(const char *hostname, const process_id_t, const thread_id_t, const thapi_device_id, const thapi_device_id,
                                  const char *name, const uint64_t ts, const uint64_t duration, const bool err, const char* metadata,
                                  bt_event_class*, bt_self_message_iterator*, bt_stream*);

bt_message* create_device_name_message(const char* hostname, const process_id_t process_id,
                                       const thapi_device_id device_id, const char* name,
                                       bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream);

bt_message* create_traffic_message(const char *hostname, const process_id_t, const thread_id_t,
                                   const char *name, const uint64_t size,
                                   bt_event_class*, bt_self_message_iterator*, bt_stream*, backend_t = BACKEND_UNKNOWN);
