#ifndef BTX_CXIINTERVAL_CALLBACKS_HPP
#define BTX_CXIINTERVAL_CALLBACKS_HPP

#include "xprof_utils.hpp"
#include <metababel/metababel.h>

typedef std::tuple<std::string, std::string, std::string> NicKey;

struct data_s {
  /* CXI Sampling */
  std::unordered_map<NicKey, uint64_t> nic_initial; //remember the first value seen for each (hostname, interface_name, counter)
};
typedef struct data_s data_t;
#endif //BTX_CXIINTERVAL_CALLBACKS_HPP
