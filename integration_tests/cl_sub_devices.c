// Partitions the first device with num_devices_ret = NULL, then writes a buffer
// on a queue of the first sub-device. THAPI must still report that write on the
// root device and that sub-device (#572).
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>

#define MAX_SUB_DEVICES 1024

int main(void) {
  cl_platform_id platform;
  cl_device_id device;
  if (clGetPlatformIDs(1, &platform, NULL) != CL_SUCCESS ||
      clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, NULL) != CL_SUCCESS)
    return 1;

  const cl_device_partition_property props[] = {CL_DEVICE_PARTITION_EQUALLY, 1, 0};
  static cl_device_id sub_devices[MAX_SUB_DEVICES];
  if (clCreateSubDevices(device, props, MAX_SUB_DEVICES, sub_devices, NULL) != CL_SUCCESS)
    return 1;

  cl_context context = clCreateContext(NULL, 1, &sub_devices[0], NULL, NULL, NULL);
  cl_command_queue queue = clCreateCommandQueueWithProperties(context, sub_devices[0], NULL, NULL);
  char host[4096] = {0};
  cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof host, NULL, NULL);
  if (clEnqueueWriteBuffer(queue, buffer, CL_TRUE, 0, sizeof host, host, 0, NULL, NULL) !=
      CL_SUCCESS)
    return 1;

  clReleaseMemObject(buffer);
  clReleaseCommandQueue(queue);
  clReleaseContext(context);
  return 0;
}
