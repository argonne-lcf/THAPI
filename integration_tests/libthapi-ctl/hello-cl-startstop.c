// Includes
#include <stdio.h>
#include <string.h>
#include <CL/opencl.h>
#include "./cl_utils.h"

#include "thapi-ctl.h"

#define NLOOP 10

int main(int argc, char* argv[]) {

   cl_int err;

   //  _              _                      _
   // |_) |  _. _|_ _|_ _  ._ ._ _    ()    | \  _     o  _  _
   // |   | (_|  |_  | (_) |  | | |   (_X   |_/ (/_ \/ | (_ (/_
   //
    printf(">>> Initializing OpenCL Platform and Device...\n");

    cl_uint platform_idx = 0;
    cl_uint device_idx = 0;

    if (argc > 1)
      platform_idx = (cl_uint) atoi(argv[1]);
    if (argc > 2)
      device_idx = (cl_uint) atoi(argv[2]);

    char name[128];
    /* - - -
    Plateform
    - - - - */
    //A platform is a specific OpenCL implementation, for instance AMD, NVIDIA or Intel.
    // Intel may have a different OpenCL implementation for the CPU and GPU.

    // Discover the number of platforms:
    cl_uint platform_count;
    err = clGetPlatformIDs(0, NULL, &platform_count);
    check_error(err, "clGetPlatformIds");

    // Now ask OpenCL for the platform IDs:
    cl_platform_id* platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * platform_count);
    err = clGetPlatformIDs(platform_count, platforms, NULL);
    check_error(err, "clGetPlatformIds");

    cl_platform_id platform = platforms[platform_idx];
    err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 128, name, NULL);
    check_error(err, "clGetPlatformInfo");

    printf("Platform #%d: %s\n", platform_idx, name);

    /* - - - -
    Device
    - - - - */
    // Device gather data
    cl_uint device_count;
    err = clGetDeviceIDs(platform,  CL_DEVICE_TYPE_ALL , 0, NULL, &device_count);
    check_error(err, "clGetdeviceIds");

    cl_device_id* devices = (cl_device_id*)malloc(sizeof(cl_device_id) * device_count);
    err = clGetDeviceIDs(platform,  CL_DEVICE_TYPE_ALL , device_count, devices, NULL);
    check_error(err, "clGetdeviceIds");

    cl_device_id device = devices[device_idx];
    err = clGetDeviceInfo(device, CL_DEVICE_NAME, 128, name, NULL);
    check_error(err, "clGetPlatformInfo");

    printf("-- Device #%d: %s\n", device_idx, name);

    //
    // /   _  ._ _|_  _    _|_   ()    / \
    // \_ (_) | | |_ (/_ >< |_   (_X   \_X |_| (/_ |_| (/_
    //

    /* - - - -
    Context
    - - - - */
    // A context is a platform with a set of available devices for that platform.
    cl_context context = clCreateContext(0, device_count, devices, NULL, NULL, &err);
    check_error(err,"clCreateContext");

    /* - - - -
    Command queue
    - - - - */
    // The OpenCL functions that are submitted to a command-queue are enqueued in the order the calls are made but can be configured to execute in-order or out-of-order.
    cl_command_queue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    check_error(err,"clCreateCommandQueue");

    // create a device buffer
    size_t a_size = 100;
    size_t batch_size = a_size / NLOOP;
    size_t offset = 0;
    size_t a_bytes = a_size * sizeof(float);
    float *h_a = (float*)malloc(a_bytes);
    for (unsigned int i = 0; i < a_size; i++) {
        h_a[i] = (float)i;
    }
    cl_mem d_a = clCreateBuffer(context, CL_MEM_READ_WRITE, a_bytes, NULL, &err);
    check_error(err,"cclCreateBuffer");

    /* - - - -
    Execute
    - - - - */
    for (unsigned int i = 0; i < NLOOP; i++) {
      printf(">>> Kernel Execution [%d]...\n", i);

      // trace only odd executions
      if (i % 2 == 1)
         thapi_start_tracing();

    // (cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t size, const void *ptr, cl_uint num_events_in_wait_list, const cl_event *event_wait_list, cl_event *event);
      err  = clEnqueueWriteBuffer(queue, d_a, CL_TRUE, offset * sizeof(float), batch_size * sizeof(float), h_a, 0, NULL, NULL);
      check_error(err,"clEnqueueWriteBuffer");

      /* - - -
      Sync & check
      - - - */

       // Wait for the command queue to get serviced before reading back results
      clFinish(queue);

      if (i % 2 == 1)
         thapi_stop_tracing();

      offset += batch_size;
    }

    //
    // /  |  _   _. ._  o ._   _  
    // \_ | (/_ (_| | | | | | (_| 
    //                         _|
    free(devices);
    free(platforms);
    clReleaseCommandQueue(queue);
    clReleaseMemObject(d_a);
    clReleaseContext(context);

    // Exit
    return 0;
}

// =================================================================================================
