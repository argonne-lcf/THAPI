#pragma once

#include <cstdint>
#include <vector>
#include <limits>
#include <cassert>

class TallyCoreBase {
public:
  TallyCoreBase() {}

  TallyCoreBase(uint64_t _dur, bool _err) : error{ (uint64_t) _err}, count{1} {
    if (!_err) {
      duration = _dur;
      min = _dur;
      max = _dur;
    } 
  }

  TallyCoreBase(uint64_t _dur, uint64_t _err, uint64_t _count, uint64_t _min, uint64_t _max) :
	  duration{_dur}, error{_err}, count{_count}, min{_min}, max{_max} {}

  uint64_t duration{0};
  uint64_t error{0};
  uint64_t count{0};
  uint64_t min{std::numeric_limits<uint64_t>::max()};
  uint64_t max{0};
  double duration_ratio{std::numeric_limits<double>::quiet_NaN() };

  TallyCoreBase &operator+=(const TallyCoreBase &rhs) {
    this->duration += rhs.duration;
    this->min = std::min(this->min, rhs.min);
    this->max = std::max(this->max, rhs.max);
    this->count += rhs.count;
    this->error += rhs.error;
    return *this;
  }

  bool operator>(const TallyCoreBase &rhs) { return duration > rhs.duration; }

  double average() {
    return (count && count != error) ? static_cast<double>(duration) / (count - error) : 0.;
  }

  void compute_duration_ratio(const TallyCoreBase &rhs) {
    if (rhs.duration > 0)
      duration_ratio = static_cast<double>(duration) / rhs.duration;
  }
};
