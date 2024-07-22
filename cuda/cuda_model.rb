require 'yaml'
require 'pp'
require 'set'
require_relative '../utils/yaml_ast_lttng'
require_relative '../utils/LTTng'
require_relative '../utils/command.rb'
require_relative '../utils/meta_parameters'

if ENV["SRC_DIR"]
  SRC_DIR = ENV["SRC_DIR"]
else
  SRC_DIR = "."
end

RESULT_NAME = "cuResult"

$cuda_api_versions_yaml = YAML::load_file("cuda_api_versions.yaml")
$cuda_api_yaml = YAML::load_file("cuda_api.yaml")
$cuda_exports_api_yaml = YAML::load_file("cuda_exports_api.yaml")

$cuda_api = YAMLCAst.from_yaml_ast($cuda_api_yaml)
$cuda_exports_api = YAMLCAst.from_yaml_ast($cuda_exports_api_yaml)

cuda_funcs_e = $cuda_api["functions"]
cuda_exports_funcs_e = $cuda_exports_api["functions"]

cuda_types_e = $cuda_api["typedefs"]
cuda_exports_type_e = $cuda_exports_api["typedefs"]

typedefs = cuda_types_e + cuda_exports_type_e
structs = $cuda_api["structs"] + $cuda_exports_api["structs"]

find_all_types(typedefs)
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

HEX_INT_TYPES.push("CUdeviceptr")

class TracepointParameter
  attr_reader :name
  attr_reader :type
  attr_reader :init
  attr_reader :after

  def initialize(name, type, init, after: false)
    @name = name
    @type = type
    @init = init
    @after = after
  end

  def after?
    @after
  end
end

class OutNullArray < OutArray
  def initialize(command, name)
    sname = "_#{name.split("->").join(MEMBER_SEPARATOR)}_size"
    checks = check_for_null("#{name}")
    command.tracepoint_parameters.push TracepointParameter::new(sname, "size_t", <<EOF, after: true)
  #{sname} = 0;
  if(#{checks.join(" && ")}) {
    while(#{name}[#{sname}] != 0) {
      #{sname} += 2;
    }
    #{sname} ++;
  }
EOF
    super(command, name, sname)
  end
end

class InNullArray < InArray
  def initialize(command, name)
    sname = "_#{name.split("->").join(MEMBER_SEPARATOR)}_size"
    checks = check_for_null("#{name}")
    command.tracepoint_parameters.push TracepointParameter::new(sname, "size_t", <<EOF)
  #{sname} = 0;
  if(#{checks.join(" && ")}) {
    while(#{name}[#{sname}] != 0) {
      #{sname} += 2;
    }
    #{sname} ++;
  }
EOF
    super(command, name, sname)
  end
end

$cuda_meta_parameters = YAML::load_file(File.join(SRC_DIR,"cuda_meta_parameters.yaml"))
$cuda_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}
$cuda_exports_meta_parameters = YAML::load_file(File.join(SRC_DIR,"cuda_exports_meta_parameters.yaml"))
$cuda_exports_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$cuda_commands = cuda_funcs_e.collect { |func|
  Command::new(func)
}

$cuda_exports_commands = cuda_exports_funcs_e.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

CUDA_POINTER_NAMES = ($cuda_commands +
                      $cuda_exports_commands).collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h

dump_args = <<EOF
  _dump_kernel_args(f, kernelParams, extra);
EOF

[ "cuLaunchKernel",
  "cuLaunchKernel_ptsz",
  "cuLaunchKernelEx",
  "cuLaunchKernelEx_ptsz" ].each { |m|
  register_prologue m, dump_args
}

dump_args = <<EOF
  _dump_kernel_args(f, kernelParams, NULL);
EOF

[ "cuLaunchCooperativeKernel",
  "cuLaunchCooperativeKernel_ptsz" ].each { |m|
  register_prologue m, dump_args
}

dump_args = <<EOF
  if (nodeParams) {
    _dump_kernel_args(nodeParams->func, nodeParams->kernelParams, nodeParams->extra);
  }
EOF

[ "cuGraphAddKernelNode",
  "cuGraphExecKernelNodeSetParams" ].each { |m|
  register_prologue m, dump_args
}

register_prologue "cuLaunchCooperativeKernelMultiDevice", <<EOF
  if (launchParamsList) {
    for( unsigned int _i = 0; _i < numDevices; _i++) {
      _dump_kernel_args(launchParamsList[_i].function, launchParamsList[_i].kernelParams, NULL);
    }
  }
EOF

register_epilogue "cuGraphKernelNodeGetParams", <<EOF
  if (_retval == CUDA_SUCCESS && nodeParams) {
    _dump_kernel_args(nodeParams->func, nodeParams->kernelParams, nodeParams->extra);
  }
EOF

# Profiling

profiling_start = lambda { |stream|
  <<EOF
  CUevent _hStart = NULL;
  if (_do_profile)
    _hStart = _create_record_event(#{stream});
EOF
}

profiling_start_no_stream = profiling_start.call("NULL")
profiling_start_stream = profiling_start.call("hStream")

profiling_start_config = <<EOF
  CUevent _hStart = NULL;
  if (_do_profile && config)
    _hStart = _create_record_event(config->hStream);
EOF

profiling_stop = lambda { |stream|
  <<EOF
  if (_do_profile)
    _event_profile(_retval, _hStart, #{stream});
EOF
}

profiling_stop_no_stream = profiling_stop.call("NULL")
profiling_stop_stream = profiling_stop.call("hStream")

profiling_stop_config = <<EOF
  if (_do_profile && config)
    _event_profile(_retval, _hStart, config->hStream);
EOF

stream_commands = []
no_stream_commands = []
mem_commands = $cuda_commands.select { |c| c.name.match(/cuMemcpy|cuMemset/) }
mem_stream_commands = mem_commands.select { |c| c.name.match(/Async/) }
mem_no_stream_commands = mem_commands - mem_stream_commands
stream_commands += mem_stream_commands.collect(&:name)
no_stream_commands += mem_no_stream_commands.collect(&:name)

stream_commands += %w(
  cuMemPrefetchAsync
  cuMemPrefetchAsync_ptsz
  cuLaunchGridAsync
  cuLaunchKernel
  cuLaunchCooperativeKernel
  cuLaunchHostFunc
  cuLaunchKernel_ptsz
  cuLaunchCooperativeKernel_ptsz
  cuLaunchHostFunc_ptsz
)

no_stream_commands += %w(
  cuLaunch
  cuLaunchGrid
)

config_commands = %w(
  cuLaunchKernelEx
  cuLaunchKernelEx_ptsz
)

stream_commands.each { |m|
  register_prologue m, profiling_start_stream
  register_epilogue m, profiling_stop_stream
}

no_stream_commands.each { |m|
  register_prologue m, profiling_start_no_stream
  register_epilogue m, profiling_stop_no_stream
}

config_commands.each { |m|
  register_prologue m, profiling_start_config
  register_epilogue m, profiling_stop_config
}

# if a context is to be destroyed we must attempt to get profiling event results
[ "cuCtxDestroy",
  "cuCtxDestroy_v2" ].each { |m|
  register_prologue m, <<EOF
  if (ctx) {
    _context_event_cleanup(ctx);
  }
EOF
}

register_epilogue "cuDevicePrimaryCtxRetain", <<EOF
  if (_do_profile && _retval == CUDA_SUCCESS && *pctx) {
    _primary_context_retain(dev, *pctx);
  }
EOF

[ "cuDevicePrimaryCtxRelease_v2",
  "cuDevicePrimaryCtxRelease" ].each { |m|
  register_prologue m, <<EOF
  if (_do_profile) {
    _primary_context_release(dev);
  }
EOF
}

[ "cuDevicePrimaryCtxReset_v2",
  "cuDevicePrimaryCtxReset" ].each { |m|
  register_prologue m, <<EOF
  if (_do_profile) {
    _primary_context_reset(dev);
  }
EOF
}

# Export tracing
register_epilogue "cuGetExportTable", <<EOF
  if (_do_trace_export_tables && _retval == CUDA_SUCCESS) {
    const void *tmp = _wrap_and_cache_export_table(*ppExportTable, pExportTableId);
    *ppExportTable = tmp;
  }
EOF

register_epilogue "cuInit", <<EOF
  if (_retval == CUDA_SUCCESS) {
    _init_cuda();
  }
EOF

# cuGetProcAddress*
command_names = $cuda_commands.collect(&:name).to_set
pt_condition = "((flags & CU_GET_PROC_ADDRESS_PER_THREAD_DEFAULT_STREAM) && !(flags & CU_GET_PROC_ADDRESS_LEGACY_STREAM))"
normal_condition = "((flags & CU_GET_PROC_ADDRESS_LEGACY_STREAM) || !(flags & CU_GET_PROC_ADDRESS_PER_THREAD_DEFAULT_STREAM))"

register_proc_callbacks = lambda { |method|
  str = <<EOF
  const int pt_condition = #{pt_condition};
  const int normal_condition = #{normal_condition};
  if (_retval == CUDA_SUCCESS && pfn && *pfn) {
EOF
  str << $cuda_api_versions_yaml.map { |name, suffixes|
    suffixes.map { |suffix, versions|
      versions.map.with_index { |version, i|
        fullname = "#{name}#{versions.size - i > 1 ? "_v#{versions.size - i}" : ""}#{suffix}"
        fullname = "#{name}_v2#{suffix}" unless command_names.include?(fullname)
        sstr = <<EOF
    if ((!_trace_from_start() || tracepoint_enabled(lttng_ust_cuda, #{fullname}_#{START})) #{suffixes.size > 1 ? "&& #{suffix ? "pt_condition" : "normal_condition" } " : ""}&& cudaVersion >= #{version} && strcmp(symbol, "#{name}") == 0) {
      wrap_#{fullname}(pfn);
    }
EOF
      }
    }
  }.flatten.join(<<EOF)
    else
EOF
#  str << $cuda_commands.collect { |c|
#    <<EOF
#    if (tracepoint_enabled(lttng_ust_cuda, #{c.name}_#{START}) && strcmp(symbol, "#{c.name}") == 0) {
#      wrap_#{c.name}(pfn);
#    }
#EOF
#  }.join(<<EOF)
#    else
#EOF
  str << <<EOF
  }
EOF

  register_epilogue method, str
}

register_proc_callbacks.call("cuGetProcAddress")
register_proc_callbacks.call("cuGetProcAddress_v2")
