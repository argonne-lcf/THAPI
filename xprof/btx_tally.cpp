#include <metababel/metababel.h>

#include "btx_tally.hpp"
#include "my_demangle.h"
#include "xprof_utils.hpp"
#include <array>
#include <sstream> // std::stringstream, std::stringbuf
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class TallyCoreTime : public TallyCoreString {
  using TallyCoreString::TallyCoreString;
public:
  static constexpr std::array headers{"Time", "Time(%)", "Calls", "Average", "Min", "Max", "Error"};
  virtual const std::array<std::string, nfields> to_string() override {
    return std::array{
        (count == error) ? "" : this->format_time(duration),
        (count == error) ? "" : this->to_pretty_string(100. * duration_ratio, "%"),
        this->to_pretty_string(count, "", 0),
        (count == error) ? "" : this->format_time(average()),
        (count == error) ? "" : this->format_time(min),
        (count == error) ? "" : this->format_time(max),
        this->to_pretty_string(error, "", 0)};
  }

private:
  //! Returns duration as a formatted string with units.
  template <typename T> std::string format_time(const T duration) {
    const double h = duration / 3.6e+12;
    if (h >= 1.)
      return this->to_pretty_string(h, "h");

    const double min = duration / 6e+10;
    if (min >= 1.)
      return this->to_pretty_string(min, "min");

    const double s = duration / 1e+9;
    if (s >= 1.)
      return this->to_pretty_string(s, "s");

    const double ms = duration / 1e+6;
    if (ms >= 1.)
      return this->to_pretty_string(ms, "ms");

    const double us = duration / 1e+3;
    if (us >= 1.)
      return this->to_pretty_string(us, "us");

    return this->to_pretty_string(duration, "ns");
  }
};

class TallyCoreByte : public TallyCoreString {
  using TallyCoreString::TallyCoreString;
public:
  static constexpr std::array headers{"Byte", "Byte(%)", "Calls", "Average", "Min", "Max", "Error"};
  virtual const std::array<std::string, nfields> to_string() override {
    return std::array {format_byte(duration),
                                    this->to_pretty_string(100. * duration_ratio, "%"),
                                    this->to_pretty_string(count, "", 0),
                                    this->format_byte(average()),
                                    this->format_byte(min),
                                    this->format_byte(max),
                                    this->to_pretty_string(error, "", 0)};
  }

private:
  //! Returns a data transfer size (duration) as a formatted string with units.
  template <typename T> std::string format_byte(const T duration) {
    const double PB = duration / 1e+15;
    if (PB >= 1.)
      return this->to_pretty_string(PB, "PB");

    const double TB = duration / 1e+12;
    if (TB >= 1.)
      return this->to_pretty_string(TB, "TB");

    const double GB = duration / 1e+9;
    if (GB >= 1.)
      return this->to_pretty_string(GB, "GB");

    const double MB = duration / 1e+6;
    if (MB >= 1.)
      return this->to_pretty_string(MB, "MB");

    const double kB = duration / 1e+3;
    if (kB >= 1.)
      return this->to_pretty_string(kB, "kB");

    return this->to_pretty_string(duration, "B");
  }
};


//! User data collection structure.
//! It is used to collect interval messages data, once data is collected,
//! it is aggregated and tabulated for printing.
struct tally_dispatch_s {
  //! User params provided to the user component.
  btx_params_t *params;

  std::array<int, BACKEND_MAX> backend_level;

  std::map<backend_level_t, std::set<std::string>> host_backend_name;
  std::map<backend_level_t, std::set<std::string>> traffic_backend_name;

  std::map<backend_level_t, std::unordered_map<hpt_function_name_t, TallyCoreTime>> host;
  std::map<backend_level_t, std::unordered_map<hpt_function_name_t, TallyCoreByte>> traffic;

  std::unordered_map<hpt_device_function_name_t, TallyCoreTime> device;
  std::unordered_map<hp_device_t, std::string> device_name;

  std::vector<std::string> metadata;
};
using tally_dispatch_t = struct tally_dispatch_s;

static std::string join_iterator(const std::set<std::string> &x, std::string delimiter = ",") {
  return std::accumulate(std::begin(x), std::end(x), std::string{},
                         [&delimiter](const std::string &a, const std::string &b) {
                           return a.empty() ? b : a + delimiter + b;
                         });
}

static int get_backend_id(std::string name) {
  for (int i = 0; i < BACKEND_MAX; ++i)
    // backend_name is located in xprof_utils.hpp
    if (std::string{pretty_backend_name[i]} == name)
      return i;
  return -1;
}

static thapi_function_name f_demangle_name(thapi_function_name mangle_name) {
  std::string result = mangle_name;
  std::string line_num;

  // C++ don't handle PCRE, hence and lazy/non-greedy and $.
  const static std::regex base_regex("__omp_offloading_[^_]+_[^_]+_(.*?)_([^_]+)$");
  std::smatch base_match;
  if (std::regex_match(mangle_name, base_match, base_regex) && base_match.size() == 3) {
    result = base_match[1].str();
    line_num = base_match[2].str();
  }

  const char *demangle = my_demangle(result.c_str());
  if (demangle) {
    thapi_function_name s{demangle};
    if (!line_num.empty())
      s += "_" + line_num;

    /* We name the kernels after the type that gets passed in the first
       template parameter to the sycl_kernel function in order to prevent
       it from conflicting with any actual function name.
       The result is the demangling will always be something like, “typeinfo
       for...”.
    */
    const std::string prefix{"typeinfo name for "};
    if (s.rfind(prefix) == 0)
      return s.substr(prefix.size(), s.size());
    return s;
  }
  return mangle_name;
}

static void print_metadata(std::vector<std::string> metadata) {
  if (metadata.empty())
    return;

  std::cout << "Metadata" << std::endl;
  std::cout << std::endl;
  for (std::string value : metadata)
    std::cout << value << std::endl;
}

static void initialize_component_callback(void **usr_data) {
  /* User allocates its own data structure */
  auto *data = new tally_dispatch_t;
  *usr_data = data;

  /* Backend information must match enum backend_e in xprof_utils.hpp */
  data->backend_level = {
      2, // BACKEND_UNKNOWN
      2, // BACKEND_ZE
      2, // BACKEND_OPENCL
      2, // BACKEND_CUDA
      1, // BACKEND_OMP_TARGET_OPERATIONS
      0, // BACKEND_OMP
      2, // BACKEND_HIP
  };
}

static void read_params_callback(void *usr_data, btx_params_t *usr_params) {

  auto *data = static_cast<tally_dispatch_t *>(usr_data);
  data->params = usr_params;

  // Consumes key:value pairs in the stringstream k1:v1,..,kn:vn
  std::stringstream tokens{data->params->backend_level};
  std::string tmp;
  while (std::getline(tokens, tmp, ',')) {
    std::stringstream tmp_string{tmp};
    std::string k, v;
    std::getline(tmp_string, k, ':');
    int id = get_backend_id(k);
    assert((id > 0) && "Backend not found. Please check --backend-level format.");
    std::getline(tmp_string, v);
    data->backend_level[id] = std::stoi(v);
  }
}

static void finalize_component_callback(void *usr_data) {

  auto *data = static_cast<tally_dispatch_t *>(usr_data);

  const int max_name_size = data->params->display_name_max_size;

  if (strcmp(data->params->display_mode, "human") == 0) {
    if (data->params->display_metadata)
      print_metadata(data->metadata);

    if (strcmp(data->params->display, "compact") == 0) {

      for (const auto &[level, host] : data->host) {
        std::string s = join_iterator(data->host_backend_name[level]);
        print_compact(s, host, std::make_tuple("Hostnames", "Processes", "Threads"), max_name_size);
      }
      print_compact("Device profiling", data->device,
                    std::make_tuple("Hostnames", "Processes", "Threads", "Devices", "Subdevices"),
                    max_name_size);

      for (const auto &[level, traffic] : data->traffic) {
        std::string s = join_iterator(data->traffic_backend_name[level]);
        print_compact("Explicit memory traffic (" + s + ")", traffic,
                      std::make_tuple("Hostnames", "Processes", "Threads"), max_name_size);
      }
    } else {
      for (const auto &[level, host] : data->host) {
        std::string s = join_iterator(data->host_backend_name[level]);
        print_extended(s, host, std::make_tuple("Hostname", "Process", "Thread"), max_name_size);
      }
      print_extended(
          "Device profiling", data->device,
          std::make_tuple("Hostname", "Process", "Thread", "Device pointer", "Subdevice pointer"),
          max_name_size);

      for (const auto &[level, traffic] : data->traffic) {
        std::string s = join_iterator(data->traffic_backend_name[level]);
        print_extended("Explicit memory traffic (" + s + ")", traffic,
                       std::make_tuple("Hostname", "Process", "Thread"), max_name_size);
      }
    }
  } else {

    nlohmann::json j;
    j["units"] = {{"time", "ns"}, {"size", "bytes"}};

    if (data->params->display_metadata)
      j["metadata"] = data->metadata;

    if (strcmp(data->params->display, "compact") == 0) {
      for (auto &[level, host] : data->host)
        j["host"][level] = json_compact(host);

      if (!data->device.empty())
        j["device"] = json_compact(data->device);

      for (auto &[level, traffic] : data->traffic)
        j["traffic"][level] = json_compact(traffic);

    } else {
      for (auto &[level, host] : data->host)
        j["host"][level] = json_extented(host, std::make_tuple("Hostname", "Process", "Thread"));

      if (!data->device.empty())
        j["device"] =
            json_extented(data->device, std::make_tuple("Hostname", "Process", "Thread",
                                                        "Device pointer", "Subdevice pointer"));

      for (auto &[level, traffic] : data->traffic)
        j["traffic"][level] =
            json_extented(traffic, std::make_tuple("Hostname", "Process", "Thread"));
    }
    std::cout << j << std::endl;
  }

  /* Delete user data */
  delete data;
}

static void aggreg_host_callback(void *btx_handle, void *usr_data, const char *hostname,
                                 int64_t vpid, uint64_t vtid, const char *name, uint64_t min,
                                 uint64_t max, uint64_t total, uint64_t count, uint64_t backend,
                                 uint64_t err) {

  auto *data = static_cast<tally_dispatch_t *>(usr_data);

  const int level = data->backend_level[backend];
  data->host_backend_name[level].insert(backend_name[backend]);
  data->host[level][{hostname, vpid, vtid, name}] += {total, err, count, min, max};
}

static void aggreg_device_callback(void *btx_handle, void *usr_data, const char *hostname,
                                   int64_t vpid, uint64_t vtid, const char *name, uint64_t min,
                                   uint64_t max, uint64_t total, uint64_t count, uint64_t did,
                                   uint64_t sdid, const char *metadata) {

  auto *data = static_cast<tally_dispatch_t *>(usr_data);
  const auto name_demangled =
      (strcmp(data->params->name, "demangle") == 0) ? f_demangle_name(name) : name;
  const auto name_with_metadata = (data->params->display_kernel_verbose && strlen(metadata) != 0)
                                      ? name_demangled + "[" + metadata + "]"
                                      : name_demangled;
  data->device[{hostname, vpid, vtid, did, sdid, name_with_metadata}] +=
      {total, 0, count, min, max};
}

static void aggreg_traffic_callback(void *btx_handle, void *usr_data, const char *hostname,
                                    int64_t vpid, uint64_t vtid, const char *name, uint64_t min,
                                    uint64_t max, uint64_t total, uint64_t count,
                                    uint64_t backend) {

  auto *data = static_cast<tally_dispatch_t *>(usr_data);
  const int level = data->backend_level[backend];
  data->traffic_backend_name[level].insert(backend_name[backend]);
  data->traffic[level][{hostname, vpid, vtid, name}] += {total, 0, count, min, max};
  ;
}

static void device_name_usr_callback(void *btx_handle, void *usr_data, const char *hostname,
                                     int64_t vpid, uint64_t vtid, int64_t ts, int64_t backend,
                                     const char *name, uint64_t did) {

  auto *data = static_cast<tally_dispatch_t *>(usr_data);
  data->device_name[hp_device_t(hostname, vpid, did)] = name;
}

static void ust_thapi_metadata_usr_callback(void *btx_handle, void *usr_data, const char *hostname,
                                            int64_t vpid, uint64_t vtid, int64_t ts,
                                            int64_t backend, const char *metadata) {

  auto *data = static_cast<tally_dispatch_t *>(usr_data);
  data->metadata.push_back(metadata);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &initialize_component_callback);
  btx_register_callbacks_read_params(btx_handle, &read_params_callback);
  btx_register_callbacks_finalize_component(btx_handle, &finalize_component_callback);

  btx_register_callbacks_lttng_device_name(btx_handle, &device_name_usr_callback);
  btx_register_callbacks_lttng_ust_thapi_metadata(btx_handle, &ust_thapi_metadata_usr_callback);

  btx_register_callbacks_aggreg_host(btx_handle, &aggreg_host_callback);
  btx_register_callbacks_aggreg_device(btx_handle, &aggreg_device_callback);
  btx_register_callbacks_aggreg_traffic(btx_handle, &aggreg_traffic_callback);
}
