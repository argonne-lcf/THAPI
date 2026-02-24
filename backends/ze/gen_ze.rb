require_relative 'ze_model'

puts <<~EOF
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
  #include "zex_tracepoints.h"
  #include "ze_structs_tracepoints.h"
  #include "zet_structs_tracepoints.h"
  #include "zes_structs_tracepoints.h"
  #include "zel_structs_tracepoints.h"
  #include "zex_structs_tracepoints.h"
  #include "ze_sampling.h"
  #include "ze_profiling.h"
  #include "ze_properties.h"
  #include "ze_build.h"

EOF

def get_structs_types(namespace, types, structs)
  types.select do |t|
    t.type.is_a?(YAMLCAst::Struct) && (struct = structs.find do |s|
      t.type.name == s.name
    end) && struct.members.first.name == 'stype'
  end.reject do |t|
    t.name.start_with?("#{namespace}_base_")
  end.map(&:name).to_set
end

def gen_struct_printer(namespace, types)
  puts <<~EOF
    static
    void _print_lttng_ust_#{namespace}_struct(const void * p) {
      #{namespace}_structure_type_t stype = (#{namespace}_structure_type_t)((ze_base_desc_t *)p)->stype;
      switch (stype) {
  EOF
  types.reject { |t| $struct_type_reject.include?(t.to_s) }.each do |t|
    ename = "#{namespace.to_s.upcase}_STRUCTURE_TYPE_#{t.delete_prefix(namespace.to_s + '_').delete_suffix('_t').upcase}"
    ename = $struct_type_conversion_table[ename] if $struct_type_conversion_table[ename]
    puts <<EOF
  case #{ename}:
    tracepoint(lttng_ust_#{namespace}_structs, #{t}, ((#{t} *)p));
    break;
EOF
  end
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
  case ZES_STRUCTURE_TYPE_DEVICE_UUID:
    break;
EOF
  end
  puts <<~EOF
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

ze_struct_types = get_structs_types(:ze, $ze_api['typedefs'], $ze_api['structs'])
zet_struct_types = get_structs_types(:zet, $zet_api['typedefs'], $zet_api['structs'])
zes_struct_types = get_structs_types(:zes, $zes_api['typedefs'], $zes_api['structs'])
zel_struct_types = get_structs_types(:zel, $zel_api['typedefs'], $zel_api['structs'])
zex_struct_types = get_structs_types(:zex, $zex_api['typedefs'], $zex_api['structs'])

gen_struct_printer(:ze, ze_struct_types)
gen_struct_printer(:zet, zet_struct_types)
gen_struct_printer(:zes, zes_struct_types)
gen_struct_printer(:zel, zel_struct_types)
# gen_struct_printer(:zex, zex_struct_types)

all_commands = $ze_commands + $zet_commands + $zes_commands + $zel_commands
all_commands.each do |c|
  puts "#define #{ZE_POINTER_NAMES[c]} #{c.pointer_name}"
end

all_commands.each do |c|
  puts <<~EOF

    #{c.decl_pointer(c.pointer_type_name)};
    static #{c.pointer_type_name} #{ZE_POINTER_NAMES[c]} = (void *) 0x0;
  EOF
end

$zex_commands.each do |c|
  puts <<~EOF

    #{c.decl_pointer(c.pointer_type_name)};
    static #{c.decl_ffi_wrapper};
  EOF
end

puts <<~EOF

  static void find_ze_symbols(void * handle, int verbose) {
EOF

all_commands.each do |c|
  puts <<EOF

  #{ZE_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");
  if (!#{ZE_POINTER_NAMES[c]} && verbose)
    fprintf(stderr, "Missing symbol #{c.name}!\\n");
EOF
end

puts <<~EOF
  }

EOF

puts File.read(File.join(SRC_DIR, 'tracer_ze_helpers.include.c'))

common_block = lambda { |c, provider, types|
  params = c.parameters ? c.parameters.collect(&:name) : []
  tp_params = if c.parameters
                c.parameters.collect do |p|
                  if p.type.is_a?(YAMLCAst::Pointer) && p.type.type.is_a?(YAMLCAst::Function)
                    '(void *)(intptr_t)' + p.name
                  else
                    p.name
                  end
                end
              else
                []
              end
  tracepoint_params = c.tracepoint_parameters.collect(&:name)
  c.tracepoint_parameters.each do |p|
    puts "  #{p.type} #{p.name};"
  end
  c.tracepoint_parameters.each do |p|
    puts p.init
  end
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{START}, #{(tp_params + tracepoint_params).join(', ')});
EOF
  c.meta_parameters.select do |p|
    p.is_a?(InScalar) &&
      (a = p.command[p.name]) &&
      types.include?(a.type.type.name)
  end.each do |p|
    puts <<EOF
  if (_do_chained_structs && #{p.name})
    _print_#{provider}_structs(#{p.name}->pNext);
EOF
  end

  c.prologues.each do |p|
    puts p
  end

  if c.has_return_type?
    puts <<EOF unless c.name.match(/(ze|zet|zes|zel)Get.*ProcAddrTable/)
  #{c.type} _retval;
EOF
    puts <<EOF
  _retval = #{ZE_POINTER_NAMES[c]}(#{params.join(', ')});
EOF
  else
    puts "  #{ZE_POINTER_NAMES[c]}(#{params.join(', ')});"
  end
  c.epilogues.each do |e|
    puts e
  end
  tp_params.push '_retval' if c.has_return_type?
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{STOP}, #{(tp_params + tracepoint_params).join(', ')});
EOF
  c.meta_parameters.select do |p|
    p.is_a?(OutScalar) &&
      (a = p.command[p.name]) &&
      !a.type.type.is_a?(YAMLCAst::Pointer) &&
      types.include?(a.type.type.name)
  end.each do |p|
    puts <<EOF
  if (_do_chained_structs && #{p.name})
    _print_#{provider}_structs(#{p.name}->pNext);
EOF
  end
}

normal_wrapper = lambda { |c, provider, types|
  puts <<~EOF
    #{c.decl} {
  EOF
  if c.init?
    puts <<EOF
  _init_tracer();
EOF
    if c.name == 'zeInit'
      puts <<EOF
  _init_tracer_dump();
EOF
    end
  end
  common_block.call(c, provider, types)
  if c.has_return_type?
    puts <<EOF
  return _retval;
EOF
  end
  puts <<~EOF
    }

  EOF
}

$ze_commands.each do |c|
  if c.name.match(/zeGet.*ProcAddrTable/) || c.name.match(/zeLoaderInit|zelLoaderDriverCheck|zelLoaderTracingLayerInit|zeLoaderGetTracingHandle/)
    next
  end

  puts <<~EOF
    #{c.decl_hidden_alias};

  EOF
end
$zet_commands.each do |c|
  puts <<~EOF unless c.name.match(/zetGet.*ProcAddrTable/)
    #{c.decl_hidden_alias};

  EOF
end
$zes_commands.each do |c|
  puts <<~EOF unless c.name.match(/zesGet.*ProcAddrTable/)
    #{c.decl_hidden_alias};

  EOF
end
$zel_commands.each do |c|
  puts <<~EOF if c.name.match(/^zelTracer/) && !c.name.match(/RegisterCallback$/)
    #{c.decl_hidden_alias};

  EOF
end

$ze_commands.each do |c|
  normal_wrapper.call(c, :lttng_ust_ze, ze_struct_types)
end
$zet_commands.each do |c|
  normal_wrapper.call(c, :lttng_ust_zet, zet_struct_types)
end
$zes_commands.each do |c|
  normal_wrapper.call(c, :lttng_ust_zes, zes_struct_types)
end
$zel_commands.each do |c|
  normal_wrapper.call(c, :lttng_ust_zel, zel_struct_types)
end

$zex_commands.each do |c|
  puts <<~EOF
    static #{c.decl_ffi_wrapper} {
      (void)cif;
  EOF
  c.parameters.each_with_index do |p, i|
    puts <<EOF
  #{p} = *(#{p.type} *)args[#{i}];
EOF
  end
  common_block.call(c, :lttng_ust_zex, zex_struct_types)
  if c.has_return_type?
    puts <<EOF
  *ffi_ret = _retval;
EOF
  end
  puts <<~EOF
    }

  EOF
end
