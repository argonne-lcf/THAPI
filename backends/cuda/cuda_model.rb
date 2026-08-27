require 'yaml'
require 'pp'
require 'set'
require_relative '../../utils/api_model'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'

SRC_DIR = ENV['SRC_DIR'] || '.'

cuda_api = ApiModel.load_file('cuda_api.yaml')
cuda_exports_api = ApiModel.load_file('cuda_exports_api.yaml')

# The driver and its export tables are traced as one API: an export-table
# typedef can name a driver struct, so they have to be classified together.
API = cuda_api + cuda_exports_api

# CUdeviceptr is a device address carried in an integer, so it reads as hex
# rather than as a decimal that means nothing to anyone.
TYPE_CLASSES = find_all_types(API.types, extra_hex_ints: ['CUdeviceptr'])
gen_ffi_type_map(API.types, TYPE_CLASSES)

CONTEXT = BackendContext.new(
  result_name: 'cuResult',
  # cuda's pointers start at a _uninit trampoline that calls _init_tracer(), so
  # no function is singled out as the initializer and Command#init? is never
  # asked.
  init_functions: nil,
  struct_map: API.struct_map,
  type_classes: TYPE_CLASSES
)

class TracepointParameter
  attr_reader :name, :type, :init, :after

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
    sname = "_#{name.split('->').join(MEMBER_SEPARATOR)}_size"
    checks = check_for_null("#{name}")
    command.tracepoint_parameters.push TracepointParameter.new(sname, 'size_t', <<EOF, after: true)
  #{sname} = 0;
  if(#{checks.join(' && ')}) {
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
    sname = "_#{name.split('->').join(MEMBER_SEPARATOR)}_size"
    checks = check_for_null("#{name}")
    command.tracepoint_parameters.push TracepointParameter.new(sname, 'size_t', <<EOF)
  #{sname} = 0;
  if(#{checks.join(' && ')}) {
    while(#{name}[#{sname}] != 0) {
      #{sname} += 2;
    }
    #{sname} ++;
  }
EOF
    super(command, name, sname)
  end
end

meta_parameters = load_meta_parameters('cuda_meta_parameters.yaml', 'cuda_exports_meta_parameters.yaml')

# The driver and its export tables are one API but two LTTng providers, so the
# commands are grouped by the provider that will carry them.
COMMANDS = CommandIndex.new(
  { lttng_ust_cuda: cuda_api.functions, lttng_ust_cuda_exports: cuda_exports_api.functions }.transform_values do |funcs|
    funcs.collect { |func| Command.new(func, context: CONTEXT, meta_parameters: meta_parameters[func.name]) }
  end
)

check_meta_parameters(meta_parameters, COMMANDS)

CUDA_POINTER_NAMES = COMMANDS.collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end.to_h

dump_args = <<EOF
  _dump_kernel_args(f, kernelParams, extra);
EOF

%w[cuLaunchKernel
   cuLaunchKernel_ptsz
   cuLaunchKernelEx
   cuLaunchKernelEx_ptsz].each do |m|
  COMMANDS.add_prologue m, dump_args
end

dump_args = <<EOF
  _dump_kernel_args(f, kernelParams, NULL);
EOF

%w[cuLaunchCooperativeKernel
   cuLaunchCooperativeKernel_ptsz].each do |m|
  COMMANDS.add_prologue m, dump_args
end

dump_args = <<EOF
  if (nodeParams) {
    _dump_kernel_args(nodeParams->func, nodeParams->kernelParams, nodeParams->extra);
  }
EOF

%w[cuGraphAddKernelNode
   cuGraphExecKernelNodeSetParams].each do |m|
  COMMANDS.add_prologue m, dump_args
end

COMMANDS.add_prologue 'cuLaunchCooperativeKernelMultiDevice', <<EOF
  if (launchParamsList) {
    for( unsigned int _i = 0; _i < numDevices; _i++) {
      _dump_kernel_args(launchParamsList[_i].function, launchParamsList[_i].kernelParams, NULL);
    }
  }
EOF

COMMANDS.add_epilogue 'cuGraphKernelNodeGetParams', <<EOF
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

profiling_start_no_stream = profiling_start.call('NULL')
profiling_start_stream = profiling_start.call('hStream')

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

profiling_stop_no_stream = profiling_stop.call('NULL')
profiling_stop_stream = profiling_stop.call('hStream')

profiling_stop_config = <<EOF
  if (_do_profile && config)
    _event_profile(_retval, _hStart, config->hStream);
EOF

stream_commands = []
no_stream_commands = []
mem_commands = COMMANDS.groups[:lttng_ust_cuda].select { |c| c.name.match(/cuMemcpy|cuMemset/) }
mem_stream_commands = mem_commands.select { |c| c.name.match(/Async/) }
mem_no_stream_commands = mem_commands - mem_stream_commands
stream_commands += mem_stream_commands.collect(&:name)
no_stream_commands += mem_no_stream_commands.collect(&:name)

stream_commands += %w[
  cuMemPrefetchAsync
  cuMemPrefetchAsync_ptsz
  cuLaunchGridAsync
  cuLaunchKernel
  cuLaunchCooperativeKernel
  cuLaunchHostFunc
  cuLaunchKernel_ptsz
  cuLaunchCooperativeKernel_ptsz
  cuLaunchHostFunc_ptsz
]

no_stream_commands += %w[
  cuLaunch
  cuLaunchGrid
]

config_commands = %w[
  cuLaunchKernelEx
  cuLaunchKernelEx_ptsz
]

stream_commands.each do |m|
  COMMANDS.add_prologue m, profiling_start_stream
  COMMANDS.add_epilogue m, profiling_stop_stream
end

no_stream_commands.each do |m|
  COMMANDS.add_prologue m, profiling_start_no_stream
  COMMANDS.add_epilogue m, profiling_stop_no_stream
end

config_commands.each do |m|
  COMMANDS.add_prologue m, profiling_start_config
  COMMANDS.add_epilogue m, profiling_stop_config
end

# if a context is to be destroyed we must attempt to get profiling event results
%w[cuCtxDestroy
   cuCtxDestroy_v2].each do |m|
  COMMANDS.add_prologue m, <<EOF
  if (ctx) {
    _context_event_cleanup(ctx);
  }
EOF
end

COMMANDS.add_epilogue 'cuDevicePrimaryCtxRetain', <<EOF
  if (_do_profile && _retval == CUDA_SUCCESS && *pctx) {
    _primary_context_retain(dev, *pctx);
  }
EOF

%w[cuDevicePrimaryCtxRelease_v2
   cuDevicePrimaryCtxRelease].each do |m|
  COMMANDS.add_prologue m, <<EOF
  if (_do_profile) {
    _primary_context_release(dev);
  }
EOF
end

%w[cuDevicePrimaryCtxReset_v2
   cuDevicePrimaryCtxReset].each do |m|
  COMMANDS.add_prologue m, <<EOF
  if (_do_profile) {
    _primary_context_reset(dev);
  }
EOF
end

# Export tracing
COMMANDS.add_epilogue 'cuGetExportTable', <<EOF
  if (_do_trace_export_tables && _retval == CUDA_SUCCESS) {
    const void *tmp = _wrap_and_cache_export_table(*ppExportTable, pExportTableId);
    *ppExportTable = tmp;
  }
EOF

COMMANDS.add_epilogue 'cuInit', <<EOF
  if (_retval == CUDA_SUCCESS) {
    _init_cuda();
  }
EOF

# cuGetProcAddress*
# Not an api.yaml: a plain name -> suffix -> versions map, used only here.
api_versions = YAML.load_file('cuda_api_versions.yaml')
command_names = COMMANDS.groups[:lttng_ust_cuda].collect(&:name).to_set
pt_condition = '((flags & CU_GET_PROC_ADDRESS_PER_THREAD_DEFAULT_STREAM) && !(flags & CU_GET_PROC_ADDRESS_LEGACY_STREAM))'
normal_condition = '((flags & CU_GET_PROC_ADDRESS_LEGACY_STREAM) || !(flags & CU_GET_PROC_ADDRESS_PER_THREAD_DEFAULT_STREAM))'

register_proc_callbacks = lambda { |method|
  str = <<EOF
  const int pt_condition = #{pt_condition};
  const int normal_condition = #{normal_condition};
  if (_retval == CUDA_SUCCESS && cudaVersion > CUDA_VERSION) {
    fprintf(stderr, "THAPI: CUDA version %d is unsupported, could not wrap %s symbol\\n", cudaVersion, symbol);
  } else if (_retval == CUDA_SUCCESS && pfn && *pfn) {
EOF
  str << api_versions.map { |name, suffixes|
    suffixes.map { |suffix, versions|
      versions.map.with_index { |version, i|
        fullname = "#{name}#{"_v#{versions.size - i}" if versions.size - i > 1}#{suffix}"
        fullname = "#{name}_v2#{suffix}" unless command_names.include?(fullname)
        <<EOF
    if (tracepoint_enabled(lttng_ust_cuda, #{fullname}_#{START}) #{if suffixes.size > 1
                                                                     "&& #{suffix ? 'pt_condition' : 'normal_condition'} "
                                                                   end}&& cudaVersion >= #{version} && strcmp(symbol, "#{name}") == 0) {
      wrap_#{fullname}(pfn);
    }
EOF
      }
    }
  }.flatten.join(<<EOF)
    else
EOF
  str << <<EOF
  }
EOF

  COMMANDS.add_epilogue method, str
}

register_proc_callbacks.call('cuGetProcAddress')
register_proc_callbacks.call('cuGetProcAddress_v2')
