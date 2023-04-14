require 'yaml'
require 'pp'
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

$cuda_api_yaml = YAML::load_file("cuda_api.yaml")
$cuda_exports_api_yaml = YAML::load_file("cuda_exports_api.yaml")

$cuda_api = YAMLCAst.from_yaml_ast($cuda_api_yaml)
$cuda_exports_api = YAMLCAst.from_yaml_ast($cuda_exports_api_yaml)

cuda_funcs_e = $cuda_api["functions"]
cuda_exports_funcs_e = $cuda_exports_api["functions"]

cuda_types_e = $cuda_api["typedefs"]
cuda_exports_type_e = $cuda_exports_api["typedefs"]

all_types = cuda_types_e + cuda_exports_type_e
all_structs = $cuda_api["structs"] + $cuda_exports_api["structs"]

OBJECT_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && OBJECT_TYPES.include?(t.type.name)
    OBJECT_TYPES.push t.name
  end
}

def transitive_closure(types, arr)
  sz = arr.size
  loop do
    arr.concat( types.filter_map { |t|
      t.name if t.type.kind_of?(YAMLCAst::CustomType) && arr.include?(t.type.name)
    } ).uniq!
    break if sz == arr.size
    sz = arr.size
  end
end

def transitive_closure_map(types, map)
  sz = map.size
  loop do
    types.select { |t|
      t.type.kind_of?(YAMLCAst::CustomType) && map.include?(t.type.name)
    }.each { |t| map[t.name] = map[t.type.name] }
    break if sz == map.size
    sz = map.size
  end
end

INT_TYPES = %w(size_t uint32_t cuuint32_t uint64_t cuuint64_t int short char
                      CUdevice CUdevice_v1
                      CUdeviceptr CUdeviceptr_v1 CUdeviceptr_v2
                      CUtexObject CUtexObject_v1 CUsurfObject CUsurfObject_v1
                      CUmemGenericAllocationHandle
                      VdpDevice VdpFuncId VdpVideoSurface VdpOutputSurface VdpStatus)
INT_TYPES.concat [ "long long", "unsigned long long", "unsigned long long int", "unsigned int", "unsigned short", "unsigned char" ]
CUDA_FLOAT_SCALARS = %w(float double)
CUDA_SCALARS = INT_TYPES + CUDA_FLOAT_SCALARS
ENUM_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Enum }.collect { |t| t.name }
transitive_closure(all_types, ENUM_TYPES)
STRUCT_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.collect { |t| t.name }
transitive_closure(all_types, STRUCT_TYPES)
UNION_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Union }.collect { |t| t.name }
transitive_closure(all_types, UNION_TYPES)
POINTER_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }

STRUCT_MAP = {}
all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.each { |t|
  if t.type.members
    STRUCT_MAP[t.name] = t.type.members
  else
    STRUCT_MAP[t.name] = all_structs.find { |str| str.name == t.type.name }.members
  end
}
transitive_closure_map(all_types, STRUCT_MAP)

INIT_FUNCTIONS = /cuInit|cuDriverGetVersion|cuGetExportTable|cuDeviceGetCount/

FFI_TYPE_MAP =  {
 "unsigned char" => "ffi_type_uint8",
 "char" => "ffi_type_sint8",
 "unsigned short" => "ffi_type_uint16",
 "short" => "ffi_type_sint16",
 "unsigned int" => "ffi_type_uint32",
 "int" => "ffi_type_sint32",
 "unsigned long long" => "ffi_type_uint64",
 "long long" => "ffi_type_sint64",
 "uint32_t" => "ffi_type_uint32",
 "cuuint32_t" => "ffi_type_uint32",
 "uint64_t" => "ffi_type_uint64",
 "cuuint64_t" => "ffi_type_uint64",
 "float" => "ffi_type_float",
 "double" => "ffi_type_double",
 "size_t" => "ffi_type_pointer",
 "CUdevice" => "ffi_type_uint32",
 "CUdeviceptr_v1" => "ffi_type_uint32",
 "CUdeviceptr" => "ffi_type_pointer",
 "CUtexObject" => "ffi_type_uint64",
 "CUsurfObject" => "ffi_type_uint64",
}

OBJECT_TYPES.each { |o|
  FFI_TYPE_MAP[o] = "ffi_type_pointer"
}

ENUM_TYPES.each { |e|
  FFI_TYPE_MAP[e] = "ffi_type_sint32"
}

HEX_INT_TYPES = %w( CUdeviceptr )

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
    tracepoint(lttng_ust_cuda, cuGetExportTable_exit, ppExportTable, pExportTableId, _retval);
    *ppExportTable = tmp;
    return _retval;
  }
EOF
