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
  types.each do |t|
    puts traced_structure_type_names(t.to_s).map { |stype| "  case #{stype}:\n" }.join
    puts <<EOF
    tracepoint(lttng_ust_#{namespace}_structs, #{t}, ((#{t} *)p));
    break;
EOF
  end
  # The stypes with no case: the FORCE_UINT32 end marker, an stype the spec
  # renamed (both names share one value), a struct traced_structs rejects, and
  # at run time a value from a driver newer than our headers.
  puts <<~EOF
      default:
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

struct_types = APIS.to_h { |ns, api| [ns, traced_structs(api)] }

gen_struct_printer(:ze, struct_types[:ze])
gen_struct_printer(:zet, struct_types[:zet])
gen_struct_printer(:zes, struct_types[:zes])
gen_struct_printer(:zel, struct_types[:zel])
# The printer switches on <ns>_structure_type_t, which zer and zex do not
# declare: zer has no api.yaml at all, and zex names no structure types.

# zex is excluded: it is reached through libffi closures, not dlsym'd symbols.
zex_commands = COMMANDS.groups[:lttng_ust_zex]
all_commands = COMMANDS.to_a - zex_commands
print_pointer_defines(all_commands, ZE_POINTER_NAMES)

print_pointer_table(all_commands, ZE_POINTER_NAMES)

zex_commands.each do |c|
  puts <<~EOF

    #{c.decl_pointer(c.pointer_type_name)};
    static #{c.decl_ffi_wrapper};
  EOF
end

print_find_symbols('ze', all_commands, ZE_POINTER_NAMES)

puts File.read(File.join(SRC_DIR, 'tracer_ze_helpers.include.c'))

# ze can be asked to walk the pNext chain of an extension struct a call was
# handed. `direction` picks which side of the call is worth walking: an input
# struct is only meaningful before the call, an output struct only after.
def print_chained_structs(c, provider, types, direction)
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
end

# A ProcAddrTable getter declares _retval in its own prologue, because that
# prologue reaches into the table the call is about to fill in.
def ze_body_opts(c, provider, types)
  { after_entry: ->(cmd) { print_chained_structs(cmd, provider, types, InScalar) },
    after_exit: ->(cmd) { print_chained_structs(cmd, provider, types, OutScalar) },
    declare_retval: !c.name.match(PROC_ADDR_TABLE_GETTER) }
end

# _init_tracer_dump() calls the real zeInit (ZE_INIT_PTR) and dumps device
# properties. zesInit piggybacks on it so a pure-Sysman program (no zeInit)
# still initializes the ze backend it depends on.
ze_init = lambda { |c|
  next unless c.init?

  %w[zeInit zesInit].include?(c.name) ? "_init_tracer();\n  _init_tracer_dump();" : '_init_tracer();'
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
  print_traced_wrappers(COMMANDS.groups[provider], provider, ZE_POINTER_NAMES,
                        init: ze_init, body_opts: ->(c) { ze_body_opts(c, provider, struct_types[ns]) })
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
  print_traced_body(c, :lttng_ust_zex, ZE_POINTER_NAMES,
                    **ze_body_opts(c, :lttng_ust_zex, struct_types[:zex]))
  if c.has_return_type?
    puts <<EOF
  *ffi_ret = _retval;
EOF
  end
  puts <<~EOF
    }

  EOF
end
