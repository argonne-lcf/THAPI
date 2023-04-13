require_relative 'cuda_model'

puts <<EOF
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
EOF

($cuda_commands + $cuda_exports_commands).each { |c|
  puts "#define #{CUDA_POINTER_NAMES[c]} #{c.pointer_name}"
}

($cuda_commands + $cuda_exports_commands).each { |c|
  puts <<EOF

#{c.decl_pointer(c.pointer_type_name)};
static #{c.pointer_type_name} #{CUDA_POINTER_NAMES[c]} = (void *) 0x0;
EOF
}

puts <<EOF

static void find_cuda_symbols(void * handle, int verbose) {
EOF

$cuda_commands.each { |c|
  puts <<EOF

  #{CUDA_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");
  if (!#{CUDA_POINTER_NAMES[c]} && verbose)
    fprintf(stderr, "Missing symbol #{c.name}!\\n");
EOF
}

puts <<EOF
}

EOF

export_tables = YAML::load_file(File.join(SRC_DIR,"cuda_export_tables.yaml"))

puts <<EOF
static void * cuda_extension_dispatcher(const CUuuid *uuid, size_t offset);

static void find_cuda_extensions() {

EOF
export_tables.each { |table|
  puts <<EOF
  {
    CUresult res;
    const void *pExportTable;
    CUuuid uuid = { { #{table["uuid"].collect{|e| "0x" << e.to_s(16)}.join(", ")} } };
    res = CU_GET_EXPORT_TABLE_PTR(&pExportTable, &uuid);
    if (res == CUDA_SUCCESS) {
      size_t tableSize = *(size_t*)pExportTable;
EOF
  table["functions"].each { |func|
    puts <<EOF
      if (0x#{func["offset"].to_s(16)} < tableSize) {
        #{upper_snake_case(func["name"]+"_ptr")} = *(#{func["name"]}_t *)((intptr_t)pExportTable + 0x#{func["offset"].to_s(16)});
      }
EOF
  }
  puts <<EOF
    }
  }

EOF
}

puts <<EOF
}

EOF

puts File::read(File.join(SRC_DIR,"tracer_cuda_helpers.include.c"))

common_block = lambda { |c, provider|
  params = c.parameters.collect(&:name)
  tp_params = c.parameters.collect { |p|
    if p.type.kind_of?(YAMLCAst::Pointer) && p.type.type.kind_of?(YAMLCAst::Function)
      "(void *)(intptr_t)" + p.name
    else
      p.name
    end
  }
  c.tracepoint_parameters.each { |p|
    puts "  #{p.type} #{p.name};"
  }
  c.tracepoint_parameters.each { |p|
    puts p.init unless p.after?
  }
  tracepoint_params = c.tracepoint_parameters.reject { |p| p.after? }.collect(&:name)
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{START}, #{(tp_params+tracepoint_params).join(", ")});
EOF
  tracepoint_params = c.tracepoint_parameters.collect(&:name)

  c.prologues.each { |p|
    puts p
  }

  if c.has_return_type?
    puts <<EOF
  #{c.type} _retval;
  _retval = #{CUDA_POINTER_NAMES[c]}(#{params.join(", ")});
EOF
  else
    puts "  #{CUDA_POINTER_NAMES[c]}(#{params.join(", ")});"
  end
  c.tracepoint_parameters.each { |p|
    puts p.init if p.after?
  }
  c.epilogues.each { |e|
    puts e
  }
  if c.has_return_type?
    tp_params.push "_retval"
  end
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{STOP}, #{(tp_params+tracepoint_params).join(", ")});
EOF
}

normal_wrapper = lambda { |c, provider|
  puts <<EOF
#{c.decl} {
EOF
  if c.init?
    puts <<EOF
  _init_tracer();
EOF
  end
  common_block.call(c, provider)
  if c.has_return_type?
    puts <<EOF
  return _retval;
EOF
  end
  puts <<EOF
}

EOF
} 

$cuda_commands.each { |c|
  normal_wrapper.call(c, :lttng_ust_cuda)
}

$cuda_exports_commands.each { |c|
  c.function.instance_variable_set(:@storage, "static")
  normal_wrapper.call(c, :lttng_ust_cuda_exports)
}

puts <<EOF

static void * cuda_extension_dispatcher(const CUuuid *uuid, size_t offset) {
EOF

export_tables.each { |table|
puts <<EOF
  {
    CUuuid ref = {{ #{table["uuid"].collect{|e| "0x" << e.to_s(16)}.join(", ")} }};
    if (!memcmp(uuid, &ref, sizeof(CUuuid))) {
      switch(offset) {
EOF
    table["functions"].each { |func|
puts <<EOF
      case 0x#{func["offset"].to_s(16)}:
        return &#{func["name"]};
EOF
    }
puts <<EOF
      default:
        return NULL;
      }
    }
  }
EOF
}

puts <<EOF
  return NULL;
}
EOF
