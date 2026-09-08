require_relative 'itt_model'
require_relative '../../utils/gen_tracer_base'

# Customization of codegen

COMMANDS.each do |c|
  next unless c.has_return_type?

  # `= {}` is C23, so use `= {0}`, which also works for scalars pre-C23.
  # Can be modernized once we require C23.
  init = c.type.is_a?(YAMLCAst::Pointer) ? 'calloc(1, sizeof(*_retval))' : '{0}'
  c.add_prologue <<EOF
  #{c.type} _retval = #{init};
EOF
end

# Sometime, but not always, those function are called by ittstatic
# But we never use them in btx
COMMANDS.add_prologue '__itt_event_create', <<EOF
  _retval = atomic_fetch_add(&event_counter, 1);
EOF

COMMANDS.add_prologue '__itt_domain_create', <<EOF
  _retval->flags = 1; _retval->nameA=name;
EOF

COMMANDS.add_prologue '__itt_string_handle_create', <<EOF
  _retval->strA=name;
EOF

COMMANDS.add_prologue '__itt_task_begin', <<EOF
  if (domain->flags == 0) return;
EOF

COMMANDS.add_prologue '__itt_task_end', <<EOF
  if (domain->flags == 0) return;
EOF

COMMANDS.add_prologue '__itt_metadata_add', <<EOF
  tracepoint(lttng_ust_itt_metadata, metadata, type, count, count * __itt_metadata_type_size(type), data);
EOF

# Printing

puts <<~EOF
  #define INTEL_NO_MACRO_BODY
  #define INTEL_ITTNOTIFY_API_PRIVATE
  #include "itt_tracepoints.h"
  #include "itt_metadata.h"

  #include "ittnotify.h"
  #include "ittnotify_config.h"

  #include <stdio.h>
  #include <stdlib.h>
  #include <stdatomic.h>

  static _Atomic uint32_t event_counter = 0;


  static inline size_t __itt_metadata_type_size(__itt_metadata_type type)
  {
      switch (type) {
          case __itt_metadata_u64:
          case __itt_metadata_s64:
          case __itt_metadata_double:
              return 8;
          case __itt_metadata_u32:
          case __itt_metadata_s32:
          case __itt_metadata_float:
              return 4;
          case __itt_metadata_u16:
          case __itt_metadata_s16:
              return 2;
          case __itt_metadata_unknown:
          default:
              return 0;
      }
  }
EOF

provider = :lttng_ust_itt
COMMANDS.reject { |c| c.function.inline }.each do |c|
  print_wrapper(c) { print_callback_body(c, provider) }
end

puts <<~EOF


  static void fill_func_ptr_per_lib(__itt_global* p)
  {
      __itt_api_info* api_list = (__itt_api_info*)p->api_list_ptr;

      for (int i = 0; api_list[i].name != NULL; i++)
      {
          *(api_list[i].func_ptr) = (void*)__itt_get_proc(p->lib, api_list[i].name);
          if (*(api_list[i].func_ptr) == NULL)
          {
              *(api_list[i].func_ptr) = api_list[i].null_func;
          }
      }
  }

  extern void ITTAPI __itt_api_init(__itt_global* p, __itt_group_id init_groups)
  {
      if (p != NULL)
      {
          (void)init_groups;
          fill_func_ptr_per_lib(p);
      }
      else
      {
          printf("ERROR: Failed to initialize dynamic library\\n");
      }
  }
EOF
