require_relative 'itt_model'

# Common Printing Function
common_block = lambda { |c, provider|
  params = c.parameters.collect(&:name)
  
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  c.tracepoint_parameters.each { |p|
    puts "  #{p.type} #{p.name};"
  }
  c.tracepoint_parameters.each { |p|
    puts p.init unless p.after?
  }

  c.prologues.each { |e|
    puts e
  }

  tracepoint_params = c.tracepoint_parameters.reject { |p| p.after? }.collect(&:name)
  tracepoint_params.push("_retval") if c.has_return_type?

  puts <<EOF
  tracepoint(#{provider}, #{c.name}, #{(params+tracepoint_params).join(", ")});
EOF

  c.epilogues.each { |e|
    puts e
  }

}

# Customize Model for struct

# Todo everything should done in one place
$itt_commands.each { |c|
  next unless c.has_return_type?

  register_prologue(c.name, 
if c.type.kind_of?(YAMLCAst::Pointer)
<<EOF
  #{c.type} _retval = calloc(1, sizeof(*_retval));
EOF
else
<<EOF
  #{c.type} _retval = {};
EOF
end
)

  register_epilogue(c.name, <<EOF
  return _retval;
EOF
)
}

# Handshake, registering function pointer
register_epilogue("__itt_api_init", <<EOF
  (void)group;
  if(!g || !g->api_list_ptr) return;
  
  static const struct { const char* name; void* fn; } map[] = {
    { "__itt_domain_create",        (void*)__itt_domain_create },
    { "__itt_domain_createA",       (void*)__itt_domain_createA },
    { "__itt_string_handle_create", (void*)__itt_string_handle_create },
    { "__itt_string_handle_createA",(void*)__itt_string_handle_createA },
    { "__itt_task_begin",           (void*)__itt_task_begin },
    { "__itt_task_beginA",          (void*)__itt_task_beginA },
    { "__itt_task_end",             (void*)__itt_task_end },
    { "__itt_marker",               (void*)__itt_marker },
    { "__itt_markerA",              (void*)__itt_markerA },
    { "__itt_thread_set_name",      (void*)__itt_thread_set_name },
    { "__itt_thread_set_nameA",     (void*)__itt_thread_set_nameA },
    { "__itt_pause",                (void*)__itt_pause },
    { "__itt_resume",               (void*)__itt_resume },
    { "__itt_detach",               (void*)__itt_detach },
    { "__itt_event_create",         (void*)__itt_event_create },
    { "__itt_event_start",          (void*)__itt_event_start },
    { "__itt_event_end",            (void*)__itt_event_end },
    { "__itt_metadata_add",         (void*)__itt_metadata_add },
    { NULL,                         NULL }
  };

   for(__itt_api_info* api = g->api_list_ptr; (api && api->name); ++api){
     for(int i=0; map[i].name; ++i){
       if(strcmp(api->name, map[i].name)==0){
         if(api->func_ptr) {
           *api->func_ptr = map[i].fn;
         }
         break;
       }
     }
   }
EOF
)

# Printing

normal_wrapper = lambda { |c, provider|
  puts <<EOF
#{c.decl} {
EOF
  common_block.call(c, provider)
  puts <<EOF
}

EOF
}

puts <<EOF
#include "ittnotify.h"
#include "itt_tracepoints.h"
#include <stdlib.h>
EOF

$itt_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_itt)
}
