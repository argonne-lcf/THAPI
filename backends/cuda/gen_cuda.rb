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

COMMANDS.each do |c|
  puts "#define #{CUDA_POINTER_NAMES[c]} #{c.pointer_name}"
end

COMMANDS.groups[:lttng_ust_cuda].each do |c|
  puts <<~EOF

    static #{YAMLCAst::Declaration.new(name: c.name + '_unsupp', type: c.function.type)} {
      #{c.parameters.map(&:name).map { |n| "(void)#{n};" }.join("\n  ")}
      fprintf(stderr, "THAPI: #{c.name} was called, but it is unsupported by the driver\\n");
      return CUDA_ERROR_NOT_SUPPORTED;
    }
    static #{YAMLCAst::Declaration.new(name: c.name + '_uninit', type: c.function.type)};
    #{c.decl_pointer(c.pointer_type_name)};
    static #{c.pointer_type_name} #{CUDA_POINTER_NAMES[c]} = (void *)&#{c.name}_uninit;
    static #{YAMLCAst::Declaration.new(name: c.name + '_uninit', type: c.function.type)} {
      #{c.parameters.map(&:name).map { |n| "(void)#{n};" }.join("\n  ")}
      _init_tracer();
  EOF
  params = c.parameters.collect(&:name)
  if c.has_return_type?
    puts <<EOF
  return #{CUDA_POINTER_NAMES[c]}(#{params.join(', ')});
EOF
  else
    puts <<EOF
  #{CUDA_POINTER_NAMES[c]}(#{params.join(', ')});
EOF
  end
  puts <<~EOF
    }
  EOF
end

print_pointer_table(COMMANDS.groups[:lttng_ust_cuda_exports], CUDA_POINTER_NAMES)

COMMANDS.groups[:lttng_ust_cuda].each do |c|
  puts <<~EOF
    #{c.decl_hidden_alias};
    static void wrap_#{c.name}(void **pfn);
  EOF
end

puts <<~EOF

  static void find_cuda_symbols(void * handle, int verbose) {
EOF

print_dlsym_lookups(COMMANDS.groups[:lttng_ust_cuda], CUDA_POINTER_NAMES,
                    prefix: 'THAPI: ', fallback: ->(c) { "#{c.name}_unsupp" })

puts <<~EOF
  }

EOF

export_tables = YAML.load_file(File.join(SRC_DIR, 'cuda_export_tables.yaml'))

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

normal_wrapper = lambda { |c, provider|
  print_wrapper(c) { print_traced_body(c, provider, CUDA_POINTER_NAMES) }
}

COMMANDS.groups[:lttng_ust_cuda].each do |c|
  normal_wrapper.call(c, :lttng_ust_cuda)
end

COMMANDS.groups[:lttng_ust_cuda].each do |c|
  puts <<~EOF

    static void wrap_#{c.name}(void **pfn) {
  EOF
  str = <<EOF
  if (*pfn == #{CUDA_POINTER_NAMES[c]}) {
    *pfn = #{c.hidden_alias_name};
  }
EOF
  print str
  puts <<~EOF
    }

  EOF
end

COMMANDS.groups[:lttng_ust_cuda_exports].each do |c|
  c.function.instance_variable_set(:@storage, 'static')
  normal_wrapper.call(c, :lttng_ust_cuda_exports)
end

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
