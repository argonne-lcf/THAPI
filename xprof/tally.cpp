#include "tally.hpp"
#include <string>
#include <array>
#include <cassert>
#include <sstream>      // std::stringstream, std::stringbuf

//! User data collection structure.
//! It is used to collect interval messages data, once data is collected,
//! it is aggregated and tabulated for printing.
struct tally_dispatch_s {
  //! User params provided to the user component.
  btx_params_t *params;

  std::array<int, BACKEND_MAX>backend_level;

  std::map<backend_level_t, std::set<const char *>> host_backend_name;
  std::map<backend_level_t, std::set<const char *>> traffic_backend_name;

  std::map<backend_level_t, std::unordered_map<hpt_function_name_t, TallyCoreTime>> host;
  std::map<backend_level_t, std::unordered_map<hpt_function_name_t, TallyCoreByte>> traffic;

  std::unordered_map<hpt_device_function_name_t, TallyCoreTime> device;
  std::unordered_map<hp_device_t, std::string> device_name;

  //! Collects thapi metadata "lttng_ust_thapi:metadata".
  std::vector<std::string> metadata;
};

typedef struct tally_dispatch_s tally_dispatch_t;

void print_metadata(std::vector<std::string> metadata) {
  if (metadata.empty())
    return;

  std::cout << "Metadata" << std::endl;
  std::cout << std::endl;
  for (std::string value : metadata)
    std::cout << value << std::endl;
}

int get_backend_id(std::string name) {
  for(int i = 0; i < BACKEND_MAX; ++i)
    // backend_name is located in xprof_utils.cpp
    if (std::string{backend_name[i]} == name) return i;
  return -1;
}

void btx_initialize_usr_data(void *btx_handle, void **usr_data) {
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

void btx_read_params(void *btx_handle, void *usr_data, btx_params_t *usr_params) {
  tally_dispatch_t *data = (tally_dispatch_t *)usr_data;
  data->params = usr_params;

  // Consumes key:value pairs in the stringstream k1:v1,..,kn:vn
  std::stringstream tokens{data->params->backend_level};
  std::string tmp;
  while (std::getline(tokens, tmp, ',')) {
    std::stringstream tmp_string{tmp};
    std::string k,v;
    std::getline(tmp_string, k, ':');
    int id = get_backend_id(k);
    assert((id > 0) && "Backend not found. Please check --backend-level format.");
    std::getline(tmp_string, v);
    data->backend_level[id] = std::stoi(v);
  }
}

void btx_finalize_usr_data(void *btx_handle, void *usr_data) {
  /* User cast the API usr_data that was already initialized with his/her data */
  tally_dispatch_t *data = (tally_dispatch_t *)usr_data;

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

static void tally_host_callback(void *btx_handle, void *usr_data, const char *hostname,
                                int64_t vpid, uint64_t vtid,  const char *name,
				uint64_t min, uint64_t max, uint64_t total, uint64_t count,
				uint64_t backend, uint64_t err) {

  tally_dispatch_t *data = (tally_dispatch_t *)usr_data;

  const int level = data->backend_level[backend];
  data->host_backend_name[level].insert(backend_name[backend]);
  data->host[level][ {hostname, vpid, vtid, name } ] += {total, err, count, min, max};
}

static void tally_device_callback(void *btx_handle, void *usr_data, const char *hostname,
                                int64_t vpid, uint64_t vtid,  const char *name,
                                uint64_t min, uint64_t max, uint64_t total, uint64_t count,
				uint64_t did, uint64_t sdid, const char * metadata ) {

  tally_dispatch_t *data = (tally_dispatch_t *)usr_data;

  const auto name_demangled = (strcmp(data->params->name, "demangle") == 0) ? f_demangle_name(name) : name;
  const auto name_with_metadata = (data->params->display_kernel_verbose && !strcmp(metadata, ""))
                                      ? name_demangled + "[" + metadata + "]"
                                      : name_demangled;

  data->device[ {hostname, vpid, vtid, did, sdid, name_with_metadata} ] += {total, 0, count, min, max};
}

static void tally_traffic_callback(void *btx_handle, void *usr_data, const char *hostname,
                                int64_t vpid, uint64_t vtid,  const char *name,
                                uint64_t min, uint64_t max, uint64_t total, uint64_t count,
                                uint64_t backend) {

  tally_dispatch_t *data = (tally_dispatch_t *)usr_data;

  const int level = data->backend_level[backend];
  data->traffic_backend_name[level].insert(backend_name[backend]);
  data->traffic[level][ {hostname, vpid, vtid, name} ] +=  {total, 0, count, min, max};;
}

static void device_name_usr_callback(void *btx_handle, void *usr_data, const char *hostname,
                                           int64_t vpid, uint64_t vtid, int64_t ts, int64_t backend,
                                           const char *name, uint64_t did) {
  tally_dispatch_t *data = (tally_dispatch_t *)usr_data;
  data->device_name[hp_device_t(hostname, vpid, did)] = name;
}

static void ust_thapi_metadata_usr_callback(void *btx_handle, void *usr_data,
                                              const char *hostname, int64_t vpid, uint64_t vtid,
                                              int64_t ts, int64_t backend, const char *metadata) {
  tally_dispatch_t *data = (tally_dispatch_t *)usr_data;

  data->metadata.push_back(metadata);
}


void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_usr_data(btx_handle, &btx_initialize_usr_data);
  btx_register_callbacks_read_params(btx_handle, &btx_read_params);
  btx_register_callbacks_finalize_usr_data(btx_handle, &btx_finalize_usr_data);

  btx_register_callbacks_lttng_device_name(btx_handle, &device_name_usr_callback);
  btx_register_callbacks_lttng_ust_thapi_metadata(btx_handle, &ust_thapi_metadata_usr_callback);

  btx_register_callbacks_tally_host(btx_handle, &tally_host_callback);
  btx_register_callbacks_tally_device(btx_handle, &tally_device_callback);
  btx_register_callbacks_tally_traffic(btx_handle, &tally_traffic_callback);

}
