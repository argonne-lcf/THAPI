require_relative 'ze_model'
require 'set'

puts <<EOF
#include <stdint.h>
#include <stddef.h>
#include "ze.h.include"
#include <dlfcn.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <ffi.h>
#include "uthash.h"
#include "utlist.h"

#include "ze_tracepoints.h"
#include "zet_tracepoints.h"
#include "zes_tracepoints.h"
#include "zel_tracepoints.h"
#include "ze_structs_tracepoints.h"
#include "zet_structs_tracepoints.h"
#include "zes_structs_tracepoints.h"
#include "zel_structs_tracepoints.h"
#include "ze_sampling.h"
#include "ze_profiling.h"
#include "ze_properties.h"
#include "ze_build.h"

EOF

$struct_type_conversion_table = {
  "ZE_STRUCTURE_TYPE_IMAGE_MEMORY_PROPERTIES_EXP" => "ZE_STRUCTURE_TYPE_IMAGE_MEMORY_EXP_PROPERTIES",
  "ZE_STRUCTURE_TYPE_CONTEXT_POWER_SAVING_HINT_EXP_DESC" => "ZE_STRUCTURE_TYPE_POWER_SAVING_HINT_EXP_DESC",
  "ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32_HANDLE" => "ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32",
  "ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_WIN32_HANDLE" => "ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_WIN32",
}

def get_structs_types(namespace, types, structs)
  types.select { |t|
    t.type.kind_of?(YAMLCAst::Struct) && (struct = structs.find { |s| t.type.name == s.name }) && struct.members.first.name == "stype"
  }.reject { |t|
    t.name.start_with?("#{namespace}_base_")
  }.map(&:name).to_set
end

def gen_struct_printer(namespace, types)
  puts <<EOF
static
void _print_lttng_ust_#{namespace}_struct(const void * p) {
  #{namespace}_structure_type_t stype = (#{namespace}_structure_type_t)((ze_base_desc_t *)p)->stype;
  switch (stype) {
EOF
  types.each { |t|
    ename = "#{namespace.to_s.upcase}_STRUCTURE_TYPE_#{t.delete_prefix(namespace.to_s+"_").delete_suffix("_t").upcase}"
    ename = $struct_type_conversion_table[ename] if $struct_type_conversion_table[ename]
    puts <<EOF
  case #{ename}:
    tracepoint(lttng_ust_#{namespace}_structs, #{t}, ((#{t} *)p));
    break;
EOF
  }
  if namespace == :ze
    puts <<EOF
  case ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2:
    tracepoint(lttng_ust_ze_structs, ze_device_properties_t, ((ze_device_properties_t *)p));
    break;
EOF
  elsif namespace == :zes
    puts <<EOF
  case ZES_STRUCTURE_TYPE_BASE_STATE:
    break;
EOF
  end
  puts <<EOF
  case #{namespace.to_s.upcase}_STRUCTURE_TYPE_FORCE_UINT32:
    break;
  }
}

static
void _print_lttng_ust_#{namespace}_structs(const void * p) {
  if (p) {
    _print_lttng_ust_#{namespace}_struct(p);
    _print_lttng_ust_#{namespace}_structs(((ze_base_desc_t *)p)->pNext);
  }
}

EOF
end

ze_struct_types = get_structs_types(:ze, $ze_api["typedefs"], $ze_api["structs"])
zet_struct_types = get_structs_types(:zet, $zet_api["typedefs"], $zet_api["structs"])
zes_struct_types = get_structs_types(:zes, $zes_api["typedefs"], $zes_api["structs"])
zel_struct_types = get_structs_types(:zel, $zel_api["typedefs"], $zel_api["structs"])
gen_struct_printer(:ze, ze_struct_types)
gen_struct_printer(:zet, zet_struct_types)
gen_struct_printer(:zes, zes_struct_types)
gen_struct_printer(:zel, zel_struct_types)


all_commands = $ze_commands + $zet_commands + $zes_commands + $zel_commands
(all_commands).each { |c|
  puts "#define #{ZE_POINTER_NAMES[c]} #{c.pointer_name}"
}

(all_commands).each { |c|
  puts <<EOF

#{c.decl_pointer(c.pointer_type_name)};
static #{c.pointer_type_name} #{ZE_POINTER_NAMES[c]} = (void *) 0x0;
EOF
}

puts <<EOF

static void find_ze_symbols(void * handle, int verbose) {
EOF

(all_commands).each { |c|
  puts <<EOF

  #{ZE_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");
  if (!#{ZE_POINTER_NAMES[c]} && verbose)
    fprintf(stderr, "Missing symbol #{c.name}!\\n");
EOF
}

puts <<EOF
}

EOF

puts File::read(File.join(SRC_DIR,"tracer_ze_helpers.include.c"))

common_block = lambda { |c, provider, types|
  params = c.parameters ? c.parameters.collect(&:name) : []
  tp_params = if c.parameters
    c.parameters.collect { |p|
      if p.type.kind_of?(YAMLCAst::Pointer) && p.type.type.kind_of?(YAMLCAst::Function)
        "(void *)(intptr_t)" + p.name
      else
        p.name
      end
    }
  else
    []
  end
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  c.tracepoint_parameters.each { |p|
    puts "  #{p.type} #{p.name};"
  }
  c.tracepoint_parameters.each { |p|
    puts p.init
  }
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{START}, #{(tp_params+tracepoint_params).join(", ")});
EOF
  c.meta_parameters.select { |p|
    p.kind_of?(InScalar) &&
    (a = p.command[p.name]) &&
    types.include?(a.type.type.name)
  }.each { |p|
    puts <<EOF
  if (_do_chained_structs && #{p.name})
    _print_#{provider}_structs(#{p.name}->pNext);
EOF
  }

  c.prologues.each { |p|
    puts p
  }

  if c.has_return_type?
    puts <<EOF
  #{c.type} _retval;
  _retval = #{ZE_POINTER_NAMES[c]}(#{params.join(", ")});
EOF
  else
    puts "  #{ZE_POINTER_NAMES[c]}(#{params.join(", ")});"
  end
  c.epilogues.each { |e|
    puts e
  }
  if c.has_return_type?
    tp_params.push "_retval"
  end
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{STOP}, #{(tp_params+tracepoint_params).join(", ")});
EOF
  c.meta_parameters.select { |p|
    p.kind_of?(OutScalar) &&
    (a = p.command[p.name]) &&
    !a.type.type.kind_of?(YAMLCAst::Pointer) &&
    types.include?(a.type.type.name)
  }.each { |p|
    puts <<EOF
  if (_do_chained_structs && #{p.name})
    _print_#{provider}_structs(#{p.name}->pNext);
EOF
  }

}

normal_wrapper = lambda { |c, provider, types|
  puts <<EOF
#{c.decl} {
EOF
  if c.init?
    puts <<EOF
  _init_tracer();
EOF
  end
  common_block.call(c, provider, types)
  if c.has_return_type?
    puts <<EOF
  return _retval;
EOF
  end
  puts <<EOF
}

EOF
} 

$ze_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_ze, ze_struct_types)
}
$zet_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_zet, zet_struct_types)
}
$zes_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_zes, zes_struct_types)
}
$zel_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_zel, zel_struct_types)
}
