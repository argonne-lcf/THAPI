require_relative 'opencl_model'

provider = :lttng_ust_opencl

puts <<EOF
#define CL_TARGET_OPENCL_VERSION 220
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#include <CL/opencl.h>
#include <CL/cl_gl_ext.h>
#include <CL/cl_egl.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
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

struct buffer_obj_data {
  size_t size;
};

struct svmptr_obj_data {
  void* memptr;
  size_t size;
};

#define BIN_TEMPLATE "/tmp/binXXXXXX"
#define SOURCE_TEMPLATE "/tmp/sourceXXXXXX"
#define BIN_SOURCE_TEMPLATE "/tmp/binsourceXXXXXX"
#define IL_SOURCE_TEMPLATE "/tmp/ilsourceXXXXXX"

pthread_mutex_t opencl_obj_mutex = PTHREAD_MUTEX_INITIALIZER;

struct kernel_arg {
  int type;
  size_t arg_size;
  void *arg_value;
};

struct kernel_obj_data {
  cl_uint num_args;
  struct kernel_arg *args;
};

enum cl_obj_type {
  UNKNOWN = 0,
  PLATFORM,
  DEVICE,
  CONTEXT,
  COMMAND_QUEUE,
  BUFFER,
  PIPE,
  IMAGE,
  KERNEL,
  EVENT,
  SAMPLER,
  SVMMEM
};

struct opencl_obj_h {
  void *ptr;
  UT_hash_handle hh;
  enum cl_obj_type type;
  void *obj_data;
};

struct opencl_obj_h *opencl_objs = NULL;

static inline void add_buffer(cl_mem b, size_t size) {
  struct opencl_obj_h *o_h = NULL;
  struct buffer_obj_data *b_data = NULL;
  HASH_FIND_PTR(opencl_objs, &b, o_h);
  if (o_h != NULL) {
    if (o_h->obj_data != NULL)
      free(o_h->obj_data);
    b_data = (struct buffer_obj_data *)calloc(1, sizeof(struct buffer_obj_data));
    if (b_data == NULL) {
      HASH_DEL(opencl_objs, o_h);
      free(o_h);
      return;
    }
    o_h->type = BUFFER;
    b_data->size = size;
    o_h->obj_data = (void *)b_data;
  } else {
    o_h = (struct opencl_obj_h *)calloc(1, sizeof(struct opencl_obj_h));
    if (o_h == NULL)
      return;
    b_data = (struct buffer_obj_data *)calloc(1, sizeof(struct buffer_obj_data));
    if (b_data == NULL) {
      free(o_h);
      return;
    }
    o_h->ptr = (void *)b;
    o_h->type = BUFFER;
    b_data->size = size;
    o_h->obj_data = (void *)b_data;
    HASH_ADD_PTR(opencl_objs, ptr, o_h);
  }
}

static inline void add_kernel(cl_kernel k) {
  cl_uint num_args = 0;
  cl_int err = #{$clGetKernelInfo.prototype.pointer_name}(k, CL_KERNEL_NUM_ARGS, sizeof(cl_uint), &num_args, NULL);
  if (err == CL_SUCCESS) {
    struct opencl_obj_h *o_h = NULL;
    struct kernel_obj_data *k_data = NULL;
    HASH_FIND_PTR(opencl_objs, &k, o_h);
    if (o_h != NULL) {
      if (o_h->obj_data != NULL)
        free(o_h->obj_data);
      k_data = (struct kernel_obj_data *)calloc(1, sizeof(struct kernel_obj_data)+num_args*sizeof(struct kernel_arg));
      if (k_data == NULL) {
        HASH_DEL(opencl_objs, o_h);
        free(o_h);
        return;
      }
      o_h->type = KERNEL;
      k_data->num_args = num_args;
      k_data->args = (struct kernel_arg *)((intptr_t)k_data + sizeof(struct kernel_obj_data));
      o_h->obj_data = (void *)k_data;
    } else {
      o_h = (struct opencl_obj_h *)calloc(1, sizeof(struct opencl_obj_h));
      if (o_h == NULL)
        return;
      k_data = (struct kernel_obj_data *)calloc(1, sizeof(struct kernel_obj_data)+num_args*sizeof(struct kernel_arg));
      if (k_data == NULL) {
        free(o_h);
        return;
      }
      o_h->ptr = (void *)k;
      o_h->type = KERNEL;
      k_data->num_args = num_args;
      k_data->args = (struct kernel_arg *)((intptr_t)k_data + sizeof(struct kernel_obj_data));
      o_h->obj_data = (void *)k_data;
      HASH_ADD_PTR(opencl_objs, ptr, o_h);
    }
  }
}

static inline void add_kernel_arg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void *arg_value) {
  struct opencl_obj_h *o_h = NULL;
  HASH_FIND_PTR(opencl_objs, &kernel, o_h);
  if (o_h != NULL && o_h->type == KERNEL) {
    struct kernel_obj_data *k_data = (struct kernel_obj_data *)o_h->obj_data;
    if (k_data !=NULL && arg_index < k_data->num_args) {
      struct kernel_arg *arg = k_data->args + arg_index;
      if (arg_value != NULL) {
        if (arg->arg_value == NULL) {
          arg->arg_value = malloc(arg_size);
        } else if (arg->arg_size < arg_size) {
          arg->arg_value = realloc(arg->arg_value, arg_size);
        }
        memcpy(arg->arg_value, arg_value, arg_size);
      }
      arg->arg_size = arg_size;
      arg->type = 0;
    }
  }
}

struct buffer_dump_notify_data {
  size_t size;
  int fd;
  char path[sizeof(BIN_TEMPLATE)];
};

void CL_CALLBACK  buffer_dump_notify (cl_event event, cl_int event_command_exec_status, void *user_data) {
  struct buffer_dump_notify_data * data = (struct buffer_dump_notify_data *)user_data;
  tracepoint(lttng_ust_opencl_dump, buffer_dump_result, event, event_command_exec_status, data->size, data->path);
  if (event_command_exec_status == CL_COMPLETE) {
    ftruncate(data->fd, data->size);
  } else {
    ftruncate(data->fd, 0);
    unlink(data->path);
  }
  close(data->fd);
  free(user_data);
}

static inline size_t align(size_t size, unsigned int bit) {
  size_t alignment_mask = (1 << bit) - 1;
  return (size + alignment_mask) & ~(alignment_mask);
}

static inline int create_file_and_map(char template[], size_t size, void **ptr) {
  int fd = mkstemp(template);
  size_t map_size = align(size, 12);
  int err = ftruncate(fd, map_size); //page size
  if (err == 0) {
    *ptr = mmap(NULL, map_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (*ptr == MAP_FAILED) {
      ftruncate(fd, 0);
      close(fd);
      unlink(template);
      return -1;
    }
    return fd;
  } else {
    return -1;
  }
}

static inline void create_file_and_write(char template[], size_t size, const void * ptr) {
  int fd = mkstemp(template);
  if (fd != -1) {
    if (ptr != NULL) {
      ssize_t ret;
      size_t written = 0;
      while (written < size) {
        ret = write(fd, ptr, size);
        if (ret > 0) {
          written += ret;
        } else //if (errno != EINTR) avoid errno...
          break;
      }
    }
    close(fd);
  }
}

static inline void dump_buffer(cl_command_queue command_queue, struct opencl_obj_h *o_h, cl_uint num_events_in_wait_list, cl_event *event_wait_list, cl_uint *new_num_events_in_wait_list, cl_event **new_event_wait_list) {
  cl_event event;
  void *ptr = NULL;
  struct buffer_dump_notify_data *data = NULL;
  struct buffer_obj_data *obj_data = (struct buffer_obj_data *)(o_h->obj_data);

  data = (struct buffer_dump_notify_data *)malloc(sizeof(struct buffer_dump_notify_data));
  if (data == NULL)
    return;
  ptr = (void *)((intptr_t)data + sizeof(struct buffer_dump_notify_data));
  data->size = obj_data->size;
  strncpy(data->path, BIN_TEMPLATE, sizeof(data->path));
  data->fd = create_file_and_map(data->path, data->size, &ptr);
  if (data->fd == -1) {
    free(data);
    return;
  }

  cl_int err = #{$clEnqueueReadBuffer.prototype.pointer_name}(command_queue, (cl_mem)(o_h->ptr), CL_FALSE, 0, obj_data->size, ptr, num_events_in_wait_list, event_wait_list, &event);
  if (err == CL_SUCCESS) {
    int _set_retval = #{$clSetEventCallback.prototype.pointer_name}(event, CL_COMPLETE, buffer_dump_notify, data);
    tracepoint(lttng_ust_opencl_dump, buffer_dump_event, (cl_mem)(o_h->ptr), _set_retval, event);
    if (new_event_wait_list != NULL) {
      *new_num_events_in_wait_list += 1;
      *new_event_wait_list = realloc(*new_event_wait_list, *new_num_events_in_wait_list * sizeof(cl_event));
      if (*new_event_wait_list != NULL) {
        *new_event_wait_list[*new_num_events_in_wait_list -1] = event;
      } else {
        #{$clReleaseEvent.prototype.pointer_name}(event);
      }
    } else {
      #{$clReleaseEvent.prototype.pointer_name}(event);
    }
    if (_set_retval != CL_SUCCESS) {
      free(data);
    }
  } else {
    free(data);
  }
}

static inline int dump_kernel_args(cl_command_queue command_queue, cl_kernel kernel, uint64_t enqueue_counter, cl_command_queue_properties properties, cl_uint *num_events_in_wait_list, cl_event **event_wait_list) {
  cl_event * new_event_wait_list = NULL;
  cl_uint new_num_events_in_wait_list = 0;
  struct opencl_obj_h *o_h = NULL;
  HASH_FIND_PTR(opencl_objs, &kernel, o_h);
  if (o_h != NULL && o_h->type == KERNEL) {
    struct kernel_obj_data *k_data = (struct kernel_obj_data *)o_h->obj_data;
    if (k_data !=NULL) {
      for (int arg_index = 0; arg_index < k_data->num_args; arg_index++) {
        struct kernel_arg *arg = k_data->args + arg_index;
        tracepoint(#{provider}_dump, kernel_arg_value, enqueue_counter, arg->arg_size, arg->arg_value);
        if (arg->arg_value != NULL && arg->arg_size == 8) {
          struct opencl_obj_h *oo_h = NULL;
          HASH_FIND_PTR(opencl_objs, (void **)(arg->arg_value), oo_h);
          if (oo_h != NULL) {
            switch (oo_h->type) {
            case BUFFER:
              if (properties | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)
                dump_buffer(command_queue, oo_h, *num_events_in_wait_list, *event_wait_list, &new_num_events_in_wait_list, &new_event_wait_list);
              else
                dump_buffer(command_queue, oo_h, *num_events_in_wait_list, *event_wait_list, NULL, NULL);
              break;
            default:
              break;
            }
          }
        }
      }
    }
  }
  if (new_event_wait_list != NULL && new_num_events_in_wait_list > 0) {
    *event_wait_list = new_event_wait_list;
    *num_events_in_wait_list = new_num_events_in_wait_list;
    return 1;
  }
  if (new_event_wait_list != NULL)
    free(new_event_wait_list);
  return 0;
}

static inline cl_event dump_kernel_buffers(cl_command_queue command_queue, cl_kernel kernel, cl_event *event) {
  cl_event * new_event_wait_list = NULL;
  cl_uint new_num_events_in_wait_list = 0;
  cl_uint num_event = (event == NULL ? 0 : 1);
  struct opencl_obj_h *o_h = NULL;
  HASH_FIND_PTR(opencl_objs, &kernel, o_h);
  if (o_h != NULL && o_h->type == KERNEL) {
    struct kernel_obj_data *k_data = (struct kernel_obj_data *)o_h->obj_data;
    if (k_data !=NULL) {
      for (int arg_index = 0; arg_index < k_data->num_args; arg_index++) {
        struct kernel_arg *arg = k_data->args + arg_index;
        if (arg->arg_value != NULL && arg->arg_size == 8) {
          struct opencl_obj_h *oo_h = NULL;
          HASH_FIND_PTR(opencl_objs, (void **)(arg->arg_value), oo_h);
          if (oo_h != NULL) {
            switch (oo_h->type) {
            case BUFFER:
              if (event != NULL)
                dump_buffer(command_queue, oo_h, num_event, event, &new_num_events_in_wait_list, &new_event_wait_list);
              else
                dump_buffer(command_queue, oo_h, 0, NULL, NULL, NULL);
              break;
            default:
              break;
            }
          }
        }
      }
    }
  }
  if (new_event_wait_list != NULL && new_num_events_in_wait_list > 0) {
    cl_event ev;
    #{$clEnqueueBarrierWithWaitList.prototype.pointer_name}(command_queue, new_num_events_in_wait_list, new_event_wait_list, &ev);
    for (int i = 0; i < new_num_events_in_wait_list; i++) {
      #{$clReleaseEvent.prototype.pointer_name}(new_event_wait_list[i]);
    }
    free(new_event_wait_list);
    return ev;
  }
  if (new_event_wait_list != NULL)
    free(new_event_wait_list);
  return NULL;
}

static pthread_once_t _init = PTHREAD_ONCE_INIT;

static void _load_tracer(void) {
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

static inline void _init_tracer(void) {
  pthread_once(&_init, _load_tracer);
}

EOF

$opencl_commands.each { |c|
  puts <<EOF
#{c.decl} {
EOF
  if c.init?
    puts <<EOF
  _init_tracer();
EOF
  end
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
