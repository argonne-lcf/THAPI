#include "xprof_utils.hpp"
#include <iostream>
#include <map>
#include <metababel/metababel.h>
#include <mpi.h.include>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_set>

// ---------------------------------------------------------------------------
// GPU-aware MPI tagging
//
// We maintain a per-process address-range map populated from
// lttng_ust_ze:zeMemAlloc*_exit / zeMemFree_entry events emitted by the ze
// backend (joined into our upstream model via backends/mpi/Makefile.am).
// Each MPI buffer-bearing entry event looks up its buffer pointer in that
// map; if the pointer falls inside a known GPU allocation, the
// {hostname,vpid,vtid} key is recorded in gpu_aware_calls. The matching
// exit callback consumes the key and prepends '*' to the MPI call name
// before pushing it to the aggregator, producing a separate '*MPI_Foo'
// row in the tally.
//
// When the user enables only --backend mpi, no ze events flow in, the
// range-map stays empty, and no '*' ever appears (CPU-only profile).
// ---------------------------------------------------------------------------

// Per-process address-range map: base -> (base + size).
typedef std::map<uintptr_t, uintptr_t> memory_interval_t;

struct data_s {
  EntryState entry_state;
  // GPU allocation ranges, keyed on {hostname, vpid}. Mirrors the
  // rangeset_memory_{host,device,shared} pattern in
  // backends/ze/btx_zeinterval_callbacks.cpp.
  std::unordered_map<hp_t, memory_interval_t> rangeset_memory_device;
  std::unordered_map<hp_t, memory_interval_t> rangeset_memory_shared;
  // {hostname, vpid, vtid} of MPI calls observed to carry at least one
  // device/shared GPU buffer. Populated in mpi_*_buf_entry_callback,
  // consumed (and cleared) in send_host_message.
  std::unordered_set<hpt_t> gpu_aware_calls;
};

typedef struct data_s data_t;

// True iff `ptr` lies inside any allocation tracked in `m`.
static bool memory_includes(const memory_interval_t &m, uintptr_t ptr) {
  const auto it = m.upper_bound(ptr);
  if (it == m.cbegin())
    return false;
  return ptr < std::prev(it)->second;
}

// True iff `ptr` lies inside a tracked GPU (device or shared) allocation
// for the given {hostname, vpid}. Host USM is intentionally NOT tagged
// as GPU-aware here (it behaves like host memory from MPI's perspective).
static bool is_gpu_pointer(const data_t *data, const hp_t &hp, uintptr_t ptr) {
  auto itd = data->rangeset_memory_device.find(hp);
  if (itd != data->rangeset_memory_device.end() && memory_includes(itd->second, ptr))
    return true;
  auto its = data->rangeset_memory_shared.find(hp);
  if (its != data->rangeset_memory_shared.end() && memory_includes(its->second, ptr))
    return true;
  return false;
}

static void send_host_message(void *btx_handle,
                              void *usr_data,
                              int64_t ts,
                              const char *event_class_name,
                              const char *hostname,
                              int64_t vpid,
                              uint64_t vtid,
                              bool err) {

  auto *data = static_cast<data_t *>(usr_data);
  const hpt_t hpt{hostname, vpid, vtid};

  std::string event_class_name_striped = strip_event_class_name_exit(event_class_name);
  // GPU-aware MPI calls get a leading '*' so they sort and aggregate as a
  // distinct row from their CPU-only counterparts in the tally output.
  if (data->gpu_aware_calls.erase(hpt) > 0)
    event_class_name_striped.insert(0, 1, '*');

  const int64_t entry_ts = data->entry_state.get_ts(hpt);

  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, entry_ts, BACKEND_MPI,
                              event_class_name_striped.c_str(), (ts - entry_ts), err);
}

void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }

static void entries_callback(void *btx_handle,
                             void *usr_data,
                             int64_t ts,
                             const char *event_class_name,
                             const char *hostname,
                             int64_t vpid,
                             uint64_t vtid) {
  static_cast<data_t *>(usr_data)->entry_state.set_ts({hostname, vpid, vtid}, ts);
}

static void exits_callback_mpiError_absent(void *btx_handle,
                                           void *usr_data,
                                           int64_t ts,
                                           const char *event_class_name,
                                           const char *hostname,
                                           int64_t vpid,
                                           uint64_t vtid) {

  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid, false);
}

static void exits_callback_mpiError_present(void *btx_handle,
                                            void *usr_data,
                                            int64_t ts,
                                            const char *event_class_name,
                                            const char *hostname,
                                            int64_t vpid,
                                            uint64_t vtid,
                                            int mpiResult) {

  send_host_message(btx_handle, usr_data, ts, event_class_name, hostname, vpid, vtid,
                    mpiResult != MPI_SUCCESS);
}

// MPICH ABI
// (https://github.com/pmodels/mpich/blob/main/src/mpi/datatype/typerep/src/typerep_ext32.c)
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

static void type_property_callback(void *btx_handle,
                                   void *usr_data,
                                   int64_t ts,
                                   const char *hostname,
                                   int64_t vpid,
                                   uint64_t vtid,
                                   MPI_Datatype datatype,
                                   int size) {

  mpi_datatype_info[(uint64_t)datatype] = {std::to_string((uint64_t)datatype), size};
}

static void traffic_MPI_Count_entry_callback(void *btx_handle,
                                             void *usr_data,
                                             int64_t ts,
                                             const char *event_class_name,
                                             const char *hostname,
                                             int64_t vpid,
                                             uint64_t vtid,
                                             MPI_Count count,
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

static void traffic_int_entry_callback(void *btx_handle,
                                       void *usr_data,
                                       int64_t ts,
                                       const char *event_class_name,
                                       const char *hostname,
                                       int64_t vpid,
                                       uint64_t vtid,
                                       int count,
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

// --- ze allocation / free tracking ----------------------------------------
// Bind allocation size at entry; the matching exit callback uses it together
// with the freshly-returned pointer to populate the range-map. The
// event_class_name suffix (Device vs Shared vs Host) selects which rangeset
// the allocation lands in.
static void ze_alloc_entry_callback(void *btx_handle,
                                    void *usr_data,
                                    int64_t ts,
                                    const char *event_class_name,
                                    const char *hostname,
                                    int64_t vpid,
                                    uint64_t vtid,
                                    size_t size) {
  static_cast<data_t *>(usr_data)->entry_state.set_data({hostname, vpid, vtid}, size);
}

static void ze_alloc_exit_callback(void *btx_handle,
                                   void *usr_data,
                                   int64_t ts,
                                   const char *event_class_name,
                                   const char *hostname,
                                   int64_t vpid,
                                   uint64_t vtid,
                                   void *pptr_val) {
  auto *data = static_cast<data_t *>(usr_data);
  auto size = data->entry_state.get_data<size_t>({hostname, vpid, vtid});
  // We don't see zeResult in the matching model to avoid pulling in
  // ze_api.h; a failed allocation has pptr_val == NULL.
  if (pptr_val == nullptr)
    return;

  const std::string ev{event_class_name};
  // Host USM is reachable from the host CPU so MPI sees it as a normal host
  // pointer; classifying it as GPU-aware would produce false positives.
  // Device and Shared (managed) USM live (at least partly) on the device,
  // so MPI calls on them are GPU-aware.
  std::unordered_map<hp_t, memory_interval_t> *mi = nullptr;
  if (ev.find("zeMemAllocDevice") != std::string::npos)
    mi = &data->rangeset_memory_device;
  else if (ev.find("zeMemAllocShared") != std::string::npos)
    mi = &data->rangeset_memory_shared;
  else
    return; // host USM, ignored

  const uintptr_t base = reinterpret_cast<uintptr_t>(pptr_val);
  (*mi)[{hostname, vpid}][base] = base + size;
}

static void ze_free_entry_callback(void *btx_handle,
                                   void *usr_data,
                                   int64_t ts,
                                   const char *event_class_name,
                                   const char *hostname,
                                   int64_t vpid,
                                   uint64_t vtid,
                                   void *ptr) {
  auto *data = static_cast<data_t *>(usr_data);
  if (ptr == nullptr)
    return;
  const uintptr_t base = reinterpret_cast<uintptr_t>(ptr);
  const hp_t hp{hostname, vpid};
  // The freed pointer may have been classified as device OR shared at
  // allocation; try both.
  auto itd = data->rangeset_memory_device.find(hp);
  if (itd != data->rangeset_memory_device.end())
    itd->second.erase(base);
  auto its = data->rangeset_memory_shared.find(hp);
  if (its != data->rangeset_memory_shared.end())
    its->second.erase(base);
}

// --- MPI buffer classification --------------------------------------------
static void
tag_if_gpu(data_t *data, const char *hostname, int64_t vpid, uint64_t vtid, const void *ptr) {
  if (ptr == nullptr)
    return;
  // MPICH sentinels: MPI_IN_PLACE / MPI_BOTTOM. These would never match a
  // real allocation but checking explicitly keeps the intent clear.
  if (ptr == MPI_IN_PLACE || ptr == MPI_BOTTOM)
    return;
  if (is_gpu_pointer(data, {hostname, vpid}, reinterpret_cast<uintptr_t>(ptr)))
    data->gpu_aware_calls.insert({hostname, vpid, vtid});
}

static void mpi_1buf_entry_callback(void *btx_handle,
                                    void *usr_data,
                                    int64_t ts,
                                    const char *event_class_name,
                                    const char *hostname,
                                    int64_t vpid,
                                    uint64_t vtid,
                                    const void *buf) {
  tag_if_gpu(static_cast<data_t *>(usr_data), hostname, vpid, vtid, buf);
}

// MPI_Sendrecv, MPI_Allreduce, MPI_Reduce, MPI_Gather, ... — the bulk.
static void mpi_sendrecv_entry_callback(void *btx_handle,
                                        void *usr_data,
                                        int64_t ts,
                                        const char *event_class_name,
                                        const char *hostname,
                                        int64_t vpid,
                                        uint64_t vtid,
                                        const void *sendbuf,
                                        const void *recvbuf) {
  auto *data = static_cast<data_t *>(usr_data);
  tag_if_gpu(data, hostname, vpid, vtid, sendbuf);
  tag_if_gpu(data, hostname, vpid, vtid, recvbuf);
}

// MPI_Reduce_local(inbuf, inoutbuf, ...)
static void mpi_reduce_local_entry_callback(void *btx_handle,
                                            void *usr_data,
                                            int64_t ts,
                                            const char *event_class_name,
                                            const char *hostname,
                                            int64_t vpid,
                                            uint64_t vtid,
                                            const void *inbuf,
                                            const void *inoutbuf) {
  auto *data = static_cast<data_t *>(usr_data);
  tag_if_gpu(data, hostname, vpid, vtid, inbuf);
  tag_if_gpu(data, hostname, vpid, vtid, inoutbuf);
}

// MPI_Get_accumulate, MPI_Fetch_and_op (origin_addr + result_addr).
static void mpi_rma_fetch_entry_callback(void *btx_handle,
                                         void *usr_data,
                                         int64_t ts,
                                         const char *event_class_name,
                                         const char *hostname,
                                         int64_t vpid,
                                         uint64_t vtid,
                                         const void *origin_addr,
                                         const void *result_addr) {
  auto *data = static_cast<data_t *>(usr_data);
  tag_if_gpu(data, hostname, vpid, vtid, origin_addr);
  tag_if_gpu(data, hostname, vpid, vtid, result_addr);
}

// MPI_Compare_and_swap(origin_addr, compare_addr, result_addr, ...)
static void mpi_cas_entry_callback(void *btx_handle,
                                   void *usr_data,
                                   int64_t ts,
                                   const char *event_class_name,
                                   const char *hostname,
                                   int64_t vpid,
                                   uint64_t vtid,
                                   const void *origin_addr,
                                   const void *compare_addr,
                                   const void *result_addr) {
  auto *data = static_cast<data_t *>(usr_data);
  tag_if_gpu(data, hostname, vpid, vtid, origin_addr);
  tag_if_gpu(data, hostname, vpid, vtid, compare_addr);
  tag_if_gpu(data, hostname, vpid, vtid, result_addr);
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

  // GPU-aware MPI: ze alloc/free updates the range-map, MPI buffer entries
  // look up against it. One callback per MPI buffer-shape (see the matching
  // model for why we cannot lump them into a single regex).
  btx_register_callbacks_ze_alloc_entry(btx_handle, &ze_alloc_entry_callback);
  btx_register_callbacks_ze_alloc_exit(btx_handle, &ze_alloc_exit_callback);
  btx_register_callbacks_ze_free_entry(btx_handle, &ze_free_entry_callback);
  btx_register_callbacks_mpi_1buf_entry(btx_handle, &mpi_1buf_entry_callback);
  btx_register_callbacks_mpi_sendrecv_entry(btx_handle, &mpi_sendrecv_entry_callback);
  btx_register_callbacks_mpi_reduce_local_entry(btx_handle, &mpi_reduce_local_entry_callback);
  btx_register_callbacks_mpi_rma_fetch_entry(btx_handle, &mpi_rma_fetch_entry_callback);
  btx_register_callbacks_mpi_cas_entry(btx_handle, &mpi_cas_entry_callback);
}
