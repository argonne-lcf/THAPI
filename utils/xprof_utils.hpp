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
