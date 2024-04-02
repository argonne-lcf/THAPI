#pragma once

#include <iostream>

#include "cl.h.include"

#include "xprof_utils.hpp"

using hp_command_queue_t = std::tuple<hostname_t, process_id_t, cl_command_queue>;
using hp_kernel_t = std::tuple<hostname_t, process_id_t, cl_kernel>;
using hp_event_t = std::tuple<hostname_t, process_id_t, cl_event>;
