require_relative 'cuda_model'
require_relative '../../utils/gen_tracer_base'

puts <<~EOF
  #include <pthread.h>
  #include <sys/mman.h>
  #include <string.h>
  #include "cuda_tracepoints.h"
  #include "cuda_exports_tracepoints.h"
  #include "cuda_args.h"
  #include "cuda_profiling.h"
  #include "cuda_properties.h"
  #include "utlist.h"
  #include "uthash.h"

  static void _init_tracer(void);
EOF

print_pointer_defines(COMMANDS, CUDA_POINTER_NAMES)

# _unsupp is what the dlsym lookup installs when the driver lacks the symbol;
# _uninit is what the pointer starts at.
def declare(c, suffix)
  "static #{YAMLCAst::Declaration.new(name: "#{c.name}_#{suffix}", type: c.function.type)}"
end

# The body of a stub, as lines: each cast to void so the compiler does not
# warn about a parameter the stub ignores, then whatever the stub does. A
# command with no parameters contributes no lines rather than a blank one.
def stub_body(c, *statements)
  (c.parameters.to_a.map { |p| "(void)#{p.name};" } + statements).map { |l| "  #{l}" }.join("\n")
end

def unsupported_stub(c)
  <<~EOF
    #{declare(c, 'unsupp')} {
    #{stub_body(c, %(fprintf(stderr, "THAPI: #{c.name} was called, but it is unsupported by the driver\\n");),
                'return CUDA_ERROR_NOT_SUPPORTED;')}
    }
    #{declare(c, 'uninit')};
  EOF
end

def uninitialized_stub(c)
  call = "#{CUDA_POINTER_NAMES[c]}(#{c.parameters.to_a.collect(&:name).join(', ')});"
  <<~EOF
    #{declare(c, 'uninit')} {
    #{stub_body(c, '_init_tracer();', c.has_return_type? ? "return #{call}" : call)}
    }
  EOF
end

print_pointer_table(COMMANDS.groups[:lttng_ust_cuda], CUDA_POINTER_NAMES,
                    initializer: ->(c) { "(void *)&#{c.name}_uninit" },
                    before: method(:unsupported_stub),
                    after: method(:uninitialized_stub))

print_pointer_table(COMMANDS.groups[:lttng_ust_cuda_exports], CUDA_POINTER_NAMES)

COMMANDS.groups[:lttng_ust_cuda].each do |c|
  puts <<~EOF
    #{c.decl_hidden_alias};
    static void wrap_#{c.name}(void **pfn);
  EOF
end

print_find_symbols('cuda', COMMANDS.groups[:lttng_ust_cuda], CUDA_POINTER_NAMES,
                   prefix: 'THAPI: ', fallback: ->(c) { "#{c.name}_unsupp" })

export_tables = yaml_load_file_cached(File.join(SRC_DIR, 'cuda_export_tables.yaml'))

puts <<~EOF
  static void * cuda_extension_dispatcher(const CUuuid *uuid, size_t offset);

  static void find_cuda_extensions() {

EOF
export_tables.each do |table|
  puts <<EOF
  {
    CUresult res;
    const void *pExportTable;
    CUuuid uuid = { { #{table['uuid'].collect { |e| '0x' << e.to_s(16) }.join(', ')} } };
    res = CU_GET_EXPORT_TABLE_PTR(&pExportTable, &uuid);
    if (res == CUDA_SUCCESS) {
      size_t tableSize = *(size_t*)pExportTable;
EOF
  table['functions'].each do |func|
    puts <<EOF
      if (0x#{func['offset'].to_s(16)} < tableSize) {
        #{upper_snake_case(func['name'] + '_ptr')} = *(#{func['name']}_t *)((intptr_t)pExportTable + 0x#{func['offset'].to_s(16)});
      }
EOF
  end
  puts <<EOF
    }
  }

EOF
end

puts <<~EOF
  }

EOF

puts File.read(File.join(SRC_DIR, 'tracer_cuda_helpers.include.c'))

# cuda initializes from the _uninit trampoline each pointer starts at, not
# from the wrapper, so no wrapper carries an _init_tracer() call.
no_init = ->(_c) {}

print_traced_wrappers(COMMANDS.groups[:lttng_ust_cuda], :lttng_ust_cuda, CUDA_POINTER_NAMES,
                      init: no_init)

# The dispatch table hands back a pointer to the real function; swap in the
# hidden alias so a caller that resolves through it is traced too.
COMMANDS.groups[:lttng_ust_cuda].each do |c|
  puts <<~EOF
    static void wrap_#{c.name}(void **pfn) {
      if (*pfn == #{CUDA_POINTER_NAMES[c]}) {
        *pfn = #{c.hidden_alias_name};
      }
    }

  EOF
end

# An export-table entry is reached through the dispatcher, never linked
# against, so its wrapper is not the exported symbol.
print_traced_wrappers(COMMANDS.groups[:lttng_ust_cuda_exports], :lttng_ust_cuda_exports,
                      CUDA_POINTER_NAMES, init: no_init, storage: 'static ')

puts <<~EOF
  static void * cuda_extension_dispatcher(const CUuuid *uuid, size_t offset) {
EOF

export_tables.each do |table|
  puts <<EOF
  {
    CUuuid ref = {{ #{table['uuid'].collect { |e| '0x' << e.to_s(16) }.join(', ')} }};
    if (!memcmp(uuid, &ref, sizeof(CUuuid))) {
      switch(offset) {
EOF
  table['functions'].each do |func|
    puts <<EOF
      case 0x#{func['offset'].to_s(16)}:
        return &#{func['name']};
EOF
  end
  puts <<EOF
      default:
        return NULL;
      }
    }
  }
EOF
end

puts <<~EOF
    return NULL;
  }
EOF
