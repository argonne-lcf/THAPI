// ze_device_property.cpp
//
// Standalone Level Zero utility that queries every device on the system and
// dumps its command-queue-group topology to `ze_device_property.json`.
//
// For each command queue group it reports:
//   - the group ordinal (the value passed as `ordinal` in ze_command_queue_desc_t
//     or `commandQueueGroupOrdinal` in ze_command_list_desc_t)
//   - the engine type derived from the group flags:
//       "compute", "copy", or "compute-and-copy"
//   - numQueues: the number of physical engines (queue indices) in the group,
//     i.e. the valid range for ze_command_queue_desc_t::index is [0, numQueues-1]
//
// This binary is meant to be invoked by a separate program; it performs no
// argument parsing beyond an optional output path.
//
// Build: compiled with icpx (see Makefile.am).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include <ze_api.h>

namespace {

const char *result_to_string(ze_result_t r) {
  switch (r) {
  case ZE_RESULT_SUCCESS:
    return "ZE_RESULT_SUCCESS";
  case ZE_RESULT_ERROR_UNINITIALIZED:
    return "ZE_RESULT_ERROR_UNINITIALIZED";
  case ZE_RESULT_ERROR_DEVICE_LOST:
    return "ZE_RESULT_ERROR_DEVICE_LOST";
  case ZE_RESULT_ERROR_INVALID_NULL_HANDLE:
    return "ZE_RESULT_ERROR_INVALID_NULL_HANDLE";
  case ZE_RESULT_ERROR_INVALID_NULL_POINTER:
    return "ZE_RESULT_ERROR_INVALID_NULL_POINTER";
  case ZE_RESULT_ERROR_UNSUPPORTED_FEATURE:
    return "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE";
  default:
    return "ZE_RESULT_ERROR (unlisted)";
  }
}

// Fatal-checks a Level Zero call; on failure prints the site and exits(1).
#define ZE_CHECK(call)                                                          \
  do {                                                                          \
    ze_result_t _res = (call);                                                  \
    if (_res != ZE_RESULT_SUCCESS) {                                           \
      std::fprintf(stderr, "%s failed: %s (0x%x)\n", #call,                    \
                   result_to_string(_res), (unsigned)_res);                    \
      std::exit(1);                                                            \
    }                                                                          \
  } while (0)

// Classify a command queue group by its flags.
const char *engine_type(ze_command_queue_group_property_flags_t flags) {
  const bool compute = flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE;
  const bool copy = flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY;
  if (compute && copy)
    return "compute-and-copy";
  if (compute)
    return "compute";
  if (copy)
    return "copy";
  return "other";
}

} // namespace

int main(int argc, char **argv) {
  const char *out_path =
      (argc > 1) ? argv[1] : "ze_device_property.json";

  ZE_CHECK(zeInit(ZE_INIT_FLAG_GPU_ONLY));

  uint32_t driver_count = 0;
  ZE_CHECK(zeDriverGet(&driver_count, nullptr));
  std::vector<ze_driver_handle_t> drivers(driver_count);
  if (driver_count > 0)
    ZE_CHECK(zeDriverGet(&driver_count, drivers.data()));

  std::ofstream out(out_path);
  if (!out) {
    std::fprintf(stderr, "unable to open '%s' for writing\n", out_path);
    return 1;
  }

  out << "{\n  \"devices\": [";

  bool first_device = true;
  for (uint32_t d = 0; d < driver_count; ++d) {
    uint32_t device_count = 0;
    ZE_CHECK(zeDeviceGet(drivers[d], &device_count, nullptr));
    std::vector<ze_device_handle_t> devices(device_count);
    if (device_count > 0)
      ZE_CHECK(zeDeviceGet(drivers[d], &device_count, devices.data()));

    for (uint32_t i = 0; i < device_count; ++i) {
      ze_device_properties_t dev_props{};
      dev_props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
      ZE_CHECK(zeDeviceGetProperties(devices[i], &dev_props));

      uint32_t group_count = 0;
      ZE_CHECK(zeDeviceGetCommandQueueGroupProperties(devices[i], &group_count,
                                                      nullptr));
      std::vector<ze_command_queue_group_properties_t> groups(group_count);
      for (auto &g : groups)
        g.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES;
      if (group_count > 0)
        ZE_CHECK(zeDeviceGetCommandQueueGroupProperties(
            devices[i], &group_count, groups.data()));

      out << (first_device ? "\n" : ",\n");
      first_device = false;

      out << "    {\n";
      out << "      \"driver_index\": " << d << ",\n";
      out << "      \"device_index\": " << i << ",\n";
      out << "      \"name\": \"" << dev_props.name << "\",\n";
      out << "      \"command_queue_groups\": [";

      for (uint32_t g = 0; g < group_count; ++g) {
        out << (g == 0 ? "\n" : ",\n");
        out << "        {\n";
        out << "          \"ordinal\": " << g << ",\n";
        out << "          \"type\": \"" << engine_type(groups[g].flags)
            << "\",\n";
        out << "          \"numQueues\": " << groups[g].numQueues << "\n";
        out << "        }";
      }

      out << (group_count == 0 ? "" : "\n      ") << "]\n";
      out << "    }";
    }
  }

  out << (first_device ? "" : "\n  ") << "]\n}\n";
  out.close();

  std::fprintf(stderr, "wrote %s\n", out_path);
  return 0;
}
