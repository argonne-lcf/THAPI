require_relative 'opencl_model'

DUMP_MECHANISM = :buffer # :file, :buffer

provider = :lttng_ust_opencl

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/opencl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "uthash.h"
EOF

puts <<EOF
#include "opencl_tracepoints.h"
#include "opencl_profiling.h"
#include "opencl_source.h"
#include "opencl_dump.h"
EOF

$opencl_commands.each { |c|
  puts <<EOF
static #{c.decl_pointer} = (void *) 0x0;
EOF
}

puts <<EOF
void CL_CALLBACK  event_notify (cl_event event, cl_int event_command_exec_status, void *user_data) {
  if (tracepoint_enabled(#{provider}_profiling, event_profiling_results)) {
    cl_ulong queued;
    cl_ulong submit;
    cl_ulong start;
    cl_ulong end;
    cl_int queued_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queued, NULL);
    cl_int submit_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &submit, NULL);
    cl_int start_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    cl_int end_status = #{$clGetEventProfilingInfo.prototype.pointer_name}(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    do_tracepoint(#{provider}_profiling, event_profiling_results, event, event_command_exec_status,
              queued_status, queued, submit_status, submit, start_status, start, end_status, end);
  }
}

static int     do_dump = 0;
pthread_mutex_t enqueue_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
static int64_t enqueue_counter = 0;
static int64_t dump_start = 0;
static int64_t dump_end = INT64_MAX;

pthread_mutex_t memobj_mutex = PTHREAD_MUTEX_INITIALIZER;

struct memobj_h {
  cl_mem memobj;
  UT_hash_handle hh;
  size_t sz;
};

struct memobj_h *memobjs = NULL;

struct svmmemobj_h {
  void* memobj;
  UT_hash_handle hh;
  size_t sz;
};

struct svmmemobj_h *svmmemobjs = NULL;

pthread_mutex_t kernel_mutex = PTHREAD_MUTEX_INITIALIZER;

struct kernel_param {
  cl_uint arg_index;
  int memobj;
  size_t arg_size;
  const void *arg_value;
};

struct kernel_obj_h {
  cl_kernel kernel;
  UT_hash_handle hh;
  struct kernel_param *params;
};

struct kernel_obj_h *kernels = NULL;

void __load_tracer(void) __attribute__((constructor));
void __load_tracer(void) {
  void * handle = dlopen("libOpenCL.so", RTLD_LAZY | RTLD_LOCAL);
  if( !handle ) {
    printf("Failure: could not load OpenCL library!\\n");
    exit(1);
  }

  char *s = NULL;
  s = getenv("LTTNG_UST_OPENCL_DUMP");
  if (s)
    do_dump = 1;
  s = getenv("LTTNG_UST_OPENCL_DUMP_START");
  if (s)
    dump_start = atoll(s);
  s = getenv("LTTNG_UST_OPENCL_DUMP_END");
  if (s)
    dump_end = atoll(s);
EOF

$opencl_commands.each { |c|
  puts <<EOF
  #{c.prototype.pointer_name} = dlsym(handle, "#{c.prototype.name}") ;
EOF
}

puts <<EOF
}
EOF

$opencl_commands.each { |c|
  puts <<EOF
#{c.decl} {
EOF
  params = []
  params = c.parameters.collect(&:name) unless c.parameters.size == 1 && c.parameters.first.decl.strip == "void"
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  c.tracepoint_parameters.each { |p|
    puts "  #{p.type} #{p.name};"
  }
  c.tracepoint_parameters.each { |p|
    puts p.init
  }
  puts <<EOF
  tracepoint(#{provider}, #{c.prototype.name}_start, #{(params+tracepoint_params).join(", ")});
EOF
  c.prologues.each { |p|
    puts p
  }
  if c.prototype.has_return_type?
    puts <<EOF
  #{c.prototype.return_type} _retval;
  _retval = #{c.prototype.pointer_name}(#{params.join(", ")});
EOF
  else
    puts "  #{c.prototype.pointer_name}(#{params.join(", ")});"
  end
  c.epilogues.each { |e|
    puts e
  }
  if c.prototype.has_return_type?
    params.push "_retval"
    puts <<EOF
  tracepoint(#{provider}, #{c.prototype.name}_stop, #{(params+tracepoint_params).join(", ")});
  return _retval;
}

EOF
  else
    puts <<EOF
  tracepoint(#{provider}, #{c.prototype.name}_stop, #{(params+tracepoint_params).join(", ")});
}

EOF
  end
}
