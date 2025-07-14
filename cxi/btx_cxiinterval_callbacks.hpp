#ifndef BTX_CXIINTERVAL_CALLBACKS_HPP
#define BTX_CXIINTERVAL_CALLBACKS_HPP

#include "xprof_utils.hpp"
#include <metababel/metababel.h>

// NIC key struct
struct NicKey {
  std::string hostname;
  std::string interface;
  std::string counter;

  bool operator==(NicKey const &o) const noexcept {
    return hostname==o.hostname
        && interface==o.interface
        && counter==o.counter;
  }
};

// NIC key hasher
namespace std {
  template<> struct hash<NicKey> {
    size_t operator()(NicKey const &k) const noexcept {
      size_t h = hash<string>()(k.hostname);
      // combine with interface
      h ^= hash<string>()(k.interface) + 0x9e3779b97f4a7c15ULL + (h<<6) + (h>>2);
      // combine with counter
      h ^= hash<string>()(k.counter)   + 0x9e3779b97f4a7c15ULL + (h<<6) + (h>>2);
      return h;
    }
  };
}

struct data_s {
  /* CXI Sampling */
  std::unordered_map<NicKey, uint64_t> nic_initial; //remember the first value seen for each (hostname, ifname, counter)
};
typedef struct data_s data_t;
#endif //BTX_CXIINTERVAL_CALLBACKS_HPP
