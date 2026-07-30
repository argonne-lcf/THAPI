require_relative 'cuda_model'

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

($cuda_commands + $cuda_exports_commands).each do |c|
  puts "#define #{CUDA_POINTER_NAMES[c]} #{c.pointer_name}"
end

$cuda_commands.each do |c|
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

$cuda_exports_commands.each do |c|
  puts <<~EOF

    #{c.decl_pointer(c.pointer_type_name)};
    static #{c.pointer_type_name} #{CUDA_POINTER_NAMES[c]} = (void *) 0x0;
  EOF
end

$cuda_commands.each do |c|
  puts <<~EOF
    #{c.decl_hidden_alias};
    static void wrap_#{c.name}(void **pfn);
  EOF
end

puts <<~EOF

  static void find_cuda_symbols(void * handle, int verbose) {
EOF

$cuda_commands.each do |c|
  puts <<EOF

  #{CUDA_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");
  if (!#{CUDA_POINTER_NAMES[c]}) {
    #{CUDA_POINTER_NAMES[c]} = &#{c.name}_unsupp;
    if (verbose)
      fprintf(stderr, "THAPI: Missing symbol #{c.name}!\\n");
  }
EOF
end

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

common_block = lambda { |c, provider|
  params = c.parameters.collect(&:name)
  tp_params = c.parameters.collect do |p|
    if p.type.is_a?(YAMLCAst::Pointer) && p.type.type.is_a?(YAMLCAst::Function)
      '(void *)(intptr_t)' + p.name
    else
      p.name
    end
  end
  c.tracepoint_parameters.each do |p|
    puts "  #{p.type} #{p.name};"
  end
  c.tracepoint_parameters.each do |p|
    puts p.init unless p.after?
  end
  tracepoint_params = c.tracepoint_parameters.reject { |p| p.after? }.collect(&:name)
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{START}, #{(tp_params + tracepoint_params).join(', ')});
EOF
  tracepoint_params = c.tracepoint_parameters.collect(&:name)

  c.prologues.each do |p|
    puts p
  end

  if c.has_return_type?
    puts <<EOF
  #{c.type} _retval;
  _retval = #{CUDA_POINTER_NAMES[c]}(#{params.join(', ')});
EOF
  else
    puts "  #{CUDA_POINTER_NAMES[c]}(#{params.join(', ')});"
  end
  c.tracepoint_parameters.each do |p|
    puts p.init if p.after?
  end
  c.epilogues.each do |e|
    puts e
  end
  tp_params.push '_retval' if c.has_return_type?
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{STOP}, #{(tp_params + tracepoint_params).join(', ')});
EOF
}

normal_wrapper = lambda { |c, provider|
  puts <<~EOF
    #{c.decl} {
  EOF
  common_block.call(c, provider)
  if c.has_return_type?
    puts <<EOF
  return _retval;
EOF
  end
  puts <<~EOF
    }

  EOF
}

$cuda_commands.each do |c|
  normal_wrapper.call(c, :lttng_ust_cuda)
end

$cuda_commands.each do |c|
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

$cuda_exports_commands.each do |c|
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
