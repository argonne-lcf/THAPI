#pragma once

#include <tuple>
#include <string>
#include "babeltrace2/babeltrace.h"

typedef intptr_t    process_id_t;
typedef uintptr_t   thread_id_t;
typedef std::string hostname_t;
typedef std::string thapi_function_name;
typedef uintptr_t   thapi_device_id;


// Represent a device and a sub device
typedef std::tuple<thapi_device_id, thapi_device_id> dsd_t;
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

/*
Utils function
*/
const hostname_t borrow_hostname(const bt_event *event){
    const bt_stream *stream = bt_event_borrow_stream_const(event);
    const bt_trace *trace = bt_stream_borrow_trace_const(stream);
    const bt_value *host_name_str = bt_trace_borrow_environment_entry_value_by_name_const(trace, "hostname");
    return  bt_value_string_get(host_name_str);
}

process_id_t borrow_process_id(const bt_event *event){
    const bt_field *common_context_field = bt_event_borrow_common_context_field_const(event);
    const bt_field *field = bt_field_structure_borrow_member_field_by_index_const(common_context_field, 0);
    return bt_field_integer_signed_get_value(field);
}

thread_id_t borrow_thread_id(const bt_event *event){
    const bt_field *common_context_field = bt_event_borrow_common_context_field_const(event);
    const bt_field *field = bt_field_structure_borrow_member_field_by_index_const(common_context_field, 1);
    return bt_field_integer_unsigned_get_value(field);
}
