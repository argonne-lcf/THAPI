puts <<EOF
#ifndef _TRACER_HEADER_
#define _TRACER_HEADER_
typedef enum {
    CL_ICDL_OCL_VERSION=1,
    CL_ICDL_VERSION=2,
    CL_ICDL_NAME=3,
    CL_ICDL_VENDOR=4,
  } cl_icdl_info;
#endif
EOF
