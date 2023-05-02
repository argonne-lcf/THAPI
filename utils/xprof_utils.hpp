#pragma once

#include <map>
#include <tuple>
#include <string>
#include "babeltrace2/babeltrace.h"

//! Backends identifiers enum.

//! Every backend reaching an lttng tracepoint, that subsequently 
//! generates a message, is identified by a backend_id. This 
//! identifies the backend that generated a given message.
enum backend_e{ 
  BACKEND_UNKNOWN = 0,
  BACKEND_ZE = 1,
  BACKEND_OPENCL = 2,
  BACKEND_CUDA = 3,
  BACKEND_OMP_TARGET_OPERATIONS = 4,
  BACKEND_OMP = 5 
};

//! Backends identifiers enum.

//! Every backend reaching an lttng tracepoint, that subsequently 
//! generates a message, is identified by a backend_id. This 
//! identifies the backend that generated a given message.
constexpr const char* backend_name[] = { 
  "BACKEND_UNKNOWN",
  "BACKEND_ZE",
  "BACKEND_OPENCL",
  "BACKEND_CUDA",
  "BACKEND_OMP_TARGET_OPERATIONS",
  "BACKEND_OMP" 
};

//! Backends levels.

//! "backend_level" maps a "backend_id" with its level of abstraction.
//! For instance, OpenMP in level 0 run on top of LO/OpenCL/CUDA.
//! This abstraction hierachy is represented by "backend_level".
constexpr int backend_level[] = { 
  2, // BACKEND_UNKNOWN
  2, // BACKEND_ZE
  2, // BACKEND_OPENCL
  2, // BACKEND_CUDA
  1, // BACKEND_OMP_TARGET_OPERATIONS
  0  // BACKEND_OMP
};

// TODO: Referenced in THAPI (utils/xprof_utils.hpp), but not used in metababel.
typedef enum backend_e backend_t;

// Datatypes represeting string classes (or messages) common data. 

typedef intptr_t      process_id_t;
typedef uintptr_t     thread_id_t;
typedef std::string   hostname_t;
typedef std::string   thapi_function_name;
typedef uintptr_t     thapi_device_id;


// Represent a device and a sub device

// Referenced in THAPI (opencl/clinterval_callbacks.hpp,utils/xprof_utils.hpp,opencl/clinterval_callbacks.cpp.erb), but not used in metababel, not sure if needed.
typedef std::tuple<thapi_device_id, thapi_device_id> dsd_t;

// TODO: Not referenced in any other place of THAPI (babeltrace) neither metababel.
// typedef std::tuple<hostname_t, thapi_device_id> h_device_t;

// TODO: Referenced in THAPI (ze/zeinterval_callbacks.hpp,ze/zeinterval_callbacks.cpp.erb,utils/xprof_utils.hpp) but not used in metababel, not sure if needed.
typedef std::tuple<hostname_t, process_id_t> hp_t;

// TDOD: Referenced in THAPI (cuda/cudainterval_callbacks.cpp.erb,ze/zeinterval_callbacks.hpp,opencl/clinterval_callbacks.hpp,ze/zeinterval_callbacks.cpp.erb,omp/ompinterval_callbacks.hpp,utils/xprof_utils.hpp)
//! Identifies an entity in a parallel/distributed program that generates a message. 
typedef std::tuple<hostname_t, process_id_t, thread_id_t> hpt_t;

//! Identifies an specific host, process, thread using the api call "thapi_function_name" in a parallel/distributed program
typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_function_name> hpt_function_name_t;

// TODO: Not referenced in any other place of THAPI (babeltrace) neither metababel.
typedef std::tuple<thread_id_t, thapi_function_name> t_function_name_t;

// TODO: Not referenced in any other place of THAPI (babeltrace) neither metababel.
// typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_device_id, thapi_device_id> hpt_dsd_t;

//! Identifies an specific host, process, thread, device, sudevice, using the api call "thapi_function_name" in a parallel/distributed program.
typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_device_id, thapi_device_id, thapi_function_name> hpt_device_function_name_t;

// TDOD: Referenced in THAPI (opencl/clinterval_callbacks.hpp, opencl/clinterval_callbacks.cpp.erb,ze/zeinterval_callbacks.hpp,xprof/tally.hpp,cuda/cudainterval_callbacks.hpp,cuda/cudainterval_callbacks.cpp.erb,utils/xprof_utils.hpp,xprof/tally.cpp,ze/zeinterval_callbacks.cpp.erb), but not used in metababel, not sure if needed.
//! Identifies an specific host, process using a device  in a parallel/distributed program.
typedef std::tuple<hostname_t, process_id_t, thapi_device_id> hp_device_t;

// TODO: Referenced in THAPI (xprof/timeline.hpp,utils/xprof_utils.hpp) but not used in metababel, not sure if needed.
//! Identifies an specific host, process, device an subdevice in a parallel/distributed program.
typedef std::tuple<hostname_t, process_id_t, thapi_device_id, thapi_device_id> hp_dsd_t;

// TODO: Referenced in THAPI (opencl/clinterval_callbacks.hpp,utils/xprof_utils.hpp,opencl/clinterval_callbacks.cpp.erb) but not used in metababel, not sure if needed.
//! Identifies the start time and duration (delta) of an API call in an applications. Used in the creation of intervals in the filter component .  
typedef std::tuple<long,long> sd_t;

// TDOD: 
typedef std::tuple<thread_id_t, thapi_function_name, long> tfn_ts_t;

// Referenced in THAPI (opencl/clinterval_callbacks.hpp, cuda/cudainterval_callbacks.hpp, utils/xprof_utils.hpp, cuda/cudainterval_callbacks.cpp.erb, opencl/clinterval_callbacks.cpp.erb)  but not used in metababel, not sure if needed.
typedef std::tuple<thapi_function_name, long> fn_ts_t;

// TODO: Not referenced in any other place of THAPI (babeltrace) neither metababel.
// typedef std::tuple<thapi_function_name, thapi_device_id, thapi_device_id, long> fn_dsd_ts_t;

// TODO: Not referenced in any other place of THAPI (babeltrace) neither metababel.
// typedef std::tuple<thread_id_t, thapi_function_name, thapi_device_id, thapi_device_id, long> tfn_dsd_ts_t;

// TODO: Not referenced in any other place of THAPI (babeltrace) neither metababel.
// typedef std::tuple<thapi_function_name, std::string, thapi_device_id, thapi_device_id, long> fnm_dsd_ts_t;

// TODO: Not referenced in any other place of THAPI (babeltrace) neither metababel.
// typedef std::tuple<thread_id_t, thapi_function_name, std::string, thapi_device_id, thapi_device_id, long> tfnm_dsd_ts_t;

// NOTE: Required to generate a hash of a tuple, otherwhise, the operaton "data->host[level][entity_id] += interval;"
// may fail since host[level] returns an unordered_map and this data structure does not know to hash a tuple.
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


//. Getter
//
//
// Explanation of magic number
// 0 == Idx
// 1 == GetterFunction
// 2 == Setting function
// template<class T>
// auto populate_tuple(const bt_field* payload_field, T t) {
//   const bt_field *field = bt_field_structure_borrow_member_field_by_index_const(payload_field, std::get<0>(t));
//   //Decltype return a reference
//   return static_cast<typename std::remove_reference<decltype(std::get<2>(t))>::type>(std::get<1>(t)(field));
// }

// template<class ...T, size_t ...I>
// auto thapi_bt2_getter(const bt_field* payload_field, std::tuple<T...>& a, std::index_sequence<I...>){
//    return std::tuple{populate_tuple(payload_field, std::get<I>(a))...};
// }

// template<class ...T>
// auto thapi_bt2_getter(const bt_field* payload_field, std::tuple<T...>& a){
//    constexpr auto seq = std::make_index_sequence<sizeof...(T)>();
//    return thapi_bt2_getter(payload_field,a,seq);
// }
