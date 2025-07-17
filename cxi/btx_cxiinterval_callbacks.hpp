#ifndef BTX_CXIINTERVAL_CALLBACKS_HPP
#define BTX_CXIINTERVAL_CALLBACKS_HPP

#include "xprof_utils.hpp"
#include <metababel/metababel.h>

struct nic_state_t {
  uint64_t initial;
  uint64_t last_seen;
};

struct data_s {
  /* CXI Sampling */
  std::unordered_map<hic_t, nic_state_t> nic_metric_ref; //remember the first and last values seen for each (hostname, interface_name, counter)
};

typedef struct data_s data_t;
#endif //BTX_CXIINTERVAL_CALLBACKS_HPP
