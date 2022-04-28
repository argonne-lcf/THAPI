#pragma once

#include <map>
#include <tuple>
#include <string>
#include "babeltrace2/babeltrace.h"

enum backend_e{ BACKEND_UNKNOW = 0,
                BACKEND_ZE = 1,
                BACKEND_OPENCL = 2,
                BACKEND_CUDA = 3,
                BACKEND_OMP_INTEL = 4,
                BACKEND_OMP = 5 };

constexpr int backend_level[6] = { 0, 0, 0, 0, 1, 2 };

typedef enum backend_e backend_t;

typedef intptr_t                       process_id_t;
typedef uintptr_t                      thread_id_t;
typedef std::string                    hostname_t;
typedef std::string                    thapi_function_name;
typedef uintptr_t                      thapi_device_id;

// Represent a device and a sub device
typedef std::tuple<thapi_device_id, thapi_device_id> dsd_t;
typedef std::tuple<hostname_t, thapi_device_id> h_device_t;
typedef std::tuple<hostname_t, process_id_t> hp_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t> hpt_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_function_name> hpt_function_name_t;
typedef std::tuple<thread_id_t, thapi_function_name> t_function_name_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_device_id, thapi_device_id> hpt_dsd_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_device_id, thapi_device_id, thapi_function_name> hpt_device_function_name_t;
typedef std::tuple<hostname_t, process_id_t, thapi_device_id> hp_device_t;
typedef std::tuple<hostname_t, process_id_t, thapi_device_id, thapi_device_id> hp_dsd_t;

typedef std::tuple<long,long> sd_t;
typedef std::tuple<thread_id_t, thapi_function_name, long> tfn_ts_t;
typedef std::tuple<thapi_function_name, long> fn_ts_t;
typedef std::tuple<thapi_function_name, thapi_device_id, thapi_device_id, long> fn_dsd_ts_t;
typedef std::tuple<thread_id_t, thapi_function_name, thapi_device_id, thapi_device_id, long> tfn_dsd_ts_t;

typedef std::tuple<thapi_function_name, std::string, thapi_device_id, thapi_device_id, long> fnm_dsd_ts_t;
typedef std::tuple<thread_id_t, thapi_function_name, std::string, thapi_device_id, thapi_device_id, long> tfnm_dsd_ts_t;

// https://stackoverflow.com/questions/7110301/generic-hash-for-tuples-in-unordered-map-unordered-set
// Hash of std tuple
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
}

const char* borrow_hostname(const bt_event*);
process_id_t borrow_process_id(const bt_event*);
thread_id_t borrow_thread_id(const bt_event*);

bt_message* create_host_message(const char *hostname, const process_id_t, const thread_id_t,
                                const char *name, const uint64_t ts, const uint64_t duration, const bool err,
                                bt_event_class*, bt_self_message_iterator*, bt_stream*, backend_t = BACKEND_UNKNOW);

bt_message* create_device_message(const char *hostname, const process_id_t, const thread_id_t, const thapi_device_id, const thapi_device_id,
                                  const char *name, const uint64_t ts, const uint64_t duration, const bool err, const char* metadata,
                                  bt_event_class*, bt_self_message_iterator*, bt_stream*);

bt_message* create_device_name_message(const char* hostname, const process_id_t process_id,
                                       const thapi_device_id device_id, const char* name,
                                       bt_event_class *event_class, bt_self_message_iterator *message_iterator, bt_stream *stream);

bt_message* create_traffic_message(const char *hostname, const process_id_t, const thread_id_t,
                                   const char *name, const uint64_t size,
                                   bt_event_class*, bt_self_message_iterator*, bt_stream*, backend_t = BACKEND_UNKNOW);


//. Getter
//
//
// Explanation of magic number
// 0 == Idx
// 1 == GetterFunction
// 2 == Setting function
template<class T>
auto populate_tuple(const bt_field* payload_field, T t) {
  const bt_field *field = bt_field_structure_borrow_member_field_by_index_const(payload_field, std::get<0>(t));
  //Decltype return a reference
  return static_cast<typename std::remove_reference<decltype(std::get<2>(t))>::type>(std::get<1>(t)(field));
}

template<class ...T, size_t ...I>
auto thapi_bt2_getter(const bt_field* payload_field, std::tuple<T...>& a, std::index_sequence<I...>){
   return std::tuple{populate_tuple(payload_field, std::get<I>(a))...};
}

template<class ...T>
auto thapi_bt2_getter(const bt_field* payload_field, std::tuple<T...>& a){
   constexpr auto seq = std::make_index_sequence<sizeof...(T)>();
   return thapi_bt2_getter(payload_field,a,seq);
}
