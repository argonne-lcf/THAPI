#pragma once

#include <unordered_map>
#include <string>
#include <assert.h>
#include <regex>

#include "xprof_utils.hpp"

typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_function_name> hpt_fn_t;

struct data_s {
  std::unordered_map<hpt_fn_t, int64_t> dispatch;
};

typedef struct data_s data_t;

std::string strip_event_class_name(const char *str) {
  std::string temp(str);
  std::smatch match;
  std::regex_search(temp, match, std::regex(":(.*?)_?(?:entry|exit)?$"));

  // The entire match is hold in the first item, sub_expressions 
  // (parentheses delimited groups) are stored after.
  assert( match.size() > 1 && "Event_class_name not matching regex.");

  return match[1].str();
}
