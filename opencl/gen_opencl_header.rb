puts <<EOF
#ifndef _TRACER_HEADER_
#define _TRACER_HEADER_
typedef enum {
    CL_ICDL_OCL_VERSION=1,
    CL_ICDL_VERSION=2,
    CL_ICDL_NAME=3,
    CL_ICDL_VENDOR=4,
  } cl_icdl_info;
typedef cl_bitfield  cl_mem_alloc_flags_intel;
typedef cl_bitfield  cl_mem_properties_intel;
typedef cl_mem_flags cl_mem_flags_intel;
typedef cl_uint      cl_mem_info_intel;
typedef cl_uint      cl_mem_advice_intel;
typedef cl_uint      cl_unified_shared_memory_type_intel;
typedef cl_bitfield  cl_unified_shared_memory_capabilities_intel;
typedef cl_uint      cl_execution_info_intel;
typedef cl_bitfield  cl_mem_migration_flags_intel;
typedef cl_bitfield  cl_device_affinity_domain_ext;
#endif
EOF
