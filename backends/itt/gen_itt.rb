require_relative 'itt_model'

# Customization of codegen

$itt_commands.each { |c|
  next unless c.has_return_type?

  register_prologue(c.name,
    if c.type.kind_of?(YAMLCAst::Pointer)
      "#{c.type} _retval = calloc(1, sizeof(*_retval));"
    else
      "#{c.type} _retval = {};"
end
)

  register_epilogue(c.name, "return _retval;")
}

# Sometime, but not always, those function are called by ittstatic
# But we never use them in btx
register_prologue("__itt_event_create", "_retval = atomic_fetch_add(&event_counter, 1);")
register_prologue("__itt_domain_create", "_retval->flags = 1; _retval->nameA=name;")
register_prologue("__itt_string_handle_create", "_retval->strA=name;")
register_prologue("__itt_task_begin", "if (domain->flags == 0) return;")
register_prologue("__itt_task_end", "if (domain->flags == 0) return;")

register_prologue("__itt_metadata_add", "tracepoint(lttng_ust_itt_metadata, metadata, count * __itt_metadata_type_size(type), data);")

# Printing

common_block = lambda { |c, provider|

  l = []

  l += c.tracepoint_parameters.map { |p| "  #{p.type} #{p.name};"}
  l += c.tracepoint_parameters.filter_map { |p| p.init unless p.after? }
  l += c.prologues

  params = c.parameters.collect(&:name)
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  tracepoint_params.push("_retval") if c.has_return_type?

  l += ["tracepoint(#{provider}, #{c.name}, #{(params+tracepoint_params).join(", ")});"]
  l += c.epilogues

  "  "+l.join("\n  ")
}

puts <<EOF
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
puts $itt_commands.filter_map { |c|
  next if c.function.inline
  l  = ["#{c.decl} {"]
  l += [common_block.call(c, provider)]
  l += ["}"]
}.join("  \n")

puts <<EOF


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
