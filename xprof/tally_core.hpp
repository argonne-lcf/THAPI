#pragma once

#include <vector>
#include <limits>

class TallyCoreBase {
public:
  TallyCoreBase() {}

  TallyCoreBase(uint64_t _dur, uint64_t _err) : duration{_dur}, error{_err} {
    count = 1;
    if (!error) {
      min = duration;
      max = duration;
    } else
      duration = 0;
  }

  uint64_t duration{0};
  uint64_t error{0};
  uint64_t min{std::numeric_limits<uint64_t>::max()};
  uint64_t max{0};
  uint64_t count{0};
  double duration_ratio{1.};
  double average{0};


  TallyCoreBase &operator+=(const TallyCoreBase &rhs) {
    this->duration += rhs.duration;
    this->min = std::min(this->min, rhs.min);
    this->max = std::max(this->max, rhs.max);
    this->count += rhs.count;
    this->error += rhs.error;
    return *this;
  }
  
  bool operator>(const TallyCoreBase &rhs) { return duration > rhs.duration; }

  void finalize(const TallyCoreBase &rhs) {
    average = (count && count != error) ? static_cast<double>(duration) / (count - error) : 0.;
    duration_ratio = static_cast<double>(duration) / rhs.duration;
  }
};
