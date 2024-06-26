#include "xprof_utils.hpp"
#include <iostream>
#include <metababel/metababel.h>
#include <mpi.h.include>
#include <string>
#include <tuple>
#include <sstream>

struct data_s {
  EntryState entry_state;
};

typedef struct data_s data_t;

static void send_host_message(void *btx_handle, void *usr_data, int64_t ts,
                              const char *event_class_name, const char *hostname, int64_t vpid,
                              uint64_t vtid, bool err) {

  std::string event_class_name_striped = strip_event_class_name_exit(event_class_name);
  const int64_t entry_ts =
      static_cast<data_t *>(usr_data)->entry_state.get_ts({hostname, vpid, vtid});

  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, entry_ts, BACKEND_MPI,
                              event_class_name_striped.c_str(), (ts - entry_ts), err);
}

void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }

static void entries_callback(void *btx_handle, void *usr_data, int64_t ts,
                             const char *event_class_name, const char *hostname, int64_t vpid,
                             uint64_t vtid) {
  static_cast<data_t *>(usr_data)->entry_state.set_ts({hostname, vpid, vtid}, ts);
}

static void exits_callback_mpiError_absent(void *btx_handle, void *usr_data, int64_t ts,
                                           const char *event_class_name, const char *hostname,
                                           int64_t vpid, uint64_t vtid) {

  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, false);
}

static void exits_callback_mpiError_present(void *btx_handle, void *usr_data, int64_t ts,
                                            const char *event_class_name, const char *hostname,
                                            int64_t vpid, uint64_t vtid, int mpiResult) {

  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid,
                    mpiResult != MPI_SUCCESS);
}

// MPICH ABI (https://github.com/pmodels/mpich/blob/main/src/mpi/datatype/typerep/src/typerep_ext32.c)
std::unordered_map<uint64_t, std::tuple<std::string, int>> mpi_datatype_info = {
    {0xc000000, {"MPI_DATATYPE_NULL", 0}},
    {0x4c000843, {"MPI_AINT", 8}},
    {0x4c000845, {"MPI_COUNT", 8}},
    {0x4c000844, {"MPI_OFFSET", 8}},
    {0x4c00010f, {"MPI_PACKED", 1}},
    {0x4c000203, {"MPI_SHORT", 2}},
    {0x4c000405, {"MPI_INT", 4}},
    {0x4c000807, {"MPI_LONG", 8}},
    {0x4c000809, {"MPI_LONG_LONG", 8}},
    {0x4c000809, {"MPI_LONG_LONG_INT", 8}},
    {0x4c000204, {"MPI_UNSIGNED_SHORT", 2}},
    {0x4c000406, {"MPI_UNSIGNED", 4}},
    {0x4c000808, {"MPI_UNSIGNED_LONG", 8}},
    {0x4c000819, {"MPI_UNSIGNED_LONG_LONG", 8}},
    {0x4c00040a, {"MPI_FLOAT", 4}},
    {0x4c000840, {"MPI_C_FLOAT_COMPLEX", 8}},
    {0x4c000840, {"MPI_C_COMPLEX", 8}},
    {0x4c000834, {"MPI_CXX_FLOAT_COMPLEX", 8}},
    {0x4c00080b, {"MPI_DOUBLE", 8}},
    {0x4c001041, {"MPI_C_DOUBLE_COMPLEX", 16}},
    {0x4c001035, {"MPI_CXX_DOUBLE_COMPLEX", 16}},
    {0x4c00041d, {"MPI_LOGICAL", 4}},
    {0x4c00041b, {"MPI_INTEGER", 4}},
    {0x4c00041c, {"MPI_REAL", 4}},
    {0x4c00081e, {"MPI_COMPLEX", 8}},
    {0x4c00081f, {"MPI_DOUBLE_PRECISION", 8}},
    {0x4c001022, {"MPI_DOUBLE_COMPLEX", 16}},
    {0x4c00100c, {"MPI_LONG_DOUBLE", 16}},
    {0x4c002042, {"MPI_C_LONG_DOUBLE_COMPLEX", 32}},
    {0x4c002036, {"MPI_CXX_LONG_DOUBLE_COMPLEX", 32}},
    {0xffffffff8c000000, {"MPI_FLOAT_INT", 8}},
    {0xffffffff8c000001, {"MPI_DOUBLE_INT", 12}},
    {0xffffffff8c000002, {"MPI_LONG_INT", 12}},
    {0x4c000816, {"MPI_2INT", 8}},
    {0xffffffff8c000003, {"MPI_SHORT_INT", 6}},
    {0xffffffff8c000004, {"MPI_LONG_DOUBLE_INT", 20}},
    {0x4c000821, {"MPI_2REAL", 8}},
    {0x4c001023, {"MPI_2DOUBLE_PRECISION", 16}},
    {0x4c000820, {"MPI_2INTEGER", 8}},
    {0x4c00013f, {"MPI_C_BOOL", 1}},
    {0x4c000133, {"MPI_CXX_BOOL", 1}},
    {0x4c00040e, {"MPI_WCHAR", 4}},
    {0x4c000137, {"MPI_INT8_T", 1}},
    {0x4c00013b, {"MPI_UINT8_T", 1}},
    {0x4c000101, {"MPI_CHAR", 1}},
    {0x4c000118, {"MPI_SIGNED_CHAR", 1}},
    {0x4c000102, {"MPI_UNSIGNED_CHAR", 1}},
    {0x4c00010d, {"MPI_BYTE", 1}},
    {0x4c000238, {"MPI_INT16_T", 2}},
    {0x4c00023c, {"MPI_UINT16_T", 2}},
    {0x4c000439, {"MPI_INT32_T", 4}},
    {0x4c00043d, {"MPI_UINT32_T", 4}},
    {0x4c00083a, {"MPI_INT64_T", 8}},
    {0x4c00083e, {"MPI_UINT64_T", 8}},
    {0x4c00012d, {"MPI_INTEGER1", 1}},
    {0x4c00011a, {"MPI_CHARACTER", 1}},
    {0x4c00022f, {"MPI_INTEGER2", 2}},
    {0x4c000430, {"MPI_INTEGER4", 4}},
    {0x4c000427, {"MPI_REAL4", 4}},
    {0x4c000831, {"MPI_INTEGER8", 8}},
    {0x4c000829, {"MPI_REAL8", 8}},
    {0x4c000828, {"MPI_COMPLEX8", 8}},
    {0xc000000, {"MPI_INTEGER16", 16}},
    {0x4c00102b, {"MPI_REAL16", 16}},
    {0x4c00102a, {"MPI_COMPLEX16", 16}},
};

static void type_property_callback(void *btx_handle, void *usr_data, int64_t ts,
                                             const char *hostname,
                                             int64_t vpid, uint64_t vtid, MPI_Datatype datatype, int size ) {

	mpi_datatype_info[(uint64_t) datatype] = { std::to_string( (uint64_t) datatype), size} ;
}

static void traffic_MPI_Count_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                             const char *event_class_name, const char *hostname,
                                             int64_t vpid, uint64_t vtid, MPI_Count count,
                                             MPI_Datatype datatype) {

  auto it = mpi_datatype_info.find((uint64_t)datatype);
  if (it == mpi_datatype_info.end()) {
    std::cerr << "THAPI: Warning MPI datatype " << datatype << " unknow" << std::endl;
    return;
  }
  auto &[str, size] = it->second;
  std::ostringstream oss;
  oss << str << ", " << count;

  btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, ts, BACKEND_MPI,
                                 strip_event_class_name_entry(event_class_name).c_str(),
                                 count * size, oss.str().c_str());
}

static void traffic_int_entry_callback(void *btx_handle, void *usr_data, int64_t ts,
                                       const char *event_class_name, const char *hostname,
                                       int64_t vpid, uint64_t vtid, int count,
                                       MPI_Datatype datatype) {

  auto it = mpi_datatype_info.find((uint64_t)datatype);
  if (it == mpi_datatype_info.end()) {
    std::cerr << "THAPI: Warning MPI datatype " << datatype << " unknow" << std::endl;
    return;
  }

  auto &[str, size] = it->second;
  std::ostringstream oss;
  oss << str << ", " << count;

  btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, ts, BACKEND_MPI,
                                 strip_event_class_name_entry(event_class_name).c_str(),
                                 count * size, oss.str().c_str());
}



void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);
  btx_register_callbacks_entries(btx_handle, &entries_callback);
  btx_register_callbacks_exits_mpiError_absent(btx_handle, &exits_callback_mpiError_absent);
  btx_register_callbacks_exits_mpiError_present(btx_handle, &exits_callback_mpiError_present);

  btx_register_callbacks_traffic_MPI_Count_entry(btx_handle, &traffic_MPI_Count_entry_callback);
  btx_register_callbacks_traffic_int_entry(btx_handle, &traffic_int_entry_callback);

  btx_register_callbacks_lttng_ust_mpi_type_property(btx_handle, &type_property_callback);
}
