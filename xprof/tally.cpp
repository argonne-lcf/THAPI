#include "tally.hpp"


//! User data collection structure.
//! It is used to collect interval messages data, once data is collected. 
//! It is aggregated and tabulated for printing.
//! This structure can hold data that the user needs to be present among different 
//! callbacks calls, for instance commad line params that can modify the printing 
//! behaviour.
struct tally_dispatch {
    //! User params provided to the user component.
    btx_params_t *params;

    //! Maps "level" with the names of the backends that appeared when processing host messages (lttng:host). 
    //! This information is separed by level. Refer to the "backend_level" array at the top of this 
    //! file to see which backends may appear on each level. 
    std::map<unsigned,std::set<const char*>> host_backend_name;

    //! Maps "level" with the duration data collected from host messages (lttng:host) for every (host,pid,tid,api_call_name) entity.
    //! EXAMPLE: map{ 0 => umap{ tuple("iris",1287,2780,"ompt_target") => TallyCoreTime } }
    std::map<unsigned,std::unordered_map<hpt_function_name_t, TallyCoreTime>> host;

    //! Maps "level" with the duration data collected from device messages (lttng:device) for every (host,pid,tid,api_call_name) entity.
    //! EXAMPLE: map{ 2 => umap{ tuple("iris",1287,2780,"zeMemoryCopy") => TallyCoreTime } }
    std::unordered_map<hpt_device_function_name_t, TallyCoreTime> device;

    //! Maps "level" with the names of the backends appearing when processing traffic messages (lttng:traffic). 
    //! This information is separed by level. Refer to the "backend_level" array at the top of this 
    //! file to see which backends may appear on each level. 
    std::map<unsigned,std::set<const char*>> traffic_backend_name;

    //! Maps "level" with the duration data collected from traffic messages (lttng:traffic) for every (host,pid,tid,api_call_name) entity.
    //! EXAMPLE: map{ 0 => umap{ tuple("iris",1287,2780,"ompt_target") => TallyCoreTime } }
    std::map<unsigned,std::unordered_map<hpt_function_name_t, TallyCoreByte>> traffic;

    //! Maps a "(host,pid,tid,device_id)" with the device name.
    //! The device name is collected when processing data of "device_name" messaages (lttng:device_name). 
    //! This assume that a process is attached to a device (with a given name), once the program execution starts, 
    //! and this will not change during the execution of the program.
    std::unordered_map<hp_device_t, std::string> device_name;

    //! Collects thapi metadata appearing when processing "lttng_ust_thapi:metadata" messages.
    std::vector<std::string> metadata;
};

void print_metadata(std::vector<std::string> metadata) {
  if (metadata.empty())
    return;

  std::cout << "Metadata" << std::endl;
  std::cout << std::endl;
  for (std::string value : metadata)
    std::cout << value << std::endl;
}

void btx_initialize_usr_data(void *btx_handle, void **usr_data) {
    /* User allocates its own data structure */
    struct tally_dispatch *data = new struct tally_dispatch;
    /* User makes our API usr_data to point to his/her data structure */
    *usr_data = data;
}

void btx_read_params(void *btx_handle, void *usr_data, btx_params_t *usr_params) {
    struct tally_dispatch *data = (struct tally_dispatch *) usr_data;
    data->params = usr_params;
}

void btx_finalize_usr_data(void *btx_handle, void *usr_data) {
    /* User cast the API usr_data that was already initialized with his/her data */
    struct tally_dispatch *data = (struct tally_dispatch *) usr_data;
    
    /* User do some stuff with the saved data */

    const int max_name_size = data->params->display_name_max_size;

    if (data->params->display_human) {
        if (data->params->display_metadata)
            print_metadata(data->metadata);

        if (data->params->display_compact) {

            for (const auto& [level,host]: data->host) {
                std::string s = join_iterator(data->host_backend_name[level]);
                print_compact(s, host,
                            std::make_tuple("Hostnames", "Processes", "Threads"),
                            max_name_size);
            }
            print_compact("Device profiling", data->device,
                            std::make_tuple("Hostnames", "Processes", "Threads",
                                            "Devices", "Subdevices"),
                            max_name_size);

            for (const auto& [level,traffic]: data->traffic) {
                std::string s = join_iterator(data->traffic_backend_name[level]);
                print_compact("Explicit memory traffic (" + s + ")", traffic,
                            std::make_tuple("Hostnames", "Processes", "Threads"),
                            max_name_size);
            }
        }else {
            for (const auto& [level,host]: data->host) {
                std::string s = join_iterator(data->host_backend_name[level]); 
                print_extended(s, host,
                            std::make_tuple("Hostname", "Process", "Thread"),
                            max_name_size);
            }
            print_extended("Device profiling", data->device,
                            std::make_tuple("Hostname", "Process", "Thread",
                                            "Device pointer", "Subdevice pointer"),
                            max_name_size);

            for (const auto& [level,traffic]: data->traffic) {
                std::string s = join_iterator(data->traffic_backend_name[level]);
                print_extended("Explicit memory traffic (" + s + ")", traffic,
                            std::make_tuple("Hostname", "Process", "Thread"),
                            max_name_size);
            }
        }
    } else {
        
        nlohmann::json j;
        j["units"] = {{"time", "ns"}, {"size", "bytes"}};
        
        if (data->params->display_metadata)
            j["metadata"] = data->metadata;
        
        if (data->params->display_compact) {
            for (auto& [level,host]: data->host)
                j["host"][level] = json_compact(host);

            if (!data->device.empty())
                j["device"] = json_compact(data->device);

            for (auto& [level,traffic]: data->traffic)
                j["traffic"][level] = json_compact(traffic);

        } else {
            for (auto& [level,host]: data->host)
                j["host"][level] = json_extented(host, std::make_tuple("Hostname", "Process", "Thread"));
                
            if (!data->device.empty())
                j["device"] = json_extented(data->device,std::make_tuple(
                    "Hostname", "Process","Thread", "Device pointer","Subdevice pointer"));

            for (auto& [level,traffic]: data->traffic)
                    j["traffic"][level] = json_extented(traffic,std::make_tuple(
                        "Hostname", "Process", "Thread"));
        }
        std::cout << j << std::endl;
    }

    /* Delete user data */
    delete data;
}

static void lttng_host_usr_callback(
    void *btx_handle, void *usr_data,   const char* hostname,
    int64_t vpid, uint64_t vtid, int64_t ts, int64_t backend_id, const char* name,
    uint64_t dur, bt_bool err
) 
{
    /* In callbacks, the user just  need to cast our API usr_data to his/her data structure */
    struct tally_dispatch *data = (struct tally_dispatch *) usr_data;

    TallyCoreTime a{dur, (uint64_t)err};
    const int level = backend_level[backend_id];
    data->host_backend_name[level].insert(backend_name[backend_id]);
    data->host[level][hpt_function_name_t(hostname, vpid, vtid, name)] += a;
}

static void lttng_device_usr_callback(
    void *btx_handle, void *usr_data, const char* hostname, int64_t vpid,
    uint64_t vtid, int64_t ts, int64_t backend, const char* name, uint64_t dur, 
    uint64_t did, uint64_t sdid, bt_bool err, const char* metadata
)
{
    /* In callbacks, the user just  need to cast our API usr_data to his/her data structure */
    struct tally_dispatch *data = (struct tally_dispatch *) usr_data;

    /* TODO: Should fucking cache this function */
    const auto name_demangled = (data->params->demangle_name) ? f_demangle_name(name) : name;
    const auto name_with_metadata = (data->params->display_kernel_verbose && !strcmp(metadata, "")) ? name_demangled + "[" + metadata + "]" : name_demangled;

    TallyCoreTime a{dur, (uint64_t)err};
    data->device[hpt_device_function_name_t(hostname, vpid, vtid, did, sdid, name_with_metadata)] += a;

}

static void lttng_traffic_usr_callback(
    void *btx_handle, void *usr_data, const char* hostname,
    int64_t vpid, uint64_t vtid, int64_t ts, int64_t backend, 
    const char* name, uint64_t size
)
{
    /* In callbacks, the user just  need to cast our API usr_data to his/her data structure */
    struct tally_dispatch *data = (struct tally_dispatch *) usr_data;

    TallyCoreByte a{(uint64_t)size, false};
    const int level = backend_level[backend];
    data->traffic_backend_name[level].insert(backend_name[backend]);
    data->traffic[level][hpt_function_name_t(hostname, vpid, vtid, name)] += a;
}

static void lttng_device_name_usr_callback(
    void *btx_handle, void *usr_data, const char* hostname, int64_t vpid,
    uint64_t vtid, int64_t ts, int64_t backend, const char* name, uint64_t did
)
{
    /* In callbacks, the user just  need to cast our API usr_data to his/her data structure */
    struct tally_dispatch *data = (struct tally_dispatch *) usr_data;

    data->device_name[hp_device_t(hostname, vpid, did)] = name;
}

static void lttng_thapi_metadata_usr_callback(
    void *btx_handle, void *usr_data,const char* hostname, int64_t vpid,
    uint64_t vtid, int64_t ts, int64_t backend, const char* metadata
)
{
    /* In callbacks, the user just  need to cast our API usr_data to his/her data structure */
    struct tally_dispatch *data = (struct tally_dispatch *) usr_data;

    data->metadata.push_back(metadata);
}

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_usr_data(btx_handle,&btx_initialize_usr_data);
  btx_register_callbacks_read_params(btx_handle, &btx_read_params);
  btx_register_callbacks_finalize_usr_data(btx_handle, &btx_finalize_usr_data);

  btx_register_callbacks_lttng_host(btx_handle, &lttng_host_usr_callback);
  btx_register_callbacks_lttng_device(btx_handle, &lttng_device_usr_callback);
  btx_register_callbacks_lttng_traffic(btx_handle, &lttng_traffic_usr_callback);
  btx_register_callbacks_lttng_device_name(btx_handle, &lttng_device_name_usr_callback);
  btx_register_callbacks_lttng_ust_thapi_metadata(btx_handle, &lttng_thapi_metadata_usr_callback);
}
