void CL_CALLBACK  event_notify (cl_event event, cl_int event_command_exec_status, void *user_data) {
  (void)user_data;
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling_results)) {
    cl_ulong queued;
    cl_ulong submit;
    cl_ulong start;
    cl_ulong end;
    cl_int queued_status = CL_GET_EVENT_PROFILING_INFO_PTR(event, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queued, NULL);
    cl_int submit_status = CL_GET_EVENT_PROFILING_INFO_PTR(event, CL_PROFILING_COMMAND_SUBMIT, sizeof(cl_ulong), &submit, NULL);
    cl_int start_status = CL_GET_EVENT_PROFILING_INFO_PTR(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    cl_int end_status = CL_GET_EVENT_PROFILING_INFO_PTR(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    do_tracepoint(lttng_ust_opencl_profiling, event_profiling_results, event, event_command_exec_status,
              queued_status, queued, submit_status, submit, start_status, start, end_status, end);
  }
}

static int     do_dump = 0;
pthread_mutex_t enqueue_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t opencl_obj_mutex = PTHREAD_MUTEX_INITIALIZER;
static int64_t enqueue_counter = 0;
static int64_t dump_start = 0;
static int64_t dump_end = INT64_MAX;

#define BIN_TEMPLATE "/tmp/binXXXXXX"
#define SOURCE_TEMPLATE "/tmp/sourceXXXXXX"
#define BIN_SOURCE_TEMPLATE "/tmp/binsourceXXXXXX"
#define IL_SOURCE_TEMPLATE "/tmp/ilsourceXXXXXX"

pthread_mutex_t opencl_closures_mutex = PTHREAD_MUTEX_INITIALIZER;

struct opencl_closure {
  void *ptr;
  void *c_ptr;
  UT_hash_handle hh;
  ffi_cif cif;
  ffi_closure *closure;
  ffi_type **types;
};

struct opencl_closure * opencl_closures = NULL;

struct buffer_obj_data {
  size_t size;
};

struct svmptr_obj_data;

struct svmptr_obj_data {
  struct svmptr_obj_data *prev;
  struct svmptr_obj_data *next;
  void* ptr;
  size_t size;
};

struct svmptr_obj_data *svmptr_objs = NULL;

struct kernel_arg {
  // 0 regular, 1 SVM
  int type;
  size_t arg_size;
  void *arg_value;
};

struct kernel_obj_data {
  cl_uint num_args;
  struct kernel_arg *args;
};

void kernel_obj_data_free(void *obj_data) {
  struct kernel_obj_data *data = (struct kernel_obj_data *)obj_data;
  for (cl_uint i = 0; i < data->num_args; i++) {
    if (data->args[i].arg_value) {
      free(data->args[i].arg_value);
    }
  }
  free(obj_data);
}

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
  void (*obj_data_free)(void *obj_data);
};

struct opencl_obj_h *opencl_objs = NULL;

static inline void delete_opencl_obj(struct opencl_obj_h *o_h) {
  HASH_DEL(opencl_objs, o_h);
  if (o_h->obj_data != NULL) {
    if (o_h->obj_data_free != NULL)
      o_h->obj_data_free(o_h->obj_data);
    else
      free(o_h->obj_data);
  }
  free(o_h);
}

int cmp_svm_ptr(struct svmptr_obj_data *svm_a, struct svmptr_obj_data *svm_b) {
  intptr_t a = (intptr_t)(svm_a->ptr);
  intptr_t b = (intptr_t)(svm_b->ptr);
  return (a < b ? -1 : (a == b ? 0 : 1));
}

static inline void add_svmptr(void *ptr, size_t size) {
  struct opencl_obj_h *o_h = NULL;
  struct svmptr_obj_data *svm_data = NULL;

  pthread_mutex_lock(&opencl_obj_mutex);
  HASH_FIND_PTR(opencl_objs, &ptr, o_h);
  if (o_h != NULL) {
    delete_opencl_obj(o_h);
  }
  pthread_mutex_unlock(&opencl_obj_mutex);

  o_h = (struct opencl_obj_h *)calloc(1, sizeof(struct opencl_obj_h));
  if (o_h == NULL)
    return;
  svm_data = (struct svmptr_obj_data *)calloc(1, sizeof(struct svmptr_obj_data));
  if (svm_data == NULL) {
    free(o_h);
    return;
  }
  o_h->ptr = ptr;
  o_h->type = SVMMEM;
  svm_data->ptr = ptr;
  svm_data->size = size;
  o_h->obj_data = (void *)svm_data;

  pthread_mutex_lock(&opencl_obj_mutex);
  HASH_ADD_PTR(opencl_objs, ptr, o_h);
  DL_INSERT_INORDER(svmptr_objs, svm_data, cmp_svm_ptr);
  pthread_mutex_unlock(&opencl_obj_mutex);
}

static inline void remove_svmptr(void *ptr) {
  struct opencl_obj_h *o_h = NULL;
  struct svmptr_obj_data *svm_data = NULL;

  pthread_mutex_lock(&opencl_obj_mutex);
  HASH_FIND_PTR(opencl_objs, &ptr, o_h);
  if (o_h != NULL) {
    svm_data = (struct svmptr_obj_data *)o_h->obj_data;
    DL_DELETE(svmptr_objs, svm_data);
    delete_opencl_obj(o_h);
  }
  pthread_mutex_unlock(&opencl_obj_mutex);
}

static inline void add_buffer(cl_mem b, size_t size) {
  struct opencl_obj_h *o_h = NULL;
  struct buffer_obj_data *b_data = NULL;

  pthread_mutex_lock(&opencl_obj_mutex);
  HASH_FIND_PTR(opencl_objs, &b, o_h);
  if (o_h != NULL) {
      delete_opencl_obj(o_h);
  }
  pthread_mutex_unlock(&opencl_obj_mutex);

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

  pthread_mutex_lock(&opencl_obj_mutex);
  HASH_ADD_PTR(opencl_objs, ptr, o_h);
  pthread_mutex_unlock(&opencl_obj_mutex);
}

static inline void add_kernel(cl_kernel k) {
  cl_uint num_args = 0;
  cl_int err = CL_GET_KERNEL_INFO_PTR(k, CL_KERNEL_NUM_ARGS, sizeof(cl_uint), &num_args, NULL);
  if (err == CL_SUCCESS) {
    struct opencl_obj_h *o_h = NULL;
    struct kernel_obj_data *k_data = NULL;

    pthread_mutex_lock(&opencl_obj_mutex);
    HASH_FIND_PTR(opencl_objs, &k, o_h);
    if (o_h != NULL) {
      delete_opencl_obj(o_h);
    }
    pthread_mutex_unlock(&opencl_obj_mutex);

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
    o_h->obj_data_free = kernel_obj_data_free;

    pthread_mutex_lock(&opencl_obj_mutex);
    HASH_ADD_PTR(opencl_objs, ptr, o_h);
    pthread_mutex_unlock(&opencl_obj_mutex);
  }
}

static inline void add_kernel_arg(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void *arg_value, int type) {
  struct opencl_obj_h *o_h = NULL;
  pthread_mutex_lock(&opencl_obj_mutex);
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
      arg->type = type;
    }
  }
  pthread_mutex_unlock(&opencl_obj_mutex);
}

struct buffer_dump_notify_data {
  uint64_t enqueue_counter;
  cl_uint arg_index;
  int direction;
  size_t size;
  int fd;
  char path[sizeof(BIN_TEMPLATE)];
};

void CL_CALLBACK  buffer_dump_notify (cl_event event, cl_int event_command_exec_status, void *user_data) {
  struct buffer_dump_notify_data * data = (struct buffer_dump_notify_data *)user_data;
  tracepoint(lttng_ust_opencl_dump, buffer_dump_result, data->enqueue_counter, data->arg_index, data->direction, event, event_command_exec_status, data->size, data->path);
  if (event_command_exec_status == CL_COMPLETE) {
    int err = ftruncate(data->fd, data->size);
    if(err)
      unlink(data->path);
  } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    ftruncate(data->fd, 0);
#pragma GCC diagnostic pop
    unlink(data->path);
  }
  close(data->fd);
  free(user_data);
}

struct svmptr_dump_notify_data {
  uint64_t enqueue_counter;
  cl_uint arg_index;
  int direction;
  size_t size;
  int fd;
  char path[sizeof(BIN_TEMPLATE)];
};

void CL_CALLBACK  svmptr_dump_notify (cl_event event, cl_int event_command_exec_status, void *user_data) {
  struct svmptr_dump_notify_data * data = (struct svmptr_dump_notify_data *)user_data;
  tracepoint(lttng_ust_opencl_dump, buffer_dump_result, data->enqueue_counter, data->arg_index, data->direction, event, event_command_exec_status, data->size, data->path);
  if (event_command_exec_status == CL_COMPLETE) {
    int err = ftruncate(data->fd, data->size);
    if(err)
      unlink(data->path);
  } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
    ftruncate(data->fd, 0);
#pragma GCC diagnostic pop
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
      ftruncate(fd, 0);
#pragma GCC diagnostic pop
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

static void dump_buffer(cl_command_queue command_queue, struct opencl_obj_h *o_h, uint64_t enqueue_counter, cl_uint arg_index, int direction, cl_uint num_events_in_wait_list, cl_event *event_wait_list, cl_uint *new_num_events_in_wait_list, cl_event **new_event_wait_list) {
  cl_event event;
  void *ptr = NULL;
  struct buffer_dump_notify_data *data = NULL;
  struct buffer_obj_data *obj_data = (struct buffer_obj_data *)(o_h->obj_data);

  data = (struct buffer_dump_notify_data *)malloc(sizeof(struct buffer_dump_notify_data));
  if (data == NULL)
    return;
  data->enqueue_counter = enqueue_counter;
  data->arg_index = arg_index;
  data->direction = direction;
  data->size = obj_data->size;
  strncpy(data->path, BIN_TEMPLATE, sizeof(data->path));
  data->fd = create_file_and_map(data->path, data->size, &ptr);
  if (data->fd == -1) {
    free(data);
    return;
  }

  cl_int err = CL_ENQUEUE_READ_BUFFER_PTR(command_queue, (cl_mem)(o_h->ptr), CL_FALSE, 0, obj_data->size, ptr, num_events_in_wait_list, event_wait_list, &event);
  if (err == CL_SUCCESS) {
    int _set_retval = CL_SET_EVENT_CALLBACK_PTR(event, CL_COMPLETE, buffer_dump_notify, data);
    tracepoint(lttng_ust_opencl_dump, buffer_dump_event, enqueue_counter, arg_index, direction, (cl_mem)(o_h->ptr), _set_retval, event);
    if (new_event_wait_list != NULL) {
      *new_num_events_in_wait_list += 1;
      *new_event_wait_list = (cl_event *)realloc(*new_event_wait_list, *new_num_events_in_wait_list * sizeof(cl_event));
      if (*new_event_wait_list != NULL) {
        (*new_event_wait_list)[*new_num_events_in_wait_list -1] = event;
      } else {
        CL_RELEASE_EVENT_PTR(event);
      }
    } else {
      CL_RELEASE_EVENT_PTR(event);
    }
    if (_set_retval != CL_SUCCESS) {
      free(data);
    }
  } else {
    free(data);
  }
}

static void dump_svmptr(cl_command_queue command_queue, struct opencl_obj_h *o_h, uint64_t enqueue_counter, cl_uint arg_index, int direction, cl_uint num_events_in_wait_list, cl_event *event_wait_list, cl_uint *new_num_events_in_wait_list, cl_event **new_event_wait_list) {
  cl_event event;
  void *ptr = NULL;
  struct svmptr_dump_notify_data *data = NULL;
  struct svmptr_obj_data *obj_data = (struct svmptr_obj_data *)(o_h->obj_data);

  data = (struct svmptr_dump_notify_data *)malloc(sizeof(struct svmptr_dump_notify_data));
  if (data == NULL)
    return;
  data->enqueue_counter = enqueue_counter;
  data->arg_index = arg_index;
  data->direction = direction;
  data->size = obj_data->size;
  strncpy(data->path, BIN_TEMPLATE, sizeof(data->path));
  data->fd = create_file_and_map(data->path, data->size, &ptr);
  if (data->fd == -1) {
    free(data);
    return;
  }

  cl_int err = CL_ENQUEUE_SVM_MEMCPY_PTR(command_queue, CL_FALSE, ptr, obj_data->ptr, obj_data->size, num_events_in_wait_list, event_wait_list, &event);
  if (err == CL_SUCCESS) {
    int _set_retval = CL_SET_EVENT_CALLBACK_PTR(event, CL_COMPLETE, svmptr_dump_notify, data);
    tracepoint(lttng_ust_opencl_dump, svmptr_dump_event, enqueue_counter, arg_index, direction, obj_data->ptr, _set_retval, event);
    if (new_event_wait_list != NULL) {
      *new_num_events_in_wait_list += 1;
      *new_event_wait_list = (cl_event *)realloc(*new_event_wait_list, *new_num_events_in_wait_list * sizeof(cl_event));
      if (*new_event_wait_list != NULL) {
        (*new_event_wait_list)[*new_num_events_in_wait_list -1] = event;
      } else {
        CL_RELEASE_EVENT_PTR(event);
      }
    } else {
      CL_RELEASE_EVENT_PTR(event);
    }
    if (_set_retval != CL_SUCCESS) {
      free(data);
    }
  } else {
    free(data);
  }
}

static void dump_opencl_object(cl_command_queue command_queue, uint64_t enqueue_counter, struct kernel_arg *arg, int do_event, cl_uint arg_index, int direction, cl_uint num_events_in_wait_list, cl_event *event_wait_list, cl_uint *new_num_events_in_wait_list, cl_event **new_event_wait_list) {
  struct opencl_obj_h *oo_h = NULL;
  struct svmptr_obj_data *svm_data = NULL;
  pthread_mutex_lock(&opencl_obj_mutex);
  HASH_FIND_PTR(opencl_objs, (void **)(arg->arg_value), oo_h);
  pthread_mutex_unlock(&opencl_obj_mutex);
  if (oo_h != NULL) {
    switch (oo_h->type) {
    case BUFFER:
      if (do_event)
        dump_buffer(command_queue, oo_h, enqueue_counter, arg_index, direction, num_events_in_wait_list, event_wait_list, new_num_events_in_wait_list, new_event_wait_list);
      else
        dump_buffer(command_queue, oo_h, enqueue_counter, arg_index, direction, num_events_in_wait_list, event_wait_list, NULL, NULL);
      break;
    case SVMMEM:
      if (do_event)
        dump_svmptr(command_queue, oo_h, enqueue_counter, arg_index, direction, num_events_in_wait_list, event_wait_list, new_num_events_in_wait_list, new_event_wait_list);
      else
        dump_svmptr(command_queue, oo_h, enqueue_counter, arg_index, direction, num_events_in_wait_list, event_wait_list, NULL, NULL);
      break;
    default:
      break;
    }
  } else {
    //check if it is a pointer into an SVM region
    pthread_mutex_lock(&opencl_obj_mutex);
    DL_FOREACH(svmptr_objs, svm_data) {
      if (*(uintptr_t *)(arg->arg_value) > (uintptr_t)(svm_data->ptr) && *(uintptr_t *)(arg->arg_value) < (uintptr_t)(svm_data->ptr) + svm_data->size) {
        HASH_FIND_PTR(opencl_objs, (void **)(arg->arg_value), oo_h);
        break;
      }
    }
    pthread_mutex_unlock(&opencl_obj_mutex);
    if (oo_h != NULL && oo_h->type == SVMMEM) {
      if (do_event)
        dump_svmptr(command_queue, oo_h, enqueue_counter, arg_index, direction, num_events_in_wait_list, event_wait_list, new_num_events_in_wait_list, new_event_wait_list);
      else
        dump_svmptr(command_queue, oo_h, enqueue_counter, arg_index, direction, num_events_in_wait_list, event_wait_list, NULL, NULL);
    }
  }
}


static int dump_kernel_args(cl_command_queue command_queue, cl_kernel kernel, uint64_t enqueue_counter, cl_command_queue_properties properties, cl_uint *num_events_in_wait_list, cl_event **event_wait_list) {
  cl_event * new_event_wait_list = NULL;
  cl_uint new_num_events_in_wait_list = 0;
  struct opencl_obj_h *o_h = NULL;
  pthread_mutex_lock(&opencl_obj_mutex);
  HASH_FIND_PTR(opencl_objs, &kernel, o_h);
  pthread_mutex_unlock(&opencl_obj_mutex);
  if (o_h != NULL && o_h->type == KERNEL) {
    struct kernel_obj_data *k_data = (struct kernel_obj_data *)o_h->obj_data;
    if (k_data !=NULL) {
      for (cl_uint arg_index = 0; arg_index < k_data->num_args; arg_index++) {
        struct kernel_arg *arg = k_data->args + arg_index;
        tracepoint(lttng_ust_opencl_dump, kernel_arg_value, enqueue_counter, arg_index, arg->arg_size, arg->arg_value);
        if (arg->arg_value != NULL && arg->arg_size == 8) {
          dump_opencl_object(command_queue, enqueue_counter, arg, properties | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, arg_index, 0, *num_events_in_wait_list, *event_wait_list, &new_num_events_in_wait_list, &new_event_wait_list);
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

static cl_event dump_kernel_buffers(cl_command_queue command_queue, cl_kernel kernel, uint64_t enqueue_counter, cl_event *event) {
  cl_event * new_event_wait_list = NULL;
  cl_uint new_num_events_in_wait_list = 0;
  cl_uint num_event = (event == NULL ? 0 : 1);
  struct opencl_obj_h *o_h = NULL;
  pthread_mutex_lock(&opencl_obj_mutex);
  HASH_FIND_PTR(opencl_objs, &kernel, o_h);
  pthread_mutex_unlock(&opencl_obj_mutex);
  if (o_h != NULL && o_h->type == KERNEL) {
    struct kernel_obj_data *k_data = (struct kernel_obj_data *)o_h->obj_data;
    if (k_data !=NULL) {
      for (cl_uint arg_index = 0; arg_index < k_data->num_args; arg_index++) {
        struct kernel_arg *arg = k_data->args + arg_index;
        if (arg->arg_value != NULL && arg->arg_size == 8) {
          dump_opencl_object(command_queue, enqueue_counter, arg, event != NULL, arg_index, 1, num_event, event, &new_num_events_in_wait_list, &new_event_wait_list);
        }
      }
    }
  }
  if (new_event_wait_list != NULL && new_num_events_in_wait_list > 0) {
    cl_event ev;
    CL_EMQUEUE_BARRIER_WITH_WAIT_LIST_PTR(command_queue, new_num_events_in_wait_list, new_event_wait_list, &ev);
    for (cl_uint i = 0; i < new_num_events_in_wait_list; i++) {
      CL_RELEASE_EVENT_PTR(new_event_wait_list[i]);
    }
    free(new_event_wait_list);
    return ev;
  }
  if (new_event_wait_list != NULL)
    free(new_event_wait_list);
  return NULL;
}

