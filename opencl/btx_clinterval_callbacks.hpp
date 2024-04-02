#pragma once

#include <iostream>

#include "cl.h.include"

#include "xprof_utils.hpp"

using hp_command_queue_t = std::tuple<hostname_t, process_id_t, cl_command_queue>;
using hp_kernel_t = std::tuple<hostname_t, process_id_t, cl_kernel>;
using hp_event_t = std::tuple<hostname_t, process_id_t, cl_event>;

// custom defines for cl backend using std::string instead of const char *.
using hpt_function_name_cl_t = std::tuple<hostname_t, process_id_t, thread_id_t, std::string>;
using tfn_ts_cl_t = std::tuple<thread_id_t, std::string, long>;
using fn_ts_cl_t = std::tuple<std::string, long>;
using hpt_function_name_cl_t = std::tuple<hostname_t, process_id_t, thread_id_t, thapi_function_name>;
