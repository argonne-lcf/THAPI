#ifndef OPENCL_MISSING_APIS_H
#define OPENCL_MISSING_APIS_H

/* Intel extension entry points that THAPI traces but that no public OpenCL
 * header declares (KhronosGroup/OpenCL-Headers does not carry them; they live
 * in Intel compute-runtime private headers). The signatures are the ones of
 * cl.xml.patch. */

typedef cl_uint cl_execution_info_intel;

extern CL_API_ENTRY cl_int CL_API_CALL clAddCommentINTEL(cl_device_id device, const char *comment);

extern CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceFunctionPointerINTEL(cl_device_id device,
                                cl_program program,
                                const char *function_name,
                                cl_ulong *function_pointer_ret);

extern CL_API_ENTRY cl_mem CL_API_CALL
clCreateImageWithPropertiesINTEL(cl_context context,
                                 const cl_mem_properties_intel *properties,
                                 const cl_image_format *image_format,
                                 const cl_image_desc *image_desc,
                                 void *host_ptr,
                                 cl_int *errcode_ret);

extern CL_API_ENTRY cl_int CL_API_CALL
clGetDeviceGlobalVariablePointerINTEL(cl_device_id device,
                                      cl_program program,
                                      const char *global_variable_name,
                                      size_t *global_variable_size_ret,
                                      void **global_variable_pointer_ret);

extern CL_API_ENTRY cl_command_queue CL_API_CALL
clCreatePerfCountersCommandQueueINTEL(cl_context context,
                                      cl_device_id device,
                                      cl_command_queue_properties properties,
                                      cl_uint configuration,
                                      cl_int *errcode_ret);

extern CL_API_ENTRY cl_int CL_API_CALL clGetExecutionInfoINTEL(cl_command_queue command_queue,
                                                               cl_kernel kernel,
                                                               cl_uint work_dim,
                                                               const size_t *global_work_offset,
                                                               const size_t *local_work_size,
                                                               cl_execution_info_intel param_name,
                                                               size_t param_value_size,
                                                               void *param_value,
                                                               size_t *param_value_size_ret);

extern CL_API_ENTRY cl_int CL_API_CALL clEnqueueNDRangeKernelINTEL(cl_command_queue command_queue,
                                                                   cl_kernel kernel,
                                                                   cl_uint work_dim,
                                                                   const size_t *global_work_offset,
                                                                   const size_t *work_group_count,
                                                                   const size_t *local_work_size,
                                                                   cl_uint num_events_in_wait_list,
                                                                   const cl_event *event_wait_list,
                                                                   cl_event *event);

extern CL_API_ENTRY cl_int CL_API_CALL clGetImageParamsINTEL(cl_context context,
                                                             const cl_image_format *image_format,
                                                             const cl_image_desc *image_desc,
                                                             size_t *image_row_pitch,
                                                             size_t *image_slice_pitch);

extern CL_API_ENTRY cl_int CL_API_CALL clEnqueueVerifyMemoryINTEL(cl_command_queue command_queue,
                                                                  const void *allocation_ptr,
                                                                  const void *expected_data,
                                                                  size_t size_of_comparison,
                                                                  cl_uint comparison_mode);

extern CL_API_ENTRY cl_int CL_API_CALL clSetPerformanceConfigurationINTEL(cl_device_id device,
                                                                          cl_uint count,
                                                                          cl_uint *offsets,
                                                                          cl_uint *values);

#endif
