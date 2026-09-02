require_relative 'ze_model'
require_relative '../../utils/gen_tracer_base'

puts <<~EOF
    #include <stdint.h>
    #include <stddef.h>
    #include <stdbool.h>
    #include "ze.h.include"
    #include <dlfcn.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <alloca.h>
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
  #ifdef NEW_VERSION_WITH_ZER
    #include "zer_tracepoints.h"
  #endif
    #include "zex_tracepoints.h"
    #include "ze_structs_tracepoints.h"
    #include "zet_structs_tracepoints.h"
    #include "zes_structs_tracepoints.h"
    #include "zel_structs_tracepoints.h"
  #ifdef NEW_VERSION_WITH_ZER
    #include "zer_structs_tracepoints.h"
  #endif
    #include "zex_structs_tracepoints.h"
    #include "ze_sampling.h"
    #include "ze_profiling.h"
    #include "ze_properties.h"
    #include "ze_build.h"

EOF

def gen_struct_printer(namespace, types)
  puts <<~EOF
    static
    void _print_lttng_ust_#{namespace}_struct(const void * p) {
      #{namespace}_structure_type_t stype = (#{namespace}_structure_type_t)((ze_base_desc_t *)p)->stype;
      switch (stype) {
  EOF
  types.reject { |t| STRUCT_TYPE_REJECT.include?(t.to_s) }.each do |t|
    ename = "#{namespace.to_s.upcase}_STRUCTURE_TYPE_#{t.delete_prefix(namespace.to_s + '_').delete_suffix('_t').upcase}"
    ename = STRUCT_TYPE_CONVERSION_TABLE[ename] if STRUCT_TYPE_CONVERSION_TABLE[ename]
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

struct_types = APIS.to_h { |ns, api| [ns, concrete_tagged_structs(ns, api)] }

gen_struct_printer(:ze, struct_types[:ze])
gen_struct_printer(:zet, struct_types[:zet])
gen_struct_printer(:zes, struct_types[:zes])
gen_struct_printer(:zel, struct_types[:zel])
# The printer switches on <ns>_structure_type_t, which zer and zex do not
# declare: zer has no api.yaml at all, and zex names no structure types.

# zex is excluded: it is reached through libffi closures, not dlsym'd symbols.
zex_commands = COMMANDS.groups[:lttng_ust_zex]
all_commands = COMMANDS.to_a - zex_commands
all_commands.each do |c|
  puts "#define #{ZE_POINTER_NAMES[c]} #{c.pointer_name}"
end

all_commands.each do |c|
  puts <<~EOF

    #{c.decl_pointer(c.pointer_type_name)};
    static #{c.pointer_type_name} #{ZE_POINTER_NAMES[c]} = (void *) 0x0;
  EOF
end

zex_commands.each do |c|
  puts <<~EOF

    #{c.decl_pointer(c.pointer_type_name)};
    static #{c.decl_ffi_wrapper};
  EOF
end

puts <<~EOF

  static void find_ze_symbols(void * handle, int verbose) {
EOF

print_dlsym_lookups(all_commands, ZE_POINTER_NAMES)

puts <<~EOF
  }

EOF

puts File.read(File.join(SRC_DIR, 'tracer_ze_helpers.include.c'))

# ze can be asked to walk the pNext chain of an extension struct a call was
# handed. `direction` picks which side of the call is worth walking: an input
# struct is only meaningful before the call, an output struct only after.
print_chained_structs = lambda { |c, provider, types, direction|
  chained = c.meta_parameters.select do |p|
    p.is_a?(direction) &&
      (a = p.command[p.name]) &&
      !a.type.type.is_a?(YAMLCAst::Pointer) &&
      types.include?(a.type.type.name)
  end
  chained.each do |p|
    puts <<EOF
  if (_do_chained_structs && #{p.name})
    _print_#{provider}_structs(#{p.name}->pNext);
EOF
  end
}

common_block = lambda { |c, provider, types|
  call_args = tracepoint_call_args(c)
  print_tracepoint_locals(c)
  print_tracepoint_call(provider, c, :start, call_args)
  print_chained_structs.call(c, provider, types, InScalar)

  c.prologues.each do |p|
    puts p
  end

  # A ProcAddrTable getter declares _retval in its own prologue, because the
  # prologue has to reach into the table the call is about to fill in.
  print_traced_call(c, ZE_POINTER_NAMES[c],
                    declare_retval: !c.name.match(/(ze|zet|zes|zel|zer)Get.*ProcAddrTable/))

  c.epilogues.each do |e|
    puts e
  end
  call_args.push '_retval' if c.has_return_type?
  print_tracepoint_call(provider, c, :stop, call_args)
  print_chained_structs.call(c, provider, types, OutScalar)
}

normal_wrapper = lambda { |c, provider, types|
  # _init_tracer_dump() calls the real zeInit (ZE_INIT_PTR) and dumps device
  # properties. zesInit piggybacks on it so a pure-Sysman program (no zeInit)
  # still initializes the ze backend it depends on.
  init = if !c.init?
           nil
         elsif %w[zeInit zesInit].include?(c.name)
           "_init_tracer();\n  _init_tracer_dump();"
         else
           '_init_tracer();'
         end
  print_wrapper(c, init: init) { common_block.call(c, provider, types) }
}

# Which of a namespace's entry points get a hidden alias. zel is the exception:
# only its tracer API is aliased, so it opts in rather than out.
aliased = {
  ze: ->(n) { !n.match(/zeGet.*ProcAddrTable|^zeLoaderInit|^zeLoaderGetTracingHandle/) },
  zet: ->(n) { !n.match(/zetGet.*ProcAddrTable/) },
  zes: ->(n) { !n.match(/zesGet.*ProcAddrTable/) },
  zel: ->(n) { n.match(/^zelTracer/) && !n.match(/RegisterCallback$|ResetAllCallbacks$/) },
  zer: ->(n) { !n.match(/zerGet.*ProcAddrTable/) },
}

aliased.each do |ns, alias_wanted|
  COMMANDS.groups[:"lttng_ust_#{ns}"].each do |c|
    puts <<~EOF if alias_wanted.call(c.name)
      #{c.decl_hidden_alias};

    EOF
  end
end

%i[ze zet zes zel zer].each do |ns|
  provider = :"lttng_ust_#{ns}"
  COMMANDS.groups[provider].each do |c|
    normal_wrapper.call(c, provider, struct_types[ns])
  end
end

zex_commands.each do |c|
  puts <<~EOF
    static #{c.decl_ffi_wrapper} {
      (void)cif;
  EOF
  c.parameters.each_with_index do |p, i|
    puts <<EOF
  #{p} = *(#{p.type} *)args[#{i}];
EOF
  end
  common_block.call(c, :lttng_ust_zex, struct_types[:zex])
  if c.has_return_type?
    puts <<EOF
  *ffi_ret = _retval;
EOF
  end
  puts <<~EOF
    }

  EOF
end
