#include "babeltrace2/babeltrace.h"
#include "babeltrace_cl.h"
#include "clprof.h"
#include "utils.h"
#include <iomanip>
#include <vector>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <tuple>
#include <climits>
#include "clprof_callbacks.h"
#include <set>
/*
Global variable
*/
const bt_value *display_mode;
typedef std::string hostname_t;
typedef std::string thapi_command_name;
typedef int64_t     process_id_t;
typedef uint64_t    thread_id_t;
typedef std::tuple<hostname_t, process_id_t> hp_t;
typedef std::tuple<thread_id_t, thapi_command_name> t_command_name_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t> hpt_t;
typedef std::tuple<hostname_t, process_id_t, cl_command_queue> hp_command_queue_t;
typedef std::tuple<hostname_t, process_id_t, cl_device_id> hp_device_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, cl_device_id> hpt_device_t;
typedef std::tuple<hostname_t, process_id_t, cl_event> hp_event_t;
typedef std::tuple<hostname_t, process_id_t, cl_kernel> hp_kernel_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, thapi_command_name> hpt_command_name_t;
typedef std::tuple<hostname_t, process_id_t, thread_id_t, cl_device_id, thapi_command_name> hpt_device_command_name_t;
std::unordered_map<hp_t, cl_device_id> last_device;
std::unordered_map<hpt_t, thapi_command_name> last_command;
std::unordered_map<hp_command_queue_t, cl_device_id> command_queue_to_device;
std::unordered_map<hpt_command_name_t, cl_device_id> command_name_to_device;
std::unordered_map<hp_event_t,t_command_name_t> event_to_command_name;
std::unordered_map<hpt_device_command_name_t, StatTime> device_id_result;
std::unordered_map<hpt_command_name_t, StatTime> api_call;
std::unordered_map<hpt_command_name_t, StatByte> memory_trafic;
std::unordered_map<hp_event_t, uint64_t> event_result_to_delta;
std::unordered_map<hp_device_t, std::string> device_to_name;
std::unordered_map<hp_kernel_t, std::string> kernel_to_name;
/* Callback */
static void clprof_lttng_ust_opencl_clCreateEventFromEGLSyncKHR_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
CLeglSyncKHR sync,
CLeglDisplayKHR display,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateEventFromEGLSyncKHR")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateEventFromEGLSyncKHR_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateEventFromEGLSyncKHR")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromEGLImageKHR_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
CLeglDisplayKHR egldisplay,
CLeglImageKHR eglimage,
cl_mem_flags flags,
cl_egl_image_properties_khr * properties,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_egl_image_properties_khr * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromEGLImageKHR")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromEGLImageKHR_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromEGLImageKHR")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueAcquireEGLObjectsKHR_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_objects,
cl_mem * mem_objects,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _mem_objects_vals_length,
cl_mem * mem_objects_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueAcquireEGLObjectsKHR")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueAcquireEGLObjectsKHR";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueAcquireEGLObjectsKHR_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueAcquireEGLObjectsKHR")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueReleaseEGLObjectsKHR_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_objects,
cl_mem * mem_objects,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _mem_objects_vals_length,
cl_mem * mem_objects_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReleaseEGLObjectsKHR")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueReleaseEGLObjectsKHR";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueReleaseEGLObjectsKHR_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReleaseEGLObjectsKHR")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseDeviceEXT_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseDeviceEXT")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clReleaseDeviceEXT_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseDeviceEXT")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainDeviceEXT_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainDeviceEXT")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clRetainDeviceEXT_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainDeviceEXT")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateSubDevicesEXT_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id in_device,
cl_device_partition_property_ext * properties,
cl_uint num_entries,
cl_device_id * out_devices,
cl_uint * num_devices,
size_t _properties_vals_length,
cl_device_partition_property_ext * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateSubDevicesEXT")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateSubDevicesEXT_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_uint num_devices_val,
size_t _out_devices_vals_length,
cl_device_id * out_devices_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateSubDevicesEXT")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetKernelSubGroupInfoKHR_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel in_kernel,
cl_device_id in_device,
cl_kernel_sub_group_info param_name,
size_t input_value_size,
void * input_value,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetKernelSubGroupInfoKHR")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetKernelSubGroupInfoKHR_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetKernelSubGroupInfoKHR")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateEventFromGLsyncKHR_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_GLsync sync,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateEventFromGLsyncKHR")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateEventFromGLsyncKHR_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateEventFromGLsyncKHR")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetGLContextInfoKHR_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context_properties * properties,
cl_gl_context_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetGLContextInfoKHR")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetGLContextInfoKHR_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetGLContextInfoKHR")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromGLBuffer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
cl_GLuint bufobj,
int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromGLBuffer")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromGLBuffer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromGLBuffer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromGLTexture_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
cl_GLenum target,
cl_GLint miplevel,
cl_GLuint texture,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromGLTexture")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromGLTexture_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromGLTexture")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromGLRenderbuffer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
cl_GLuint renderbuffer,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromGLRenderbuffer")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromGLRenderbuffer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromGLRenderbuffer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetGLObjectInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem memobj,
cl_gl_object_type * gl_object_type,
cl_GLuint * gl_object_name
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetGLObjectInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetGLObjectInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_gl_object_type gl_object_type_val,
cl_GLuint gl_object_name_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetGLObjectInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetGLTextureInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem memobj,
cl_gl_texture_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetGLTextureInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetGLTextureInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetGLTextureInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueAcquireGLObjects_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_objects,
cl_mem * mem_objects,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _mem_objects_vals_length,
cl_mem * mem_objects_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueAcquireGLObjects")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueAcquireGLObjects";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueAcquireGLObjects_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueAcquireGLObjects")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueReleaseGLObjects_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_objects,
cl_mem * mem_objects,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _mem_objects_vals_length,
cl_mem * mem_objects_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReleaseGLObjects")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueReleaseGLObjects";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueReleaseGLObjects_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReleaseGLObjects")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromGLTexture2D_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
cl_GLenum target,
cl_GLint miplevel,
cl_GLuint texture,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromGLTexture2D")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromGLTexture2D_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromGLTexture2D")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromGLTexture3D_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
cl_GLenum target,
cl_GLint miplevel,
cl_GLuint texture,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromGLTexture3D")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateFromGLTexture3D_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateFromGLTexture3D")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetPlatformIDs_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_uint num_entries,
cl_platform_id * platforms,
cl_uint * num_platforms
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetPlatformIDs")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetPlatformIDs_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_uint num_platforms_val,
size_t _platforms_vals_length,
cl_platform_id * platforms_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetPlatformIDs")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetPlatformInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_platform_id platform,
cl_platform_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetPlatformInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetPlatformInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetPlatformInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetDeviceIDs_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_platform_id platform,
cl_device_type device_type,
cl_uint num_entries,
cl_device_id * devices,
cl_uint * num_devices
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetDeviceIDs")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetDeviceIDs_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_uint num_devices_val,
size_t _devices_vals_length,
cl_device_id * devices_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetDeviceIDs")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetDeviceInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device,
cl_device_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetDeviceInfo")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clGetDeviceInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetDeviceInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateSubDevices_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id in_device,
cl_device_partition_property * properties,
cl_uint num_devices,
cl_device_id * out_devices,
cl_uint * num_devices_ret,
size_t _properties_vals_length,
cl_device_partition_property * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateSubDevices")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateSubDevices_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_uint num_devices_ret_val,
size_t _out_devices_vals_length,
cl_device_id * out_devices_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateSubDevices")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainDevice_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainDevice")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clRetainDevice_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainDevice")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseDevice_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseDevice")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clReleaseDevice_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseDevice")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetDefaultDeviceCommandQueue_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_device_id device,
cl_command_queue command_queue
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetDefaultDeviceCommandQueue")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clSetDefaultDeviceCommandQueue_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetDefaultDeviceCommandQueue")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetDeviceAndHostTimer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device,
cl_ulong * device_timestamp,
cl_ulong * host_timestamp
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetDeviceAndHostTimer")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clGetDeviceAndHostTimer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_ulong device_timestamp_val,
cl_ulong host_timestamp_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetDeviceAndHostTimer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetHostTimer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device,
cl_ulong * host_timestamp
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetHostTimer")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clGetHostTimer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_ulong host_timestamp_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetHostTimer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateContext_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context_properties * properties,
cl_uint num_devices,
cl_device_id * devices,
void * pfn_notify,
void * user_data,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_context_properties * properties_vals,
size_t _devices_vals_length,
cl_device_id * devices_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateContext")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateContext_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateContext")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateContextFromType_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context_properties * properties,
cl_device_type device_type,
void * pfn_notify,
void * user_data,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_context_properties * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateContextFromType")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateContextFromType_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateContextFromType")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainContext_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainContext")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainContext_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainContext")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseContext_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseContext")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseContext_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseContext")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetContextInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_context_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetContextInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetContextInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetContextInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateCommandQueueWithProperties_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_device_id device,
cl_queue_properties * properties,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_queue_properties * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateCommandQueueWithProperties")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clCreateCommandQueueWithProperties_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateCommandQueueWithProperties")].stop(ns_from_origin);
    const cl_device_id device  = last_device[hp_t(hostname,process_id) ];
    command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)] = device ;
}
static void clprof_lttng_ust_opencl_clRetainCommandQueue_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainCommandQueue")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clRetainCommandQueue";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clRetainCommandQueue_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainCommandQueue")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseCommandQueue_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseCommandQueue")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clReleaseCommandQueue";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clReleaseCommandQueue_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseCommandQueue")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetCommandQueueInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_command_queue_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetCommandQueueInfo")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clGetCommandQueueInfo";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clGetCommandQueueInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetCommandQueueInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateBuffer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
size_t size,
void * host_ptr,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateBuffer")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateBuffer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateBuffer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateBufferWithProperties_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_properties * properties,
cl_mem_flags flags,
size_t size,
void * host_ptr,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_mem_properties * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateBufferWithProperties")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateBufferWithProperties_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateBufferWithProperties")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateSubBuffer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem buffer,
cl_mem_flags flags,
cl_buffer_create_type buffer_create_type,
void * buffer_create_info,
cl_int * errcode_ret,
size_t _buffer_create_info_vals_length,
void * buffer_create_info_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateSubBuffer")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateSubBuffer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateSubBuffer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateImage_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
cl_image_format * image_format,
cl_image_desc * image_desc,
void * host_ptr,
cl_int * errcode_ret,
cl_channel_order image_format__image_channel_order,
cl_channel_type image_format__image_channel_data_type,
cl_mem_object_type image_desc__image_type,
size_t image_desc__image_width,
size_t image_desc__image_height,
size_t image_desc__image_depth,
size_t image_desc__image_array_size,
size_t image_desc__image_row_pitch,
size_t image_desc__image_slice_pitch,
cl_uint image_desc__num_mip_levels,
cl_uint image_desc__num_samples,
cl_mem image_desc__buffer
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateImage")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateImage_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateImage")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateImageWithProperties_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_properties * properties,
cl_mem_flags flags,
cl_image_format * image_format,
cl_image_desc * image_desc,
void * host_ptr,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_mem_properties * properties_vals,
cl_channel_order image_format__image_channel_order,
cl_channel_type image_format__image_channel_data_type,
cl_mem_object_type image_desc__image_type,
size_t image_desc__image_width,
size_t image_desc__image_height,
size_t image_desc__image_depth,
size_t image_desc__image_array_size,
size_t image_desc__image_row_pitch,
size_t image_desc__image_slice_pitch,
cl_uint image_desc__num_mip_levels,
cl_uint image_desc__num_samples,
cl_mem image_desc__buffer
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateImageWithProperties")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateImageWithProperties_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateImageWithProperties")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreatePipe_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
cl_uint pipe_packet_size,
cl_uint pipe_max_packets,
cl_pipe_properties * properties,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_pipe_properties * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreatePipe")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreatePipe_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreatePipe")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainMemObject_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem memobj
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainMemObject")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainMemObject_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainMemObject")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseMemObject_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem memobj
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseMemObject")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseMemObject_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseMemObject")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetSupportedImageFormats_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
cl_mem_object_type image_type,
cl_uint num_entries,
cl_image_format * image_formats,
cl_uint * num_image_formats
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetSupportedImageFormats")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetSupportedImageFormats_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_uint num_image_formats_val,
size_t _image_formats_vals_length,
cl_image_format * image_formats_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetSupportedImageFormats")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetMemObjectInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem memobj,
cl_mem_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetMemObjectInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetMemObjectInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetMemObjectInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetImageInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem image,
cl_image_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetImageInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetImageInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetImageInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetPipeInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem pipe,
cl_pipe_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetPipeInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetPipeInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetPipeInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetMemObjectDestructorCallback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem memobj,
void * pfn_notify,
void * user_data
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetMemObjectDestructorCallback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetMemObjectDestructorCallback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetMemObjectDestructorCallback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSVMAlloc_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_svm_mem_flags flags,
size_t size,
cl_uint alignment
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSVMAlloc")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSVMAlloc_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
void * _retval
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSVMAlloc")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSVMFree_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
void * svm_pointer
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSVMFree")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSVMFree_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSVMFree")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateSamplerWithProperties_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_sampler_properties * sampler_properties,
cl_int * errcode_ret,
size_t _sampler_properties_vals_length,
cl_sampler_properties * sampler_properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateSamplerWithProperties")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateSamplerWithProperties_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_sampler sampler,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateSamplerWithProperties")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainSampler_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_sampler sampler
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainSampler")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainSampler_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainSampler")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseSampler_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_sampler sampler
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseSampler")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseSampler_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseSampler")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetSamplerInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_sampler sampler,
cl_sampler_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetSamplerInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetSamplerInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetSamplerInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateProgramWithSource_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_uint count,
char* * strings,
size_t * lengths,
cl_int * errcode_ret,
size_t _strings_vals_length,
char * strings_vals,
size_t _lengths_vals_length,
size_t * lengths_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateProgramWithSource")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateProgramWithSource_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateProgramWithSource")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateProgramWithBinary_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_uint num_devices,
cl_device_id * device_list,
size_t * lengths,
unsigned char* * binaries,
cl_int * binary_status,
cl_int * errcode_ret,
size_t _device_list_vals_length,
cl_device_id * device_list_vals,
size_t _lengths_vals_length,
size_t * lengths_vals,
size_t _binaries_vals_length,
unsigned char * binaries_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateProgramWithBinary")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateProgramWithBinary_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_int errcode_ret_val,
size_t _binary_status_vals_length,
cl_int * binary_status_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateProgramWithBinary")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateProgramWithBuiltInKernels_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_uint num_devices,
cl_device_id * device_list,
char * kernel_names,
cl_int * errcode_ret,
size_t _device_list_vals_length,
cl_device_id * device_list_vals,
char * kernel_names_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateProgramWithBuiltInKernels")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateProgramWithBuiltInKernels_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateProgramWithBuiltInKernels")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateProgramWithIL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
void * il,
size_t length,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateProgramWithIL")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateProgramWithIL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateProgramWithIL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainProgram_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainProgram")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainProgram_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainProgram")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseProgram_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseProgram")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseProgram_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseProgram")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clBuildProgram_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_uint num_devices,
cl_device_id * device_list,
char * options,
void * pfn_notify,
void * user_data,
size_t _device_list_vals_length,
cl_device_id * device_list_vals,
char * options_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clBuildProgram")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clBuildProgram_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clBuildProgram")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCompileProgram_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_uint num_devices,
cl_device_id * device_list,
char * options,
cl_uint num_input_headers,
cl_program * input_headers,
char* * header_include_names,
void * pfn_notify,
void * user_data,
size_t _device_list_vals_length,
cl_device_id * device_list_vals,
char * options_val,
size_t _input_headers_vals_length,
cl_program * input_headers_vals,
size_t _header_include_names_vals_length,
char * header_include_names_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCompileProgram")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCompileProgram_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCompileProgram")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clLinkProgram_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_uint num_devices,
cl_device_id * device_list,
char * options,
cl_uint num_input_programs,
cl_program * input_programs,
void * pfn_notify,
void * user_data,
cl_int * errcode_ret,
size_t _device_list_vals_length,
cl_device_id * device_list_vals,
char * options_val,
size_t _input_programs_vals_length,
cl_program * input_programs_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clLinkProgram")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clLinkProgram_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clLinkProgram")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetProgramReleaseCallback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
void * pfn_notify,
void * user_data
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetProgramReleaseCallback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetProgramReleaseCallback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetProgramReleaseCallback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetProgramSpecializationConstant_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_uint spec_id,
size_t spec_size,
void * spec_value,
size_t _spec_value_vals_length,
void * spec_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetProgramSpecializationConstant")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetProgramSpecializationConstant_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetProgramSpecializationConstant")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clUnloadPlatformCompiler_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_platform_id platform
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clUnloadPlatformCompiler")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clUnloadPlatformCompiler_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clUnloadPlatformCompiler")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetProgramInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_program_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetProgramInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetProgramInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetProgramInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetProgramBuildInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_device_id device,
cl_program_build_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetProgramBuildInfo")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clGetProgramBuildInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetProgramBuildInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateKernel_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
char * kernel_name,
cl_int * errcode_ret,
char * kernel_name_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateKernel")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateKernel_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateKernel")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateKernelsInProgram_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_uint num_kernels,
cl_kernel * kernels,
cl_uint * num_kernels_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateKernelsInProgram")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateKernelsInProgram_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t _kernels_vals_length,
cl_kernel * kernels_vals,
cl_uint num_kernels_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateKernelsInProgram")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCloneKernel_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel source_kernel,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCloneKernel")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCloneKernel_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCloneKernel")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainKernel_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainKernel")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainKernel_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainKernel")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseKernel_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseKernel")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseKernel_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseKernel")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetKernelArg_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_uint arg_index,
size_t arg_size,
void * arg_value,
size_t _arg_value_vals_length,
void * arg_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetKernelArg")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetKernelArg_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetKernelArg")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetKernelArgSVMPointer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_uint arg_index,
void * arg_value
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetKernelArgSVMPointer")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetKernelArgSVMPointer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetKernelArgSVMPointer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetKernelExecInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_kernel_exec_info param_name,
size_t param_value_size,
void * param_value,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetKernelExecInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetKernelExecInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetKernelExecInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetKernelInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_kernel_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetKernelInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetKernelInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetKernelInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetKernelArgInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_uint arg_index,
cl_kernel_arg_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetKernelArgInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetKernelArgInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetKernelArgInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetKernelWorkGroupInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_device_id device,
cl_kernel_work_group_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetKernelWorkGroupInfo")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clGetKernelWorkGroupInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetKernelWorkGroupInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetKernelSubGroupInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_device_id device,
cl_kernel_sub_group_info param_name,
size_t input_value_size,
void * input_value,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret,
size_t _input_value_vals_length,
void * input_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetKernelSubGroupInfo")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clGetKernelSubGroupInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetKernelSubGroupInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clWaitForEvents_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_uint num_events,
cl_event * event_list,
size_t _event_list_vals_length,
cl_event * event_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clWaitForEvents")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clWaitForEvents_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clWaitForEvents")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetEventInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event,
cl_event_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetEventInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetEventInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetEventInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateUserEvent_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateUserEvent")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateUserEvent_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateUserEvent")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainEvent_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainEvent")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clRetainEvent_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clRetainEvent")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseEvent_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseEvent")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clReleaseEvent_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clReleaseEvent")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetUserEventStatus_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event,
cl_int execution_status
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetUserEventStatus")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetUserEventStatus_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetUserEventStatus")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetEventCallback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event,
cl_int command_exec_callback_type,
void * pfn_notify,
void * user_data
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetEventCallback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetEventCallback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetEventCallback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetEventProfilingInfo_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event,
cl_profiling_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetEventProfilingInfo")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetEventProfilingInfo_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetEventProfilingInfo")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clFlush_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clFlush")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clFlush";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clFlush_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clFlush")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clFinish_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clFinish")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clFinish";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clFinish_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clFinish")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueReadBuffer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem buffer,
cl_bool blocking_read,
size_t offset,
size_t size,
void * ptr,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReadBuffer")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReadBuffer")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueReadBuffer";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueReadBuffer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReadBuffer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueReadBufferRect_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem buffer,
cl_bool blocking_read,
size_t * buffer_offset,
size_t * host_offset,
size_t * region,
size_t buffer_row_pitch,
size_t buffer_slice_pitch,
size_t host_row_pitch,
size_t host_slice_pitch,
void * ptr,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _buffer_offset_vals_length,
size_t * buffer_offset_vals,
size_t _host_offset_vals_length,
size_t * host_offset_vals,
size_t _region_vals_length,
size_t * region_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReadBufferRect")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueReadBufferRect";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueReadBufferRect_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReadBufferRect")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueWriteBuffer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem buffer,
cl_bool blocking_write,
size_t offset,
size_t size,
void * ptr,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueWriteBuffer")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueWriteBuffer")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueWriteBuffer";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueWriteBuffer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueWriteBuffer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueWriteBufferRect_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem buffer,
cl_bool blocking_write,
size_t * buffer_offset,
size_t * host_offset,
size_t * region,
size_t buffer_row_pitch,
size_t buffer_slice_pitch,
size_t host_row_pitch,
size_t host_slice_pitch,
void * ptr,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _buffer_offset_vals_length,
size_t * buffer_offset_vals,
size_t _host_offset_vals_length,
size_t * host_offset_vals,
size_t _region_vals_length,
size_t * region_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueWriteBufferRect")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueWriteBufferRect";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueWriteBufferRect_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueWriteBufferRect")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueFillBuffer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem buffer,
void * pattern,
size_t pattern_size,
size_t offset,
size_t size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _pattern_vals_length,
void * pattern_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueFillBuffer")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueFillBuffer")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueFillBuffer";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueFillBuffer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueFillBuffer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueCopyBuffer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem src_buffer,
cl_mem dst_buffer,
size_t src_offset,
size_t dst_offset,
size_t size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyBuffer")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyBuffer")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueCopyBuffer";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueCopyBuffer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyBuffer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueCopyBufferRect_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem src_buffer,
cl_mem dst_buffer,
size_t * src_origin,
size_t * dst_origin,
size_t * region,
size_t src_row_pitch,
size_t src_slice_pitch,
size_t dst_row_pitch,
size_t dst_slice_pitch,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _src_origin_vals_length,
size_t * src_origin_vals,
size_t _dst_origin_vals_length,
size_t * dst_origin_vals,
size_t _region_vals_length,
size_t * region_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyBufferRect")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueCopyBufferRect";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueCopyBufferRect_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyBufferRect")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueReadImage_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem image,
cl_bool blocking_read,
size_t * origin,
size_t * region,
size_t row_pitch,
size_t slice_pitch,
void * ptr,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _origin_vals_length,
size_t * origin_vals,
size_t _region_vals_length,
size_t * region_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReadImage")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueReadImage";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueReadImage_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueReadImage")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueWriteImage_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem image,
cl_bool blocking_write,
size_t * origin,
size_t * region,
size_t input_row_pitch,
size_t input_slice_pitch,
void * ptr,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _origin_vals_length,
size_t * origin_vals,
size_t _region_vals_length,
size_t * region_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueWriteImage")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueWriteImage";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueWriteImage_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueWriteImage")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueFillImage_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem image,
void * fill_color,
size_t * origin,
size_t * region,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _origin_vals_length,
size_t * origin_vals,
size_t _region_vals_length,
size_t * region_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueFillImage")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueFillImage";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueFillImage_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueFillImage")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueCopyImage_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem src_image,
cl_mem dst_image,
size_t * src_origin,
size_t * dst_origin,
size_t * region,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _src_origin_vals_length,
size_t * src_origin_vals,
size_t _dst_origin_vals_length,
size_t * dst_origin_vals,
size_t _region_vals_length,
size_t * region_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyImage")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueCopyImage";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueCopyImage_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyImage")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueCopyImageToBuffer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem src_image,
cl_mem dst_buffer,
size_t * src_origin,
size_t * region,
size_t dst_offset,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyImageToBuffer")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueCopyImageToBuffer";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueCopyImageToBuffer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyImageToBuffer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueCopyBufferToImage_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem src_buffer,
cl_mem dst_image,
size_t src_offset,
size_t * dst_origin,
size_t * region,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _dst_origin_vals_length,
size_t * dst_origin_vals,
size_t _region_vals_length,
size_t * region_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyBufferToImage")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueCopyBufferToImage";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueCopyBufferToImage_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueCopyBufferToImage")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueMapBuffer_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem buffer,
cl_bool blocking_map,
cl_map_flags map_flags,
size_t offset,
size_t size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
cl_int * errcode_ret,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMapBuffer")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMapBuffer")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMapBuffer";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMapBuffer_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
void * _retval,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMapBuffer")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueMapImage_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem image,
cl_bool blocking_map,
cl_map_flags map_flags,
size_t * origin,
size_t * region,
size_t * image_row_pitch,
size_t * image_slice_pitch,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
cl_int * errcode_ret,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _origin_vals_length,
size_t * origin_vals,
size_t _region_vals_length,
size_t * region_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMapImage")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMapImage";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMapImage_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
void * _retval,
cl_int errcode_ret_val,
cl_event event_val,
size_t image_row_pitch_val,
size_t image_slice_pitch_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMapImage")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueUnmapMemObject_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_mem memobj,
void * mapped_ptr,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueUnmapMemObject")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueUnmapMemObject";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueUnmapMemObject_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueUnmapMemObject")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueMigrateMemObjects_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_mem_objects,
cl_mem * mem_objects,
cl_mem_migration_flags flags,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _mem_objects_vals_length,
cl_mem * mem_objects_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMigrateMemObjects")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMigrateMemObjects";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMigrateMemObjects_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMigrateMemObjects")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueNDRangeKernel_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_kernel kernel,
cl_uint work_dim,
size_t * global_work_offset,
size_t * global_work_size,
size_t * local_work_size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _global_work_offset_vals_length,
size_t * global_work_offset_vals,
size_t _global_work_size_vals_length,
size_t * global_work_size_vals,
size_t _local_work_size_vals_length,
size_t * local_work_size_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueNDRangeKernel")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    const thapi_command_name name  = kernel_to_name[hp_kernel_t(hostname,process_id,kernel)];
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueNDRangeKernel_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueNDRangeKernel")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueNativeKernel_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
void * user_func,
void * args,
size_t cb_args,
cl_uint num_mem_objects,
cl_mem * mem_list,
void* * args_mem_loc,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _args_vals_length,
void * args_vals,
size_t _mem_list_vals_length,
cl_mem * mem_list_vals,
size_t _args_mem_loc_vals_length,
void * args_mem_loc_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueNativeKernel")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueNativeKernel";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueNativeKernel_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueNativeKernel")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueMarkerWithWaitList_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMarkerWithWaitList")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMarkerWithWaitList";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMarkerWithWaitList_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMarkerWithWaitList")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueBarrierWithWaitList_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueBarrierWithWaitList")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueBarrierWithWaitList";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueBarrierWithWaitList_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueBarrierWithWaitList")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueSVMFree_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_svm_pointers,
void* * svm_pointers,
void * pfn_free_func,
void * user_data,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _svm_pointers_vals_length,
void * svm_pointers_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMFree")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueSVMFree";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueSVMFree_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMFree")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueSVMMemcpy_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_bool blocking_copy,
void * dst_ptr,
void * src_ptr,
size_t size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMemcpy")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMemcpy")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueSVMMemcpy";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueSVMMemcpy_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMemcpy")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueSVMMemFill_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
void * svm_ptr,
void * pattern,
size_t pattern_size,
size_t size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _pattern_vals_length,
void * pattern_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMemFill")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMemFill")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueSVMMemFill";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueSVMMemFill_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMemFill")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueSVMMap_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_bool blocking_map,
cl_map_flags flags,
void * svm_ptr,
size_t size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMap")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMap")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueSVMMap";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueSVMMap_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMap")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueSVMUnmap_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
void * svm_ptr,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMUnmap")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueSVMUnmap";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueSVMUnmap_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMUnmap")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueSVMMigrateMem_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_svm_pointers,
void* * svm_pointers,
size_t * sizes,
cl_mem_migration_flags flags,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _svm_pointers_vals_length,
void * svm_pointers_vals,
size_t _sizes_vals_length,
size_t * sizes_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMigrateMem")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueSVMMigrateMem";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueSVMMigrateMem_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMMigrateMem")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetExtensionFunctionAddressForPlatform_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_platform_id platform,
char * func_name,
char * func_name_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetExtensionFunctionAddressForPlatform")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetExtensionFunctionAddressForPlatform_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
void * _retval
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetExtensionFunctionAddressForPlatform")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetCommandQueueProperty_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_command_queue_properties properties,
cl_bool enable,
cl_command_queue_properties * old_properties
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetCommandQueueProperty")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clSetCommandQueueProperty";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clSetCommandQueueProperty_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_command_queue_properties old_properties_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetCommandQueueProperty")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateImage2D_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
cl_image_format * image_format,
size_t image_width,
size_t image_height,
size_t image_row_pitch,
void * host_ptr,
cl_int * errcode_ret,
cl_channel_order image_format__image_channel_order,
cl_channel_type image_format__image_channel_data_type
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateImage2D")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateImage2D_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateImage2D")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateImage3D_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_flags flags,
cl_image_format * image_format,
size_t image_width,
size_t image_height,
size_t image_depth,
size_t image_row_pitch,
size_t image_slice_pitch,
void * host_ptr,
cl_int * errcode_ret,
cl_channel_order image_format__image_channel_order,
cl_channel_type image_format__image_channel_data_type
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateImage3D")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateImage3D_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateImage3D")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueMarker_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_event * event
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMarker")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMarker";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMarker_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMarker")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueWaitForEvents_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_events,
cl_event * event_list,
size_t _event_list_vals_length,
cl_event * event_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueWaitForEvents")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueWaitForEvents";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueWaitForEvents_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueWaitForEvents")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueBarrier_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueBarrier")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueBarrier";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueBarrier_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueBarrier")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clUnloadCompiler_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clUnloadCompiler")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clUnloadCompiler_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clUnloadCompiler")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetExtensionFunctionAddress_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
char * func_name,
char * func_name_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetExtensionFunctionAddress")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetExtensionFunctionAddress_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
void * _retval
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetExtensionFunctionAddress")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateCommandQueue_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_device_id device,
cl_command_queue_properties properties,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateCommandQueue")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clCreateCommandQueue_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateCommandQueue")].stop(ns_from_origin);
    const cl_device_id device  = last_device[hp_t(hostname,process_id) ];
    command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)] = device ;
}
static void clprof_lttng_ust_opencl_clCreateSampler_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_bool normalized_coords,
cl_addressing_mode addressing_mode,
cl_filter_mode filter_mode,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateSampler")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateSampler_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_sampler sampler,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateSampler")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueTask_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_kernel kernel,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueTask")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    const thapi_command_name name  = kernel_to_name[hp_kernel_t(hostname,process_id,kernel)];
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueTask_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueTask")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateProgramWithILKHR_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
void * il,
size_t length,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateProgramWithILKHR")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateProgramWithILKHR_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateProgramWithILKHR")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clTerminateContextKHR_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clTerminateContextKHR")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clTerminateContextKHR_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clTerminateContextKHR")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateCommandQueueWithPropertiesKHR_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_device_id device,
cl_queue_properties_khr * properties,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_queue_properties_khr * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateCommandQueueWithPropertiesKHR")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clCreateCommandQueueWithPropertiesKHR_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateCommandQueueWithPropertiesKHR")].stop(ns_from_origin);
    const cl_device_id device  = last_device[hp_t(hostname,process_id) ];
    command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)] = device ;
}
static void clprof_lttng_ust_opencl_clEnqueueMigrateMemObjectEXT_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_mem_objects,
cl_mem * mem_objects,
cl_mem_migration_flags_ext flags,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _mem_objects_vals_length,
cl_mem * mem_objects_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMigrateMemObjectEXT")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMigrateMemObjectEXT";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMigrateMemObjectEXT_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMigrateMemObjectEXT")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetICDLoaderInfoOCLICD_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_icdl_info param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetICDLoaderInfoOCLICD")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetICDLoaderInfoOCLICD_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetICDLoaderInfoOCLICD")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateBufferWithPropertiesINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_properties_intel * properties,
size_t size,
void * host_ptr,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_mem_properties_intel * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateBufferWithPropertiesINTEL")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateBufferWithPropertiesINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateBufferWithPropertiesINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateImageWithPropertiesINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_properties_intel * properties,
cl_image_format * image_format,
cl_image_desc * image_desc,
void * host_ptr,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_mem_properties_intel * properties_vals,
cl_channel_order image_format__image_channel_order,
cl_channel_type image_format__image_channel_data_type,
cl_mem_object_type image_desc__image_type,
size_t image_desc__image_width,
size_t image_desc__image_height,
size_t image_desc__image_depth,
size_t image_desc__image_array_size,
size_t image_desc__image_row_pitch,
size_t image_desc__image_slice_pitch,
cl_uint image_desc__num_mip_levels,
cl_uint image_desc__num_samples,
cl_mem image_desc__buffer
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateImageWithPropertiesINTEL")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateImageWithPropertiesINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem mem,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateImageWithPropertiesINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetImageParamsINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_image_format * image_format,
cl_image_desc * image_desc,
size_t * image_row_pitch,
size_t * image_slice_pitch,
cl_channel_order image_format__image_channel_order,
cl_channel_type image_format__image_channel_data_type,
cl_mem_object_type image_desc__image_type,
size_t image_desc__image_width,
size_t image_desc__image_height,
size_t image_desc__image_depth,
size_t image_desc__image_array_size,
size_t image_desc__image_row_pitch,
size_t image_desc__image_slice_pitch,
cl_uint image_desc__num_mip_levels,
cl_uint image_desc__num_samples,
cl_mem image_desc__buffer
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetImageParamsINTEL")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetImageParamsINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t image_row_pitch_val,
size_t image_slice_pitch_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetImageParamsINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueVerifyMemoryINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
void * allocation_ptr,
void * expected_data,
size_t size_of_comparison,
cl_uint comparison_mode
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueVerifyMemoryINTEL")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueVerifyMemoryINTEL";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueVerifyMemoryINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueVerifyMemoryINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clAddCommentINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device,
char * comment,
char * comment_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clAddCommentINTEL")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clAddCommentINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clAddCommentINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreatePerfCountersCommandQueueINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_device_id device,
cl_command_queue_properties properties,
cl_uint configuration,
cl_int * errcode_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreatePerfCountersCommandQueueINTEL")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clCreatePerfCountersCommandQueueINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreatePerfCountersCommandQueueINTEL")].stop(ns_from_origin);
    const cl_device_id device  = last_device[hp_t(hostname,process_id) ];
    command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)] = device ;
}
static void clprof_lttng_ust_opencl_clSetPerformanceConfigurationINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device,
cl_uint count,
cl_uint * offsets,
cl_uint * values,
size_t _offsets_vals_length,
cl_uint * offsets_vals,
size_t _values_vals_length,
cl_uint * values_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetPerformanceConfigurationINTEL")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clSetPerformanceConfigurationINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t _offsets_vals_length,
cl_uint * offsets_vals,
size_t _values_vals_length,
cl_uint * values_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetPerformanceConfigurationINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetDeviceFunctionPointerINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device,
cl_program program,
char * function_name,
cl_ulong * function_pointer_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetDeviceFunctionPointerINTEL")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clGetDeviceFunctionPointerINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_ulong function_pointer_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetDeviceFunctionPointerINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetDeviceGlobalVariablePointerINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device,
cl_program program,
char * global_variable_name,
size_t * global_variable_size_ret,
void* * global_variable_pointer_ret,
char * global_variable_name_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetDeviceGlobalVariablePointerINTEL")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clGetDeviceGlobalVariablePointerINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t global_variable_size_ret_val,
void * global_variable_pointer_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetDeviceGlobalVariablePointerINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetExecutionInfoINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_kernel kernel,
cl_uint work_dim,
size_t * global_work_offset,
size_t * local_work_size,
cl_execution_info_intel param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret,
size_t _global_work_offset_vals_length,
size_t * global_work_offset_vals,
size_t _local_work_size_vals_length,
size_t * local_work_size_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetExecutionInfoINTEL")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    const thapi_command_name name  = kernel_to_name[hp_kernel_t(hostname,process_id,kernel)];
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clGetExecutionInfoINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetExecutionInfoINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueNDRangeKernelINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_kernel kernel,
cl_uint work_dim,
size_t * global_work_offset,
size_t * work_group_count,
size_t * local_work_size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _global_work_offset_vals_length,
size_t * global_work_offset_vals,
size_t _work_group_count_vals_length,
size_t * work_group_count_vals,
size_t _local_work_size_vals_length,
size_t * local_work_size_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueNDRangeKernelINTEL")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    const thapi_command_name name  = kernel_to_name[hp_kernel_t(hostname,process_id,kernel)];
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueNDRangeKernelINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueNDRangeKernelINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clHostMemAllocINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_mem_properties_intel * properties,
size_t size,
cl_uint alignment,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_mem_properties_intel * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clHostMemAllocINTEL")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clHostMemAllocINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
void * _retval,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clHostMemAllocINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clDeviceMemAllocINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_device_id device,
cl_mem_properties_intel * properties,
size_t size,
cl_uint alignment,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_mem_properties_intel * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clDeviceMemAllocINTEL")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clDeviceMemAllocINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
void * _retval,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clDeviceMemAllocINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSharedMemAllocINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
cl_device_id device,
cl_mem_properties_intel * properties,
size_t size,
cl_uint alignment,
cl_int * errcode_ret,
size_t _properties_vals_length,
cl_mem_properties_intel * properties_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSharedMemAllocINTEL")].start(ns_from_origin);
    last_device[hp_t(hostname,process_id) ] = device;
}
static void clprof_lttng_ust_opencl_clSharedMemAllocINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
void * _retval,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSharedMemAllocINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clMemFreeINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
void * ptr
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clMemFreeINTEL")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clMemFreeINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clMemFreeINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetMemAllocInfoINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_context context,
void * ptr,
cl_mem_info_intel param_name,
size_t param_value_size,
void * param_value,
size_t * param_value_size_ret
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetMemAllocInfoINTEL")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clGetMemAllocInfoINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
size_t param_value_size_ret_val,
size_t _param_value_vals_length,
void * param_value_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clGetMemAllocInfoINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetKernelArgMemPointerINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_uint arg_index,
void * arg_value
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetKernelArgMemPointerINTEL")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetKernelArgMemPointerINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetKernelArgMemPointerINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueMemsetINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
void * dst_ptr,
cl_int value,
size_t size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemsetINTEL")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemsetINTEL")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMemsetINTEL";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMemsetINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemsetINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueMemFillINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
void * dst_ptr,
void * pattern,
size_t pattern_size,
size_t size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals,
size_t _pattern_vals_length,
void * pattern_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemFillINTEL")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemFillINTEL")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMemFillINTEL";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMemFillINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemFillINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueMemcpyINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_bool blocking,
void * dst_ptr,
void * src_ptr,
size_t size,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemcpyINTEL")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemcpyINTEL")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMemcpyINTEL";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMemcpyINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemcpyINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueMigrateMemINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
void * ptr,
size_t size,
cl_mem_migration_flags_intel flags,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMigrateMemINTEL")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMigrateMemINTEL")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMigrateMemINTEL";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMigrateMemINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMigrateMemINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueMemAdviseINTEL_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
void * ptr,
size_t size,
cl_mem_advice_intel advice,
cl_uint num_events_in_wait_list,
cl_event * event_wait_list,
cl_event * event,
size_t _event_wait_list_vals_length,
cl_event * event_wait_list_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemAdviseINTEL")].start(ns_from_origin);
    memory_trafic[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemAdviseINTEL")].delta(size);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueMemAdviseINTEL";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueMemAdviseINTEL_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_int errcode_ret_val,
cl_event event_val
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueMemAdviseINTEL")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clBuildProgram_callback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
void * user_data
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clBuildProgram_callback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clBuildProgram_callback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clBuildProgram_callback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCompileProgram_callback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
void * user_data
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCompileProgram_callback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCompileProgram_callback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCompileProgram_callback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clLinkProgram_callback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
void * user_data
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clLinkProgram_callback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clLinkProgram_callback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clLinkProgram_callback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateContext_callback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
const char * errinfo,
const void * private_info,
size_t cb,
void * user_data,
const char * errinfo_val,
size_t _private_info_vals_length,
const void * private_info_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateContext_callback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateContext_callback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateContext_callback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateContextFromType_callback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
const char * errinfo,
const void * private_info,
size_t cb,
void * user_data,
const char * errinfo_val,
size_t _private_info_vals_length,
const void * private_info_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateContextFromType_callback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clCreateContextFromType_callback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clCreateContextFromType_callback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetMemObjectDestructorCallback_callback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_mem memobj,
void * user_data
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetMemObjectDestructorCallback_callback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetMemObjectDestructorCallback_callback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetMemObjectDestructorCallback_callback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetProgramReleaseCallback_callback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
void * user_data
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetProgramReleaseCallback_callback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetProgramReleaseCallback_callback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetProgramReleaseCallback_callback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetEventCallback_callback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event,
cl_int type,
void * user_data
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetEventCallback_callback")].start(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clSetEventCallback_callback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clSetEventCallback_callback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_clEnqueueSVMFree_callback_start_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_command_queue command_queue,
cl_uint num_svm_pointers,
void * * svm_pointers,
void * user_data,
size_t _svm_pointers_vals_length,
void * * svm_pointers_vals
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMFree_callback")].start(ns_from_origin);
    const cl_device_id device = command_queue_to_device[hp_command_queue_t(hostname,process_id,command_queue)];
    constexpr char name[] =  "clEnqueueSVMFree_callback";
    command_name_to_device[hpt_command_name_t(hostname,process_id, thread_id, name)] = device ;
    last_command[hpt_t(hostname,process_id,thread_id)] = name;
}
static void clprof_lttng_ust_opencl_clEnqueueSVMFree_callback_stop_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock
    ){
    const hostname_t   hostname   = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t  thread_id  = borrow_thread_id(bt_evt);
    int64_t ns_from_origin;
    bt_clock_snapshot_get_ns_from_origin(bt_clock, &ns_from_origin);
    api_call[hpt_command_name_t(hostname,process_id, thread_id, "clEnqueueSVMFree_callback")].stop(ns_from_origin);
}
static void clprof_lttng_ust_opencl_build_objects_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_device_id device,
size_t size,
char * path
    ){
}
static void clprof_lttng_ust_opencl_build_binaries_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_device_id device,
size_t size,
char * path
    ){
}
static void clprof_lttng_ust_opencl_build_infos_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_device_id device,
cl_build_status build_status,
char * build_options,
char * build_log
    ){
}
static void clprof_lttng_ust_opencl_build_infos_1_2_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_device_id device,
cl_program_binary_type binary_type
    ){
}
static void clprof_lttng_ust_opencl_build_infos_2_0_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_program program,
cl_device_id device,
size_t build_global_variable_total_size
    ){
}
static void clprof_lttng_ust_opencl_arguments_argument_info_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
cl_uint arg_index,
cl_kernel_arg_address_qualifier address_qualifier,
cl_kernel_arg_access_qualifier access_qualifier,
char * type_name,
cl_kernel_arg_type_qualifier type_qualifier,
char * name
    ){
}
static void clprof_lttng_ust_opencl_arguments_kernel_info_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_kernel kernel,
char * function_name,
cl_uint num_args,
cl_context context,
cl_program program,
char * attibutes
    ){
    const std::string hostname    = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    kernel_to_name[hp_kernel_t(hostname,process_id,kernel)] = std::string{function_name};
}
static void clprof_lttng_ust_opencl_dump_enqueue_counter_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
uint64_t enqueue_counter
    ){
}
static void clprof_lttng_ust_opencl_dump_kernel_arg_value_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
uint64_t enqueue_counter,
cl_uint arg_index,
size_t arg_size,
size_t _arg_value_length,
void * arg_value
    ){
}
static void clprof_lttng_ust_opencl_dump_svmptr_dump_event_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
uint64_t enqueue_counter,
cl_uint arg_index,
int direction,
void * buffer,
int status,
cl_event event
    ){
}
static void clprof_lttng_ust_opencl_dump_svmptr_dump_result_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
uint64_t enqueue_counter,
cl_uint arg_index,
int direction,
cl_event event,
cl_int status,
size_t size,
char * path
    ){
}
static void clprof_lttng_ust_opencl_dump_buffer_dump_event_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
uint64_t enqueue_counter,
cl_uint arg_index,
int direction,
cl_mem buffer,
int status,
cl_event event
    ){
}
static void clprof_lttng_ust_opencl_dump_buffer_dump_result_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
uint64_t enqueue_counter,
cl_uint arg_index,
int direction,
cl_event event,
cl_int status,
size_t size,
char * path
    ){
}
static void clprof_lttng_ust_opencl_profiling_event_profiling_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
int status,
cl_event event
    ){
    const hostname_t hostname    = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const thread_id_t thread_id   = borrow_thread_id(bt_evt);
    const hp_event_t hp_event{hostname,process_id, event};
    const thapi_command_name command_name = last_command[hpt_t(hostname,process_id,thread_id)];
    if (!event_to_command_name.count(hp_event)){
        event_to_command_name[hp_event] = t_command_name_t(thread_id, command_name);
    } else {
        const uint64_t delta = event_result_to_delta[hp_event];
        const cl_device_id device = command_name_to_device[hpt_command_name_t(hostname,process_id,thread_id, command_name)];
        device_id_result[hpt_device_command_name_t(hostname,process_id,thread_id,device,command_name)].delta(delta);
        event_to_command_name.erase(hp_event);
    }
}
static void clprof_lttng_ust_opencl_profiling_event_profiling_results_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_event event,
cl_int event_command_exec_status,
cl_int queued_status,
cl_ulong queued,
cl_int submit_status,
cl_ulong submit,
cl_int start_status,
cl_ulong start,
cl_int end_status,
cl_ulong end
    ){
    const std::string hostname    = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    const hp_event_t hp_event{hostname,process_id, event};
    const uint64_t delta = end - start;
    if (event_to_command_name.count(hp_event)) {
        const auto [thread_id,command_name] =  event_to_command_name[hp_event];
        const cl_device_id device = command_name_to_device[hpt_command_name_t(hostname,process_id,thread_id, command_name)];
        device_id_result[hpt_device_command_name_t(hostname,process_id,thread_id,device,command_name)].delta(delta);
        event_to_command_name.erase(hp_event);
    } else {
        const thread_id_t thread_id   = borrow_thread_id(bt_evt);
        const thapi_command_name command_name = last_command[hpt_t(hostname,process_id,thread_id)];
        event_result_to_delta[hp_event]= delta;
        event_to_command_name[hp_event] = t_command_name_t(thread_id, command_name);
    }
}
static void clprof_lttng_ust_opencl_source_program_string_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
int index,
size_t length,
char * path
    ){
}
static void clprof_lttng_ust_opencl_source_program_binary_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
int index,
size_t length,
char * path
    ){
}
static void clprof_lttng_ust_opencl_source_program_il_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
size_t length,
char * path
    ){
}
static void clprof_lttng_ust_opencl_devices_device_name_callback(
const bt_event *bt_evt,
const bt_clock_snapshot *bt_clock,
cl_device_id device,
char * name
    ){
    const std::string hostname    = borrow_hostname(bt_evt);
    const process_id_t process_id = borrow_process_id(bt_evt);
    device_to_name[hp_device_t(hostname,process_id,device)] = std::string{name};
}
void init_callbacks(struct opencl_dispatch   *opencl_dispatch) {
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateEventFromEGLSyncKHR_start", (void *) &clprof_lttng_ust_opencl_clCreateEventFromEGLSyncKHR_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateEventFromEGLSyncKHR_stop", (void *) &clprof_lttng_ust_opencl_clCreateEventFromEGLSyncKHR_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromEGLImageKHR_start", (void *) &clprof_lttng_ust_opencl_clCreateFromEGLImageKHR_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromEGLImageKHR_stop", (void *) &clprof_lttng_ust_opencl_clCreateFromEGLImageKHR_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueAcquireEGLObjectsKHR_start", (void *) &clprof_lttng_ust_opencl_clEnqueueAcquireEGLObjectsKHR_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueAcquireEGLObjectsKHR_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueAcquireEGLObjectsKHR_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueReleaseEGLObjectsKHR_start", (void *) &clprof_lttng_ust_opencl_clEnqueueReleaseEGLObjectsKHR_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueReleaseEGLObjectsKHR_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueReleaseEGLObjectsKHR_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseDeviceEXT_start", (void *) &clprof_lttng_ust_opencl_clReleaseDeviceEXT_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseDeviceEXT_stop", (void *) &clprof_lttng_ust_opencl_clReleaseDeviceEXT_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainDeviceEXT_start", (void *) &clprof_lttng_ust_opencl_clRetainDeviceEXT_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainDeviceEXT_stop", (void *) &clprof_lttng_ust_opencl_clRetainDeviceEXT_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateSubDevicesEXT_start", (void *) &clprof_lttng_ust_opencl_clCreateSubDevicesEXT_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateSubDevicesEXT_stop", (void *) &clprof_lttng_ust_opencl_clCreateSubDevicesEXT_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetKernelSubGroupInfoKHR_start", (void *) &clprof_lttng_ust_opencl_clGetKernelSubGroupInfoKHR_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetKernelSubGroupInfoKHR_stop", (void *) &clprof_lttng_ust_opencl_clGetKernelSubGroupInfoKHR_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateEventFromGLsyncKHR_start", (void *) &clprof_lttng_ust_opencl_clCreateEventFromGLsyncKHR_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateEventFromGLsyncKHR_stop", (void *) &clprof_lttng_ust_opencl_clCreateEventFromGLsyncKHR_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetGLContextInfoKHR_start", (void *) &clprof_lttng_ust_opencl_clGetGLContextInfoKHR_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetGLContextInfoKHR_stop", (void *) &clprof_lttng_ust_opencl_clGetGLContextInfoKHR_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromGLBuffer_start", (void *) &clprof_lttng_ust_opencl_clCreateFromGLBuffer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromGLBuffer_stop", (void *) &clprof_lttng_ust_opencl_clCreateFromGLBuffer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromGLTexture_start", (void *) &clprof_lttng_ust_opencl_clCreateFromGLTexture_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromGLTexture_stop", (void *) &clprof_lttng_ust_opencl_clCreateFromGLTexture_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromGLRenderbuffer_start", (void *) &clprof_lttng_ust_opencl_clCreateFromGLRenderbuffer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromGLRenderbuffer_stop", (void *) &clprof_lttng_ust_opencl_clCreateFromGLRenderbuffer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetGLObjectInfo_start", (void *) &clprof_lttng_ust_opencl_clGetGLObjectInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetGLObjectInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetGLObjectInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetGLTextureInfo_start", (void *) &clprof_lttng_ust_opencl_clGetGLTextureInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetGLTextureInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetGLTextureInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueAcquireGLObjects_start", (void *) &clprof_lttng_ust_opencl_clEnqueueAcquireGLObjects_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueAcquireGLObjects_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueAcquireGLObjects_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueReleaseGLObjects_start", (void *) &clprof_lttng_ust_opencl_clEnqueueReleaseGLObjects_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueReleaseGLObjects_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueReleaseGLObjects_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromGLTexture2D_start", (void *) &clprof_lttng_ust_opencl_clCreateFromGLTexture2D_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromGLTexture2D_stop", (void *) &clprof_lttng_ust_opencl_clCreateFromGLTexture2D_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromGLTexture3D_start", (void *) &clprof_lttng_ust_opencl_clCreateFromGLTexture3D_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateFromGLTexture3D_stop", (void *) &clprof_lttng_ust_opencl_clCreateFromGLTexture3D_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetPlatformIDs_start", (void *) &clprof_lttng_ust_opencl_clGetPlatformIDs_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetPlatformIDs_stop", (void *) &clprof_lttng_ust_opencl_clGetPlatformIDs_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetPlatformInfo_start", (void *) &clprof_lttng_ust_opencl_clGetPlatformInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetPlatformInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetPlatformInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetDeviceIDs_start", (void *) &clprof_lttng_ust_opencl_clGetDeviceIDs_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetDeviceIDs_stop", (void *) &clprof_lttng_ust_opencl_clGetDeviceIDs_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetDeviceInfo_start", (void *) &clprof_lttng_ust_opencl_clGetDeviceInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetDeviceInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetDeviceInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateSubDevices_start", (void *) &clprof_lttng_ust_opencl_clCreateSubDevices_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateSubDevices_stop", (void *) &clprof_lttng_ust_opencl_clCreateSubDevices_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainDevice_start", (void *) &clprof_lttng_ust_opencl_clRetainDevice_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainDevice_stop", (void *) &clprof_lttng_ust_opencl_clRetainDevice_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseDevice_start", (void *) &clprof_lttng_ust_opencl_clReleaseDevice_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseDevice_stop", (void *) &clprof_lttng_ust_opencl_clReleaseDevice_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetDefaultDeviceCommandQueue_start", (void *) &clprof_lttng_ust_opencl_clSetDefaultDeviceCommandQueue_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetDefaultDeviceCommandQueue_stop", (void *) &clprof_lttng_ust_opencl_clSetDefaultDeviceCommandQueue_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetDeviceAndHostTimer_start", (void *) &clprof_lttng_ust_opencl_clGetDeviceAndHostTimer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetDeviceAndHostTimer_stop", (void *) &clprof_lttng_ust_opencl_clGetDeviceAndHostTimer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetHostTimer_start", (void *) &clprof_lttng_ust_opencl_clGetHostTimer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetHostTimer_stop", (void *) &clprof_lttng_ust_opencl_clGetHostTimer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateContext_start", (void *) &clprof_lttng_ust_opencl_clCreateContext_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateContext_stop", (void *) &clprof_lttng_ust_opencl_clCreateContext_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateContextFromType_start", (void *) &clprof_lttng_ust_opencl_clCreateContextFromType_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateContextFromType_stop", (void *) &clprof_lttng_ust_opencl_clCreateContextFromType_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainContext_start", (void *) &clprof_lttng_ust_opencl_clRetainContext_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainContext_stop", (void *) &clprof_lttng_ust_opencl_clRetainContext_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseContext_start", (void *) &clprof_lttng_ust_opencl_clReleaseContext_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseContext_stop", (void *) &clprof_lttng_ust_opencl_clReleaseContext_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetContextInfo_start", (void *) &clprof_lttng_ust_opencl_clGetContextInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetContextInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetContextInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateCommandQueueWithProperties_start", (void *) &clprof_lttng_ust_opencl_clCreateCommandQueueWithProperties_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateCommandQueueWithProperties_stop", (void *) &clprof_lttng_ust_opencl_clCreateCommandQueueWithProperties_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainCommandQueue_start", (void *) &clprof_lttng_ust_opencl_clRetainCommandQueue_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainCommandQueue_stop", (void *) &clprof_lttng_ust_opencl_clRetainCommandQueue_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseCommandQueue_start", (void *) &clprof_lttng_ust_opencl_clReleaseCommandQueue_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseCommandQueue_stop", (void *) &clprof_lttng_ust_opencl_clReleaseCommandQueue_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetCommandQueueInfo_start", (void *) &clprof_lttng_ust_opencl_clGetCommandQueueInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetCommandQueueInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetCommandQueueInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateBuffer_start", (void *) &clprof_lttng_ust_opencl_clCreateBuffer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateBuffer_stop", (void *) &clprof_lttng_ust_opencl_clCreateBuffer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateBufferWithProperties_start", (void *) &clprof_lttng_ust_opencl_clCreateBufferWithProperties_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateBufferWithProperties_stop", (void *) &clprof_lttng_ust_opencl_clCreateBufferWithProperties_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateSubBuffer_start", (void *) &clprof_lttng_ust_opencl_clCreateSubBuffer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateSubBuffer_stop", (void *) &clprof_lttng_ust_opencl_clCreateSubBuffer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateImage_start", (void *) &clprof_lttng_ust_opencl_clCreateImage_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateImage_stop", (void *) &clprof_lttng_ust_opencl_clCreateImage_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateImageWithProperties_start", (void *) &clprof_lttng_ust_opencl_clCreateImageWithProperties_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateImageWithProperties_stop", (void *) &clprof_lttng_ust_opencl_clCreateImageWithProperties_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreatePipe_start", (void *) &clprof_lttng_ust_opencl_clCreatePipe_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreatePipe_stop", (void *) &clprof_lttng_ust_opencl_clCreatePipe_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainMemObject_start", (void *) &clprof_lttng_ust_opencl_clRetainMemObject_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainMemObject_stop", (void *) &clprof_lttng_ust_opencl_clRetainMemObject_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseMemObject_start", (void *) &clprof_lttng_ust_opencl_clReleaseMemObject_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseMemObject_stop", (void *) &clprof_lttng_ust_opencl_clReleaseMemObject_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetSupportedImageFormats_start", (void *) &clprof_lttng_ust_opencl_clGetSupportedImageFormats_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetSupportedImageFormats_stop", (void *) &clprof_lttng_ust_opencl_clGetSupportedImageFormats_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetMemObjectInfo_start", (void *) &clprof_lttng_ust_opencl_clGetMemObjectInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetMemObjectInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetMemObjectInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetImageInfo_start", (void *) &clprof_lttng_ust_opencl_clGetImageInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetImageInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetImageInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetPipeInfo_start", (void *) &clprof_lttng_ust_opencl_clGetPipeInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetPipeInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetPipeInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetMemObjectDestructorCallback_start", (void *) &clprof_lttng_ust_opencl_clSetMemObjectDestructorCallback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetMemObjectDestructorCallback_stop", (void *) &clprof_lttng_ust_opencl_clSetMemObjectDestructorCallback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSVMAlloc_start", (void *) &clprof_lttng_ust_opencl_clSVMAlloc_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSVMAlloc_stop", (void *) &clprof_lttng_ust_opencl_clSVMAlloc_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSVMFree_start", (void *) &clprof_lttng_ust_opencl_clSVMFree_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSVMFree_stop", (void *) &clprof_lttng_ust_opencl_clSVMFree_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateSamplerWithProperties_start", (void *) &clprof_lttng_ust_opencl_clCreateSamplerWithProperties_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateSamplerWithProperties_stop", (void *) &clprof_lttng_ust_opencl_clCreateSamplerWithProperties_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainSampler_start", (void *) &clprof_lttng_ust_opencl_clRetainSampler_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainSampler_stop", (void *) &clprof_lttng_ust_opencl_clRetainSampler_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseSampler_start", (void *) &clprof_lttng_ust_opencl_clReleaseSampler_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseSampler_stop", (void *) &clprof_lttng_ust_opencl_clReleaseSampler_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetSamplerInfo_start", (void *) &clprof_lttng_ust_opencl_clGetSamplerInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetSamplerInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetSamplerInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateProgramWithSource_start", (void *) &clprof_lttng_ust_opencl_clCreateProgramWithSource_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateProgramWithSource_stop", (void *) &clprof_lttng_ust_opencl_clCreateProgramWithSource_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateProgramWithBinary_start", (void *) &clprof_lttng_ust_opencl_clCreateProgramWithBinary_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateProgramWithBinary_stop", (void *) &clprof_lttng_ust_opencl_clCreateProgramWithBinary_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateProgramWithBuiltInKernels_start", (void *) &clprof_lttng_ust_opencl_clCreateProgramWithBuiltInKernels_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateProgramWithBuiltInKernels_stop", (void *) &clprof_lttng_ust_opencl_clCreateProgramWithBuiltInKernels_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateProgramWithIL_start", (void *) &clprof_lttng_ust_opencl_clCreateProgramWithIL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateProgramWithIL_stop", (void *) &clprof_lttng_ust_opencl_clCreateProgramWithIL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainProgram_start", (void *) &clprof_lttng_ust_opencl_clRetainProgram_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainProgram_stop", (void *) &clprof_lttng_ust_opencl_clRetainProgram_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseProgram_start", (void *) &clprof_lttng_ust_opencl_clReleaseProgram_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseProgram_stop", (void *) &clprof_lttng_ust_opencl_clReleaseProgram_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clBuildProgram_start", (void *) &clprof_lttng_ust_opencl_clBuildProgram_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clBuildProgram_stop", (void *) &clprof_lttng_ust_opencl_clBuildProgram_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCompileProgram_start", (void *) &clprof_lttng_ust_opencl_clCompileProgram_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCompileProgram_stop", (void *) &clprof_lttng_ust_opencl_clCompileProgram_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clLinkProgram_start", (void *) &clprof_lttng_ust_opencl_clLinkProgram_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clLinkProgram_stop", (void *) &clprof_lttng_ust_opencl_clLinkProgram_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetProgramReleaseCallback_start", (void *) &clprof_lttng_ust_opencl_clSetProgramReleaseCallback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetProgramReleaseCallback_stop", (void *) &clprof_lttng_ust_opencl_clSetProgramReleaseCallback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetProgramSpecializationConstant_start", (void *) &clprof_lttng_ust_opencl_clSetProgramSpecializationConstant_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetProgramSpecializationConstant_stop", (void *) &clprof_lttng_ust_opencl_clSetProgramSpecializationConstant_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clUnloadPlatformCompiler_start", (void *) &clprof_lttng_ust_opencl_clUnloadPlatformCompiler_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clUnloadPlatformCompiler_stop", (void *) &clprof_lttng_ust_opencl_clUnloadPlatformCompiler_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetProgramInfo_start", (void *) &clprof_lttng_ust_opencl_clGetProgramInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetProgramInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetProgramInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetProgramBuildInfo_start", (void *) &clprof_lttng_ust_opencl_clGetProgramBuildInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetProgramBuildInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetProgramBuildInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateKernel_start", (void *) &clprof_lttng_ust_opencl_clCreateKernel_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateKernel_stop", (void *) &clprof_lttng_ust_opencl_clCreateKernel_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateKernelsInProgram_start", (void *) &clprof_lttng_ust_opencl_clCreateKernelsInProgram_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateKernelsInProgram_stop", (void *) &clprof_lttng_ust_opencl_clCreateKernelsInProgram_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCloneKernel_start", (void *) &clprof_lttng_ust_opencl_clCloneKernel_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCloneKernel_stop", (void *) &clprof_lttng_ust_opencl_clCloneKernel_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainKernel_start", (void *) &clprof_lttng_ust_opencl_clRetainKernel_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainKernel_stop", (void *) &clprof_lttng_ust_opencl_clRetainKernel_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseKernel_start", (void *) &clprof_lttng_ust_opencl_clReleaseKernel_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseKernel_stop", (void *) &clprof_lttng_ust_opencl_clReleaseKernel_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetKernelArg_start", (void *) &clprof_lttng_ust_opencl_clSetKernelArg_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetKernelArg_stop", (void *) &clprof_lttng_ust_opencl_clSetKernelArg_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetKernelArgSVMPointer_start", (void *) &clprof_lttng_ust_opencl_clSetKernelArgSVMPointer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetKernelArgSVMPointer_stop", (void *) &clprof_lttng_ust_opencl_clSetKernelArgSVMPointer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetKernelExecInfo_start", (void *) &clprof_lttng_ust_opencl_clSetKernelExecInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetKernelExecInfo_stop", (void *) &clprof_lttng_ust_opencl_clSetKernelExecInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetKernelInfo_start", (void *) &clprof_lttng_ust_opencl_clGetKernelInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetKernelInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetKernelInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetKernelArgInfo_start", (void *) &clprof_lttng_ust_opencl_clGetKernelArgInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetKernelArgInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetKernelArgInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetKernelWorkGroupInfo_start", (void *) &clprof_lttng_ust_opencl_clGetKernelWorkGroupInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetKernelWorkGroupInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetKernelWorkGroupInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetKernelSubGroupInfo_start", (void *) &clprof_lttng_ust_opencl_clGetKernelSubGroupInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetKernelSubGroupInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetKernelSubGroupInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clWaitForEvents_start", (void *) &clprof_lttng_ust_opencl_clWaitForEvents_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clWaitForEvents_stop", (void *) &clprof_lttng_ust_opencl_clWaitForEvents_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetEventInfo_start", (void *) &clprof_lttng_ust_opencl_clGetEventInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetEventInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetEventInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateUserEvent_start", (void *) &clprof_lttng_ust_opencl_clCreateUserEvent_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateUserEvent_stop", (void *) &clprof_lttng_ust_opencl_clCreateUserEvent_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainEvent_start", (void *) &clprof_lttng_ust_opencl_clRetainEvent_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clRetainEvent_stop", (void *) &clprof_lttng_ust_opencl_clRetainEvent_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseEvent_start", (void *) &clprof_lttng_ust_opencl_clReleaseEvent_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clReleaseEvent_stop", (void *) &clprof_lttng_ust_opencl_clReleaseEvent_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetUserEventStatus_start", (void *) &clprof_lttng_ust_opencl_clSetUserEventStatus_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetUserEventStatus_stop", (void *) &clprof_lttng_ust_opencl_clSetUserEventStatus_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetEventCallback_start", (void *) &clprof_lttng_ust_opencl_clSetEventCallback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetEventCallback_stop", (void *) &clprof_lttng_ust_opencl_clSetEventCallback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetEventProfilingInfo_start", (void *) &clprof_lttng_ust_opencl_clGetEventProfilingInfo_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetEventProfilingInfo_stop", (void *) &clprof_lttng_ust_opencl_clGetEventProfilingInfo_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clFlush_start", (void *) &clprof_lttng_ust_opencl_clFlush_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clFlush_stop", (void *) &clprof_lttng_ust_opencl_clFlush_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clFinish_start", (void *) &clprof_lttng_ust_opencl_clFinish_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clFinish_stop", (void *) &clprof_lttng_ust_opencl_clFinish_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueReadBuffer_start", (void *) &clprof_lttng_ust_opencl_clEnqueueReadBuffer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueReadBuffer_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueReadBuffer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueReadBufferRect_start", (void *) &clprof_lttng_ust_opencl_clEnqueueReadBufferRect_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueReadBufferRect_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueReadBufferRect_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueWriteBuffer_start", (void *) &clprof_lttng_ust_opencl_clEnqueueWriteBuffer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueWriteBuffer_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueWriteBuffer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueWriteBufferRect_start", (void *) &clprof_lttng_ust_opencl_clEnqueueWriteBufferRect_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueWriteBufferRect_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueWriteBufferRect_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueFillBuffer_start", (void *) &clprof_lttng_ust_opencl_clEnqueueFillBuffer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueFillBuffer_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueFillBuffer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueCopyBuffer_start", (void *) &clprof_lttng_ust_opencl_clEnqueueCopyBuffer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueCopyBuffer_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueCopyBuffer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueCopyBufferRect_start", (void *) &clprof_lttng_ust_opencl_clEnqueueCopyBufferRect_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueCopyBufferRect_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueCopyBufferRect_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueReadImage_start", (void *) &clprof_lttng_ust_opencl_clEnqueueReadImage_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueReadImage_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueReadImage_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueWriteImage_start", (void *) &clprof_lttng_ust_opencl_clEnqueueWriteImage_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueWriteImage_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueWriteImage_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueFillImage_start", (void *) &clprof_lttng_ust_opencl_clEnqueueFillImage_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueFillImage_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueFillImage_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueCopyImage_start", (void *) &clprof_lttng_ust_opencl_clEnqueueCopyImage_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueCopyImage_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueCopyImage_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueCopyImageToBuffer_start", (void *) &clprof_lttng_ust_opencl_clEnqueueCopyImageToBuffer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueCopyImageToBuffer_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueCopyImageToBuffer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueCopyBufferToImage_start", (void *) &clprof_lttng_ust_opencl_clEnqueueCopyBufferToImage_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueCopyBufferToImage_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueCopyBufferToImage_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMapBuffer_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMapBuffer_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMapBuffer_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMapBuffer_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMapImage_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMapImage_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMapImage_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMapImage_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueUnmapMemObject_start", (void *) &clprof_lttng_ust_opencl_clEnqueueUnmapMemObject_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueUnmapMemObject_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueUnmapMemObject_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMigrateMemObjects_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMigrateMemObjects_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMigrateMemObjects_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMigrateMemObjects_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueNDRangeKernel_start", (void *) &clprof_lttng_ust_opencl_clEnqueueNDRangeKernel_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueNDRangeKernel_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueNDRangeKernel_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueNativeKernel_start", (void *) &clprof_lttng_ust_opencl_clEnqueueNativeKernel_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueNativeKernel_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueNativeKernel_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMarkerWithWaitList_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMarkerWithWaitList_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMarkerWithWaitList_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMarkerWithWaitList_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueBarrierWithWaitList_start", (void *) &clprof_lttng_ust_opencl_clEnqueueBarrierWithWaitList_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueBarrierWithWaitList_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueBarrierWithWaitList_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMFree_start", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMFree_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMFree_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMFree_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMMemcpy_start", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMMemcpy_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMMemcpy_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMMemcpy_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMMemFill_start", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMMemFill_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMMemFill_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMMemFill_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMMap_start", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMMap_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMMap_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMMap_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMUnmap_start", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMUnmap_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMUnmap_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMUnmap_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMMigrateMem_start", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMMigrateMem_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMMigrateMem_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMMigrateMem_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetExtensionFunctionAddressForPlatform_start", (void *) &clprof_lttng_ust_opencl_clGetExtensionFunctionAddressForPlatform_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetExtensionFunctionAddressForPlatform_stop", (void *) &clprof_lttng_ust_opencl_clGetExtensionFunctionAddressForPlatform_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetCommandQueueProperty_start", (void *) &clprof_lttng_ust_opencl_clSetCommandQueueProperty_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetCommandQueueProperty_stop", (void *) &clprof_lttng_ust_opencl_clSetCommandQueueProperty_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateImage2D_start", (void *) &clprof_lttng_ust_opencl_clCreateImage2D_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateImage2D_stop", (void *) &clprof_lttng_ust_opencl_clCreateImage2D_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateImage3D_start", (void *) &clprof_lttng_ust_opencl_clCreateImage3D_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateImage3D_stop", (void *) &clprof_lttng_ust_opencl_clCreateImage3D_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMarker_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMarker_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMarker_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMarker_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueWaitForEvents_start", (void *) &clprof_lttng_ust_opencl_clEnqueueWaitForEvents_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueWaitForEvents_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueWaitForEvents_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueBarrier_start", (void *) &clprof_lttng_ust_opencl_clEnqueueBarrier_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueBarrier_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueBarrier_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clUnloadCompiler_start", (void *) &clprof_lttng_ust_opencl_clUnloadCompiler_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clUnloadCompiler_stop", (void *) &clprof_lttng_ust_opencl_clUnloadCompiler_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetExtensionFunctionAddress_start", (void *) &clprof_lttng_ust_opencl_clGetExtensionFunctionAddress_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetExtensionFunctionAddress_stop", (void *) &clprof_lttng_ust_opencl_clGetExtensionFunctionAddress_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateCommandQueue_start", (void *) &clprof_lttng_ust_opencl_clCreateCommandQueue_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateCommandQueue_stop", (void *) &clprof_lttng_ust_opencl_clCreateCommandQueue_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateSampler_start", (void *) &clprof_lttng_ust_opencl_clCreateSampler_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateSampler_stop", (void *) &clprof_lttng_ust_opencl_clCreateSampler_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueTask_start", (void *) &clprof_lttng_ust_opencl_clEnqueueTask_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueTask_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueTask_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateProgramWithILKHR_start", (void *) &clprof_lttng_ust_opencl_clCreateProgramWithILKHR_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateProgramWithILKHR_stop", (void *) &clprof_lttng_ust_opencl_clCreateProgramWithILKHR_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clTerminateContextKHR_start", (void *) &clprof_lttng_ust_opencl_clTerminateContextKHR_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clTerminateContextKHR_stop", (void *) &clprof_lttng_ust_opencl_clTerminateContextKHR_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateCommandQueueWithPropertiesKHR_start", (void *) &clprof_lttng_ust_opencl_clCreateCommandQueueWithPropertiesKHR_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateCommandQueueWithPropertiesKHR_stop", (void *) &clprof_lttng_ust_opencl_clCreateCommandQueueWithPropertiesKHR_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMigrateMemObjectEXT_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMigrateMemObjectEXT_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMigrateMemObjectEXT_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMigrateMemObjectEXT_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetICDLoaderInfoOCLICD_start", (void *) &clprof_lttng_ust_opencl_clGetICDLoaderInfoOCLICD_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetICDLoaderInfoOCLICD_stop", (void *) &clprof_lttng_ust_opencl_clGetICDLoaderInfoOCLICD_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateBufferWithPropertiesINTEL_start", (void *) &clprof_lttng_ust_opencl_clCreateBufferWithPropertiesINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateBufferWithPropertiesINTEL_stop", (void *) &clprof_lttng_ust_opencl_clCreateBufferWithPropertiesINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateImageWithPropertiesINTEL_start", (void *) &clprof_lttng_ust_opencl_clCreateImageWithPropertiesINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateImageWithPropertiesINTEL_stop", (void *) &clprof_lttng_ust_opencl_clCreateImageWithPropertiesINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetImageParamsINTEL_start", (void *) &clprof_lttng_ust_opencl_clGetImageParamsINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetImageParamsINTEL_stop", (void *) &clprof_lttng_ust_opencl_clGetImageParamsINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueVerifyMemoryINTEL_start", (void *) &clprof_lttng_ust_opencl_clEnqueueVerifyMemoryINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueVerifyMemoryINTEL_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueVerifyMemoryINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clAddCommentINTEL_start", (void *) &clprof_lttng_ust_opencl_clAddCommentINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clAddCommentINTEL_stop", (void *) &clprof_lttng_ust_opencl_clAddCommentINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreatePerfCountersCommandQueueINTEL_start", (void *) &clprof_lttng_ust_opencl_clCreatePerfCountersCommandQueueINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreatePerfCountersCommandQueueINTEL_stop", (void *) &clprof_lttng_ust_opencl_clCreatePerfCountersCommandQueueINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetPerformanceConfigurationINTEL_start", (void *) &clprof_lttng_ust_opencl_clSetPerformanceConfigurationINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetPerformanceConfigurationINTEL_stop", (void *) &clprof_lttng_ust_opencl_clSetPerformanceConfigurationINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetDeviceFunctionPointerINTEL_start", (void *) &clprof_lttng_ust_opencl_clGetDeviceFunctionPointerINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetDeviceFunctionPointerINTEL_stop", (void *) &clprof_lttng_ust_opencl_clGetDeviceFunctionPointerINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetDeviceGlobalVariablePointerINTEL_start", (void *) &clprof_lttng_ust_opencl_clGetDeviceGlobalVariablePointerINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetDeviceGlobalVariablePointerINTEL_stop", (void *) &clprof_lttng_ust_opencl_clGetDeviceGlobalVariablePointerINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetExecutionInfoINTEL_start", (void *) &clprof_lttng_ust_opencl_clGetExecutionInfoINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetExecutionInfoINTEL_stop", (void *) &clprof_lttng_ust_opencl_clGetExecutionInfoINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueNDRangeKernelINTEL_start", (void *) &clprof_lttng_ust_opencl_clEnqueueNDRangeKernelINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueNDRangeKernelINTEL_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueNDRangeKernelINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clHostMemAllocINTEL_start", (void *) &clprof_lttng_ust_opencl_clHostMemAllocINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clHostMemAllocINTEL_stop", (void *) &clprof_lttng_ust_opencl_clHostMemAllocINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clDeviceMemAllocINTEL_start", (void *) &clprof_lttng_ust_opencl_clDeviceMemAllocINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clDeviceMemAllocINTEL_stop", (void *) &clprof_lttng_ust_opencl_clDeviceMemAllocINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSharedMemAllocINTEL_start", (void *) &clprof_lttng_ust_opencl_clSharedMemAllocINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSharedMemAllocINTEL_stop", (void *) &clprof_lttng_ust_opencl_clSharedMemAllocINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clMemFreeINTEL_start", (void *) &clprof_lttng_ust_opencl_clMemFreeINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clMemFreeINTEL_stop", (void *) &clprof_lttng_ust_opencl_clMemFreeINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetMemAllocInfoINTEL_start", (void *) &clprof_lttng_ust_opencl_clGetMemAllocInfoINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clGetMemAllocInfoINTEL_stop", (void *) &clprof_lttng_ust_opencl_clGetMemAllocInfoINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetKernelArgMemPointerINTEL_start", (void *) &clprof_lttng_ust_opencl_clSetKernelArgMemPointerINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetKernelArgMemPointerINTEL_stop", (void *) &clprof_lttng_ust_opencl_clSetKernelArgMemPointerINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMemsetINTEL_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMemsetINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMemsetINTEL_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMemsetINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMemFillINTEL_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMemFillINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMemFillINTEL_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMemFillINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMemcpyINTEL_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMemcpyINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMemcpyINTEL_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMemcpyINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMigrateMemINTEL_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMigrateMemINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMigrateMemINTEL_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMigrateMemINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMemAdviseINTEL_start", (void *) &clprof_lttng_ust_opencl_clEnqueueMemAdviseINTEL_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueMemAdviseINTEL_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueMemAdviseINTEL_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clBuildProgram_callback_start", (void *) &clprof_lttng_ust_opencl_clBuildProgram_callback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clBuildProgram_callback_stop", (void *) &clprof_lttng_ust_opencl_clBuildProgram_callback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCompileProgram_callback_start", (void *) &clprof_lttng_ust_opencl_clCompileProgram_callback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCompileProgram_callback_stop", (void *) &clprof_lttng_ust_opencl_clCompileProgram_callback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clLinkProgram_callback_start", (void *) &clprof_lttng_ust_opencl_clLinkProgram_callback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clLinkProgram_callback_stop", (void *) &clprof_lttng_ust_opencl_clLinkProgram_callback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateContext_callback_start", (void *) &clprof_lttng_ust_opencl_clCreateContext_callback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateContext_callback_stop", (void *) &clprof_lttng_ust_opencl_clCreateContext_callback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateContextFromType_callback_start", (void *) &clprof_lttng_ust_opencl_clCreateContextFromType_callback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clCreateContextFromType_callback_stop", (void *) &clprof_lttng_ust_opencl_clCreateContextFromType_callback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetMemObjectDestructorCallback_callback_start", (void *) &clprof_lttng_ust_opencl_clSetMemObjectDestructorCallback_callback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetMemObjectDestructorCallback_callback_stop", (void *) &clprof_lttng_ust_opencl_clSetMemObjectDestructorCallback_callback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetProgramReleaseCallback_callback_start", (void *) &clprof_lttng_ust_opencl_clSetProgramReleaseCallback_callback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetProgramReleaseCallback_callback_stop", (void *) &clprof_lttng_ust_opencl_clSetProgramReleaseCallback_callback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetEventCallback_callback_start", (void *) &clprof_lttng_ust_opencl_clSetEventCallback_callback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clSetEventCallback_callback_stop", (void *) &clprof_lttng_ust_opencl_clSetEventCallback_callback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMFree_callback_start", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMFree_callback_start_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl:clEnqueueSVMFree_callback_stop", (void *) &clprof_lttng_ust_opencl_clEnqueueSVMFree_callback_stop_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_build:objects", (void *) &clprof_lttng_ust_opencl_build_objects_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_build:binaries", (void *) &clprof_lttng_ust_opencl_build_binaries_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_build:infos", (void *) &clprof_lttng_ust_opencl_build_infos_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_build:infos_1_2", (void *) &clprof_lttng_ust_opencl_build_infos_1_2_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_build:infos_2_0", (void *) &clprof_lttng_ust_opencl_build_infos_2_0_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_arguments:argument_info", (void *) &clprof_lttng_ust_opencl_arguments_argument_info_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_arguments:kernel_info", (void *) &clprof_lttng_ust_opencl_arguments_kernel_info_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_dump:enqueue_counter", (void *) &clprof_lttng_ust_opencl_dump_enqueue_counter_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_dump:kernel_arg_value", (void *) &clprof_lttng_ust_opencl_dump_kernel_arg_value_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_dump:svmptr_dump_event", (void *) &clprof_lttng_ust_opencl_dump_svmptr_dump_event_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_dump:svmptr_dump_result", (void *) &clprof_lttng_ust_opencl_dump_svmptr_dump_result_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_dump:buffer_dump_event", (void *) &clprof_lttng_ust_opencl_dump_buffer_dump_event_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_dump:buffer_dump_result", (void *) &clprof_lttng_ust_opencl_dump_buffer_dump_result_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_profiling:event_profiling", (void *) &clprof_lttng_ust_opencl_profiling_event_profiling_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_profiling:event_profiling_results", (void *) &clprof_lttng_ust_opencl_profiling_event_profiling_results_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_source:program_string", (void *) &clprof_lttng_ust_opencl_source_program_string_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_source:program_binary", (void *) &clprof_lttng_ust_opencl_source_program_binary_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_source:program_il", (void *) &clprof_lttng_ust_opencl_source_program_il_callback);
        opencl_register_callback(opencl_dispatch, "lttng_ust_opencl_devices:device_name", (void *) &clprof_lttng_ust_opencl_devices_device_name_callback);
}
void print_array(std::unordered_map<thapi_command_name, StatTime > aggregated, std::string header) {
    uint64_t total_time{0};
    uint64_t total_count{0};
    for (std::pair<thapi_command_name,  StatTime > element: aggregated) {
        total_time += element.second._time;
        total_count += element.second._count;
   }
   if (!total_count) {
        std::cout << "No "<< header << " information avalaible" << std::endl;
        return;
   }
   std::cout << header << std::endl;
   uint64_t len_max_command_name = 4;
   uint64_t len_max_time = 4;
   uint64_t len_max_time_ratio = 7;
   uint64_t len_max_count = 5;
   uint64_t len_max_avg = 7;
   uint64_t len_max_min = 3;
   uint64_t len_max_max = 3;
   len_max_command_name = std::max(len_max_command_name,  uint64_t{5} );
   len_max_time = std::max(len_max_time, format_time(total_time).size());
   len_max_count = std::max(len_max_count, to_string_with_precision(total_count,"",0).size() );
   for (std::pair<thapi_command_name,  StatTime > element: aggregated) {
       len_max_command_name =  std::max(len_max_command_name,  element.first.size());
       const auto [time, time_ratio, count, avg, min, max ] = element.second.to_string(total_time);
       len_max_time = std::max(len_max_time, time.size());
       len_max_time_ratio = std::max(len_max_time_ratio, time_ratio.size());
       len_max_count = std::max(len_max_count, count.size());
       len_max_avg = std::max(len_max_avg, avg.size());
       len_max_min = std::max(len_max_min, min.size());
       len_max_max = std::max(len_max_max, max.size());
   }
   std::vector<std::pair<thapi_command_name,  StatTime >> array_sorted(aggregated.begin(), aggregated.end());
   std::sort(array_sorted.begin(), array_sorted.end(), [](auto a, auto b) { return a.second._time > b.second._time;} );
   std::cout
   << std::setw(len_max_command_name) << std::right << "Name" << " | "
   << std::setw(len_max_time) << std::right << "Time" << " | "
   << std::setw(len_max_time_ratio) << std::right << "Time(%)" << " | "
   << std::setw(len_max_count) << std::right << "Calls" << " | "
   << std::setw(len_max_avg) << std::right << "Average" << " | "
   << std::setw(len_max_min) << std::right << "Min" << " | "
   << std::setw(len_max_max) << std::right << "Max" << " | "
   << std::endl;
   for (std::pair<thapi_command_name,  StatTime > element: array_sorted) {
        const thapi_command_name command_name = element.first;
        const auto [time, time_ratio, count, avg, min, max ] = element.second.to_string(total_time);
        std::cout
        << std::setw(len_max_command_name) << std::right << command_name << " | "
        << std::setw(len_max_time) << std::right << time << " | "
        << std::setw(len_max_time_ratio) << std::right << time_ratio << " | "
        << std::setw(len_max_count) << std::right << count << " | "
        << std::setw(len_max_avg) << std::right << avg << " | "
        << std::setw(len_max_min) << std::right << min << " | "
        << std::setw(len_max_max) << std::right << max << " | "
        << std::endl;
    }
   std::cout << std::setw(len_max_command_name) << std::right << "Total" << " | "
             << std::setw(len_max_time) << std::right << format_time(total_time) << " | "
             << std::setw(len_max_time_ratio) << std::right << "100.00%" << " | "
             << std::setw(len_max_count) << std::right <<  to_string_with_precision(total_count,"",0) << " | "
             << std::endl;
}
void print_array(std::unordered_map<thapi_command_name, StatByte > aggregated, std::string header) {
    uint64_t total_time{0};
    uint64_t total_count{0};
    for (std::pair<thapi_command_name,  StatByte > element: aggregated) {
        total_time += element.second._time;
        total_count += element.second._count;
   }
   if (!total_count) {
        std::cout << "No "<< header << " information avalaible" << std::endl;
        return;
   }
   std::cout << header << std::endl;
   uint64_t len_max_command_name = 4;
   uint64_t len_max_time = 4;
   uint64_t len_max_time_ratio = 7;
   uint64_t len_max_count = 5;
   uint64_t len_max_avg = 7;
   uint64_t len_max_min = 3;
   uint64_t len_max_max = 3;
   len_max_command_name = std::max(len_max_command_name,  uint64_t{5} );
   len_max_time = std::max(len_max_time, format_byte(total_time).size());
   len_max_count = std::max(len_max_count, to_string_with_precision(total_count,"",0).size() );
   for (std::pair<thapi_command_name,  StatByte > element: aggregated) {
       len_max_command_name =  std::max(len_max_command_name,  element.first.size());
       const auto [time, time_ratio, count, avg, min, max ] = element.second.to_string(total_time);
       len_max_time = std::max(len_max_time, time.size());
       len_max_time_ratio = std::max(len_max_time_ratio, time_ratio.size());
       len_max_count = std::max(len_max_count, count.size());
       len_max_avg = std::max(len_max_avg, avg.size());
       len_max_min = std::max(len_max_min, min.size());
       len_max_max = std::max(len_max_max, max.size());
   }
   std::vector<std::pair<thapi_command_name,  StatByte >> array_sorted(aggregated.begin(), aggregated.end());
   std::sort(array_sorted.begin(), array_sorted.end(), [](auto a, auto b) { return a.second._time > b.second._time;} );
   std::cout
   << std::setw(len_max_command_name) << std::right << "Name" << " | "
   << std::setw(len_max_time) << std::right << "Byte" << " | "
   << std::setw(len_max_time_ratio) << std::right << "Byte(%)" << " | "
   << std::setw(len_max_count) << std::right << "Calls" << " | "
   << std::setw(len_max_avg) << std::right << "Average" << " | "
   << std::setw(len_max_min) << std::right << "Min" << " | "
   << std::setw(len_max_max) << std::right << "Max" << " | "
   << std::endl;
   for (std::pair<thapi_command_name,  StatByte > element: array_sorted) {
        const thapi_command_name command_name = element.first;
        const auto [time, time_ratio, count, avg, min, max ] = element.second.to_string(total_time);
        std::cout
        << std::setw(len_max_command_name) << std::right << command_name << " | "
        << std::setw(len_max_time) << std::right << time << " | "
        << std::setw(len_max_time_ratio) << std::right << time_ratio << " | "
        << std::setw(len_max_count) << std::right << count << " | "
        << std::setw(len_max_avg) << std::right << avg << " | "
        << std::setw(len_max_min) << std::right << min << " | "
        << std::setw(len_max_max) << std::right << max << " | "
        << std::endl;
    }
   std::cout << std::setw(len_max_command_name) << std::right << "Total" << " | "
             << std::setw(len_max_time) << std::right << format_byte(total_time) << " | "
             << std::setw(len_max_time_ratio) << std::right << "100.00%" << " | "
             << std::setw(len_max_count) << std::right <<  to_string_with_precision(total_count,"",0) << " | "
             << std::endl;
}
void finalize_callbacks() {
   std::string display {bt_value_string_get(display_mode)};
   if (display == "compact" ) {
      {
         std::set<hostname_t> s_Hostnames;
         std::set<process_id_t> s_Processes;
         std::set<thread_id_t> s_Threads;
         std::unordered_map<thapi_command_name, StatTime > aggregated;
         for (auto element: api_call)
         {
            auto [ Hostnames,Processes,Threads,command_name ] = element.first;
            s_Hostnames.insert(Hostnames);
            s_Processes.insert(Processes);
            s_Threads.insert(Threads);
            const StatTime time = element.second;
            aggregated[command_name].merge(time);
         }
         std::ostringstream oss;
         oss << "API calls"
             << " | " << s_Hostnames.size() << " Hostnames"
             << " | " << s_Processes.size() << " Processes"
             << " | " << s_Threads.size() << " Threads"
             << std::endl;
         print_array(aggregated,oss.str());
         std::cout << std::endl;
      }
      {
         std::set<hostname_t> s_Hostnames;
         std::set<process_id_t> s_Processes;
         std::set<thread_id_t> s_Threads;
         std::set<cl_device_id> s_Devices;
         std::unordered_map<thapi_command_name, StatTime > aggregated;
         for (auto element: device_id_result)
         {
            auto [ Hostnames,Processes,Threads,Devices,command_name ] = element.first;
            s_Hostnames.insert(Hostnames);
            s_Processes.insert(Processes);
            s_Threads.insert(Threads);
            s_Devices.insert(Devices);
            const StatTime time = element.second;
            aggregated[command_name].merge(time);
         }
         std::ostringstream oss;
         oss << "Device profiling"
             << " | " << s_Hostnames.size() << " Hostnames"
             << " | " << s_Processes.size() << " Processes"
             << " | " << s_Threads.size() << " Threads"
             << " | " << s_Devices.size() << " Devices"
             << std::endl;
         print_array(aggregated,oss.str());
         std::cout << std::endl;
      }
      {
         std::set<hostname_t> s_Hostnames;
         std::set<process_id_t> s_Processes;
         std::set<thread_id_t> s_Threads;
         std::unordered_map<thapi_command_name, StatByte > aggregated;
         for (auto element: memory_trafic)
         {
            auto [ Hostnames,Processes,Threads,command_name ] = element.first;
            s_Hostnames.insert(Hostnames);
            s_Processes.insert(Processes);
            s_Threads.insert(Threads);
            const StatByte time = element.second;
            aggregated[command_name].merge(time);
         }
         std::ostringstream oss;
         oss << "Explicit memory trafic"
             << " | " << s_Hostnames.size() << " Hostnames"
             << " | " << s_Processes.size() << " Processes"
             << " | " << s_Threads.size() << " Threads"
             << std::endl;
         print_array(aggregated,oss.str());
         std::cout << std::endl;
      }
   } else if (display == "extended" ) {
      {
         std::unordered_map< hpt_t, std::unordered_map<thapi_command_name, StatTime >> d1;
         for (auto [s, time]: api_call ) {
            auto [Hostname,Process,Thread, command_name] = s;
            d1[ hpt_t( Hostname,Process,Thread )][command_name].merge(time);
         }
         std::vector<std::pair< hpt_t , std::unordered_map<thapi_command_name,  StatTime >>> array_sorted(d1.begin(), d1.end());
         std::sort(array_sorted.begin(), array_sorted.end(),   [](auto a, auto b) { return a.first > b.first ; } );
         for (auto[s, aggregated]: array_sorted) {
            auto [ Hostname,Process,Thread ] = s;
            std::ostringstream oss;
            oss <<  "API calls"
                << " | Hostname: " << Hostname
                << " | Process: " << Process
                << " | Thread: " << Thread
                << std::endl;
            print_array(aggregated, oss.str());
            std::cout << std::endl;
         }
     }
      {
         std::unordered_map< hpt_device_t, std::unordered_map<thapi_command_name, StatTime >> d1;
         for (auto [s, time]: device_id_result ) {
            auto [Hostname,Process,Thread,Device, command_name] = s;
            d1[ hpt_device_t( Hostname,Process,Thread,Device )][command_name].merge(time);
         }
         std::vector<std::pair< hpt_device_t , std::unordered_map<thapi_command_name,  StatTime >>> array_sorted(d1.begin(), d1.end());
         std::sort(array_sorted.begin(), array_sorted.end(),   [](auto a, auto b) { return a.first > b.first ; } );
         for (auto[s, aggregated]: array_sorted) {
            auto [ Hostname,Process,Thread,Device ] = s;
            std::ostringstream oss;
            oss <<  "Device profiling"
                << " | Hostname: " << Hostname
                << " | Process: " << Process
                << " | Thread: " << Thread
                << " | Device: " << Device
                <<  " (" << device_to_name[hp_device_t(Hostname,Process,Device)] << ")"
                << std::endl;
            print_array(aggregated, oss.str());
            std::cout << std::endl;
         }
     }
      {
         std::unordered_map< hpt_t, std::unordered_map<thapi_command_name, StatByte >> d1;
         for (auto [s, time]: memory_trafic ) {
            auto [Hostname,Process,Thread, command_name] = s;
            d1[ hpt_t( Hostname,Process,Thread )][command_name].merge(time);
         }
         std::vector<std::pair< hpt_t , std::unordered_map<thapi_command_name,  StatByte >>> array_sorted(d1.begin(), d1.end());
         std::sort(array_sorted.begin(), array_sorted.end(),   [](auto a, auto b) { return a.first > b.first ; } );
         for (auto[s, aggregated]: array_sorted) {
            auto [ Hostname,Process,Thread ] = s;
            std::ostringstream oss;
            oss <<  "Explicit memory trafic"
                << " | Hostname: " << Hostname
                << " | Process: " << Process
                << " | Thread: " << Thread
                << std::endl;
            print_array(aggregated, oss.str());
            std::cout << std::endl;
         }
     }
   }
}
