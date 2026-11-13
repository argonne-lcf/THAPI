require_relative '../../utils/backend_model'

# What the registry says and the headers cannot (gen_opencl_xml_yaml.rb).
OPENCL_XML = yaml_load_file_cached('opencl_xml.yaml')

# OpenCL traces each scalar typedef as the cl_* (or C) type the registry says
# it is (type_map).
API = ApiModel.load_file('opencl_api.yaml', trace_names: OPENCL_XML['type_map']).register_ffi_types
# The libffi type OpenCL's closures have always named a C int with.
FFI_TYPE_MAP['int'] = 'ffi_type_sint'

# The field a call's return value is recorded in: the error code, or the
# object it creates, named after its type.
def opencl_result_field(command)
  type = command.type.to_s
  case type
  when 'cl_int' then 'errcode_ret_val'
  when /\*/ then '_retval'
  else type.sub(/\A(?:CL|cl_)/, '').sub(/(?:KHR|_khr)\z/, '')
  end
end

CONTEXT = BackendContext.for(
  API, result_name: '_retval',
       result_field: method(:opencl_result_field),
       duration: true,
       xml_arg_spelling: true,
       legacy_blobs: true,
       null_test: ->(pointer) { "#{pointer} != NULL" },
       pointer_type_suffix: '_ptr_t',
       declarations: OPENCL_XML['declarations'],
       null_guard: lambda { |checks, expr, default, cast: false|
         guard = "#{checks.map { |c| "#{c} == NULL" }.join(' || ')} ? #{default} : #{expr}"
         cast ? "(#{guard})" : guard
       },
       init_functions: /clGetPlatformIDs|clGetPlatformInfo|clGetDeviceIDs|clCreateContext|clCreateContextFromType|clUnloadPlatformCompiler|clGetExtensionFunctionAddressForPlatform|clGetExtensionFunctionAddress|clGetGLContextInfoKHR/
)

# A device-partition property list (clCreateSubDevices, clCreateSubDevicesEXT):
# keys, each followed by one value or by a list of values ended by its own
# terminator, and a 0 after the last key. The core API and cl_ext_device_fission
# spell the keys with different constants; cl_intel_device_partition_by_names
# adds a key to the core list.
class DeviceFissionPropertyList < InArray
  # Key => terminator of its value list. Every other key takes one value, so a
  # key that takes a list must be here, or the walk reads past the array.
  VALUE_LISTS = {
    'cl_device_partition_property' => {
      'CL_DEVICE_PARTITION_BY_COUNTS' => 'CL_DEVICE_PARTITION_BY_COUNTS_LIST_END',
      'CL_DEVICE_PARTITION_BY_NAMES_INTEL' => 'CL_PARTITION_BY_NAMES_LIST_END_INTEL',
    },
    'cl_device_partition_property_ext' => {
      'CL_DEVICE_PARTITION_BY_COUNTS_EXT' => 'CL_PARTITION_BY_COUNTS_LIST_END_EXT',
      'CL_DEVICE_PARTITION_BY_NAMES_EXT' => 'CL_PARTITION_BY_NAMES_LIST_END_EXT',
    },
  }.freeze

  def initialize(command, name)
    sname = "_#{name}_size"
    lists = VALUE_LISTS.fetch(command[name].type.type.name)
    cases = lists.map do |key, terminator|
      "case #{key}:\n        while(#{name}[#{sname}++] != #{terminator});\n        break;"
    end
    command.tracepoint_parameters.push TracepointParameter.new(
      name: sname, type: YAMLCAst::CustomType.new(name: 'size_t'), dir: :start, fill: <<C
  #{sname} = 0;
  if(#{name} != NULL) {
    while(#{name}[#{sname}] != 0) {
      switch(#{name}[#{sname}++]) {
      #{cases.join("\n      ")}
      default:
        #{sname}++; //value
        break;
      }
    }
    #{sname}++; //terminator
  }
C
    )
    super(command, name, sname)
  end
end

# The registry names the parameters: the tracepoints keep its names where a
# header spells one differently (the oneAPI cl.h's clGetKernelArgInfo says
# `arg_indx` for `arg_index`).
def with_registry_names(func)
  names = OPENCL_XML['declarations'].fetch(func.name)['params'].filter_map { |p| p['name'] }
  params = func.type.params.to_a
  return func if params.map(&:name) == names

  params = params.zip(names).map { |p, n| YAMLCAst::Declaration.new(name: n, type: p.type) }
  type = YAMLCAst::Function.new(type: func.type.type, params: params, var_args: func.type.var_args)
  YAMLCAst::Declaration.new(name: func.name, type: type, storage: func.storage)
end

# Whether a function takes a pointer named `name`.
def pointer_parameter?(func, name)
  func.type.params.to_a.any? { |p| p.name == name && p.type.is_a?(YAMLCAst::Pointer) }
end

# A function's meta-parameters: the ones OpenCL's naming conventions imply, in
# the order the tracepoints list them, then the hand-written ones, then the key
# a clGet*Info call queries. errcode_ret and param_value_size_ret need no NULL
# check: the prologues below point them at a local when the caller passes NULL.
def opencl_meta_parameters(func, hand)
  params = func.type.params.to_a.to_h { |p| [p.name, p.type] }
  rows = []
  rows << [InArray, %w[event_wait_list num_events_in_wait_list]] if params.key?('event_wait_list')
  rows << [OutScalar, ['errcode_ret'], { nocheck: true }] if pointer_parameter?(func, 'errcode_ret')
  rows << [OutScalar, ['param_value_size_ret'], { nocheck: true }] if pointer_parameter?(func, 'param_value_size_ret')
  # clSetKernelExecInfo's param_value is an input.
  if params.key?('param_value') && func.name != 'clSetKernelExecInfo'
    rows << [OutArray, %w[param_value param_value_size]]
  end
  rows << [OutScalar, ['event']] if pointer_parameter?(func, 'event')
  rows += hand[func.name]
  if func.name.match?(/clGet\w*?Info/) && params.key?('param_name')
    rows << [OutLTTng, ['param_name', 'ctf_integer_hex', params['param_name'].to_s, '_param_name', 'param_name']]
  end
  rows
end

functions = API.functions.to_h { |f| [f.name, f] }
traced = OPENCL_XML['functions'].values.flatten.map { |n| with_registry_names(functions.fetch(n)) }
hand = load_meta_parameters('opencl_meta_parameters.yaml')
check_meta_parameters(hand, traced)
COMMANDS = build_command_index(
  { lttng_ust_opencl: traced }, context: CONTEXT, spec: traced.to_h { |f| [f.name, opencl_meta_parameters(f, hand)] }
)

CORE_COMMANDS = OPENCL_XML['functions']['core'].map { |n| COMMANDS[n] }
EXTENSION_COMMANDS = OPENCL_XML['functions']['extension'].map { |n| COMMANDS[n] }

# An extension is reached through libffi rather than dlsym, so its pointer
# keeps the header's own spelling instead of the upper-snake macro.
OPENCL_POINTER_NAMES = COMMANDS.pointer_names { |c, name| name if EXTENSION_COMMANDS.include?(c) }

# Core-API functions named like extensions: clGetExtensionFunctionAddress hands
# them back too. The libraries export the GL ones, not the KHR/EXT ones: the
# tracer looks those up with clGetExtensionFunctionAddress.
CORE_EXTENSION_COMMANDS = CORE_COMMANDS.select { |c| c.name.match(/KHR$|EXT$|GL/) }
LOOKED_UP_COMMANDS = CORE_EXTENSION_COMMANDS.select { |c| c.name.match(/KHR$|EXT$/) }

def opencl_returns_event?(c)
  c.type.to_s == 'cl_event'
end

# A call that creates an event, returned or through its `event` argument.
def opencl_event?(c)
  opencl_returns_event?(c) || pointer_parameter?(c.function, 'event')
end

COMMANDS.select { |c| pointer_parameter?(c.function, 'errcode_ret') }.each do |c|
  c.add_prologue <<EOF
  cl_int _errcode_ret_force;
  if (!errcode_ret)
    errcode_ret = &_errcode_ret_force;
EOF
end

COMMANDS.select { |c| pointer_parameter?(c.function, 'param_value_size_ret') }.each do |c|
  c.add_prologue <<EOF
  size_t _new_param_value_size;
  if (!param_value_size_ret)
    param_value_size_ret = &_new_param_value_size;
EOF
  c.add_epilogue <<EOF
  param_value_size = (param_value_size <= *param_value_size_ret ? param_value_size : *param_value_size_ret );
EOF
end

COMMANDS.each do |c|
  next unless c.name.match 'clEnqueue'

  c.add_prologue <<EOF
  int64_t _enqueue_counter = 0;
  if (do_dump) {
    pthread_mutex_lock(&enqueue_counter_mutex);
    _enqueue_counter = enqueue_counter;
    enqueue_counter++;
    pthread_mutex_unlock(&enqueue_counter_mutex);
    tracepoint(lttng_ust_opencl_dump, enqueue_counter, _enqueue_counter);
  }
EOF
end

COMMANDS.add_epilogue 'clCreateKernel', <<EOF
  if (do_dump && _retval != NULL) {
    add_kernel(_retval);
  }
EOF

COMMANDS.add_epilogue 'clSetKernelArg', <<EOF
  if (do_dump && _retval == CL_SUCCESS) {
    add_kernel_arg(kernel, arg_index, arg_size, arg_value, 0);
  }
EOF

COMMANDS.add_epilogue 'clSetKernelArgSVMPointer', <<EOF
  if (do_dump && _retval == CL_SUCCESS) {
    add_kernel_arg(kernel, arg_index, sizeof(arg_value), arg_value, 1);
  }
EOF

COMMANDS.add_epilogue 'clSVMAlloc', <<EOF
  if (do_dump && _retval != NULL) {
    add_svmptr(_retval, size);
  }
EOF

COMMANDS.add_prologue 'clSVMFree', <<EOF
  if (do_dump && svm_pointer != NULL) {
    remove_svmptr(svm_pointer);
  }
EOF

str = <<EOF
  int _dump_release_events = 0;
  int _dump_release_event = 0;
  cl_event extra_event;
  if (do_dump && command_queue != NULL && kernel != NULL && _enqueue_counter >= dump_start && _enqueue_counter <= dump_end) {
    cl_command_queue_properties properties;
    #{OPENCL_POINTER_NAMES[COMMANDS['clGetCommandQueueInfo']]}(command_queue, CL_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &properties, NULL);
    _dump_release_events = dump_kernel_args(command_queue, kernel, _enqueue_counter, properties, &num_events_in_wait_list, (cl_event **)&event_wait_list);
    if ((properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) && event == NULL) {
      event = &extra_event;
      _dump_release_event = 1;
    }
  }
EOF
COMMANDS.add_prologue 'clEnqueueNDRangeKernel', str
COMMANDS.add_prologue 'clEnqueueNDRangeKernelINTEL', str

str = <<EOF
  if (do_dump && _dump_release_events) {
    for (cl_uint event_index = 0; event_index < num_events_in_wait_list; event_index++) {
      #{OPENCL_POINTER_NAMES[COMMANDS['clReleaseEvent']]}(event_wait_list[event_index]);
    }
    free((void *)event_wait_list);
  }
EOF
COMMANDS.add_epilogue 'clEnqueueNDRangeKernel', str
COMMANDS.add_epilogue 'clEnqueueNDRangeKernelINTEL', str

COMMANDS.add_prologue 'clCreateBuffer', <<EOF
  if (do_dump) {
    flags &= ~CL_MEM_HOST_WRITE_ONLY;
    flags &= ~CL_MEM_HOST_NO_ACCESS;
  }
EOF

COMMANDS.add_epilogue 'clCreateBuffer', <<EOF
  if (do_dump && _retval != NULL) {
    add_buffer(_retval, size);
  }
EOF

COMMANDS.add_prologue 'clCreateCommandQueue', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling)) {
    properties |= CL_QUEUE_PROFILING_ENABLE;
  }
EOF

COMMANDS.add_prologue 'clCreateCommandQueueWithProperties', <<EOF
  cl_queue_properties *_profiling_properties = NULL;
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling)) {
    int _found_queue_properties = 0;
    int _queue_properties_index = 0;
    int _properties_count = 0;
    if (properties) {
      while(properties[_properties_count]) {
        if (properties[_properties_count] == CL_QUEUE_PROPERTIES){
          _found_queue_properties = 1;
          _queue_properties_index = _properties_count;
        }
        _properties_count += 2;
      }
      _properties_count++;
      if (!_found_queue_properties)
        _properties_count +=2;
    } else
      _properties_count = 3;
    _profiling_properties = (cl_queue_properties *)malloc(_properties_count*sizeof(cl_queue_properties));
    if (_profiling_properties) {
      if (properties) {
        int _i = 0;
        while(properties[_i]) {
          _profiling_properties[_i] = properties[_i];
          _profiling_properties[_i+1] = properties[_i+1];
          _i += 2;
        }
        if (_found_queue_properties) {
          _profiling_properties[_queue_properties_index+1] |= CL_QUEUE_PROFILING_ENABLE;
          _profiling_properties[_i] = 0;
        } else {
          _profiling_properties[_i++] = CL_QUEUE_PROPERTIES;
          _profiling_properties[_i++] = CL_QUEUE_PROFILING_ENABLE;
          _profiling_properties[_i] = 0;
        }
      } else {
        _profiling_properties[0] = CL_QUEUE_PROPERTIES;
        _profiling_properties[1] = CL_QUEUE_PROFILING_ENABLE;
        _profiling_properties[2] = 0;
      }
      properties = _profiling_properties;
    }
  }
EOF

COMMANDS.add_epilogue 'clCreateCommandQueueWithProperties', <<EOF
  if (_profiling_properties) free(_profiling_properties);
EOF

COMMANDS.add_prologue 'clCreateProgramWithSource', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_source, program_string) && strings != NULL) {
    cl_uint index;
    for (index = 0; index < count; index++) {
      size_t length = 0;
      char path[sizeof(SOURCE_TEMPLATE)];
      strncpy(path, SOURCE_TEMPLATE, sizeof(path));
      if ( strings[index] != NULL ) {
        if (lengths == NULL || lengths[index] == 0)
          length = strlen(strings[index]);
        else
          length = lengths[index];
      }
      create_file_and_write(path, length, strings[index]);
      do_tracepoint(lttng_ust_opencl_source, program_string, index, length, path);
    }
  }
EOF

COMMANDS.add_prologue 'clCreateProgramWithBinary', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_source, program_binary) && binaries != NULL && lengths != NULL) {
    cl_uint index;
    for (index = 0; index < num_devices; index++) {
      char path[sizeof(BIN_SOURCE_TEMPLATE)];
      strncpy(path, BIN_SOURCE_TEMPLATE, sizeof(path));
      create_file_and_write(path, lengths[index], binaries[index]);
      do_tracepoint(lttng_ust_opencl_source, program_binary, index, lengths[index], path);
    }
  }
EOF

%w[clCreateProgramWithIL clCreateProgramWithILKHR].each do |func|
  COMMANDS.add_prologue func, <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_source, program_il) && il != NULL) {
    char path[sizeof(IL_SOURCE_TEMPLATE)];
    strncpy(path, IL_SOURCE_TEMPLATE, sizeof(path));
    create_file_and_write(path, length, il);
    do_tracepoint(lttng_ust_opencl_source, program_il, length, path);
  }
EOF
end

# Kernel argument infos need the program built with -cl-kernel-arg-info.
# `guard` is what else must hold, `program` the program whose version decides.
add_kernel_arg_info_option = lambda do |func, program: 'program', guard: ''|
  COMMANDS.add_prologue func, <<EOF
  int _free_options = 0;
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info)#{guard}) {
    struct opencl_version version = {1, 0};
    get_program_platform_version(#{program}, &version);
    if (compare_opencl_version(&version, &opencl_version_1_2) >= 0) {
      if (options) {
        if (!strstr(options, "-cl-kernel-arg-info")) {
          size_t sz = strlen(options) + strlen("-cl-kernel-arg-info") + 2;
          char * new_options = (char *)malloc(sz);
          if (new_options) {
            snprintf(new_options, sz, "%s %s", options, "-cl-kernel-arg-info");
            _free_options = 1;
            options = new_options;
          }
        }
      } else {
        options = "-cl-kernel-arg-info";
      }
    }
  }
EOF
end
add_kernel_arg_info_option.call('clBuildProgram')
add_kernel_arg_info_option.call('clCompileProgram')
add_kernel_arg_info_option.call('clLinkProgram', program: '*input_programs',
                                                 guard: ' && input_programs && num_input_programs > 0')

str = <<EOF
  if (_free_options)
    free((char *)options);
EOF
COMMANDS.add_epilogue 'clBuildProgram', str
COMMANDS.add_epilogue 'clCompileProgram', str
COMMANDS.add_epilogue 'clLinkProgram', str

l = lambda { |func, name: 'pfn_notify', extra_conditions: nil|
  COMMANDS.add_prologue func, <<EOF
  struct #{func}_callback_payload *_payload = NULL;
  if ((tracepoint_enabled(lttng_ust_opencl, #{func}_callback_#{START})#{if extra_conditions
                                                                          " || #{extra_conditions.join(' || ')}"
                                                                        end}) && #{name}) {
    _payload = (struct #{func}_callback_payload *)malloc(sizeof(struct #{func}_callback_payload));
    _payload->#{name} = #{name};
    _payload->user_data = user_data;
    user_data = (void *)_payload;
    #{name} = &#{func}_callback;
  }
EOF
}
program_conditions = ['tracepoint_enabled(lttng_ust_opencl_build, binaries)',
                      'tracepoint_enabled(lttng_ust_opencl_build, infos)']
l.call('clBuildProgram', extra_conditions: program_conditions)
l.call('clCompileProgram',
       extra_conditions: ['tracepoint_enabled(lttng_ust_opencl_build, objects)',
                          'tracepoint_enabled(lttng_ust_opencl_build, infos)'])
l.call('clLinkProgram', extra_conditions: program_conditions)
l.call('clCreateContext')
l.call('clCreateContextFromType')
l.call('clSetMemObjectDestructorCallback')
l.call('clSetProgramReleaseCallback')
l.call('clSetEventCallback')
l.call('clEnqueueSVMFree', name: 'pfn_free_func')

str = <<EOF
  if (_payload && _retval != CL_SUCCESS)
    free(_payload);
EOF
COMMANDS.add_epilogue 'clBuildProgram', str
COMMANDS.add_epilogue 'clCompileProgram', str
COMMANDS.add_epilogue 'clSetMemObjectDestructorCallback', str
COMMANDS.add_epilogue 'clSetProgramReleaseCallback', str
COMMANDS.add_epilogue 'clSetEventCallback', str
COMMANDS.add_epilogue 'clEnqueueSVMFree', str
str = <<EOF
  if (_payload && !_retval)
    free(_payload);
EOF
COMMANDS.add_epilogue 'clLinkProgram', str
COMMANDS.add_epilogue 'clCreateContext', str
COMMANDS.add_epilogue 'clCreateContextFromType', str

COMMANDS.add_epilogue 'clBuildProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, binaries) && !pfn_notify) {
    dump_program_binaries(program);
  }
EOF
COMMANDS.add_epilogue 'clCompileProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, objects) && !pfn_notify) {
    dump_program_objects(program);
  }
EOF
COMMANDS.add_epilogue 'clLinkProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, binaries) && _retval && !pfn_notify) {
    dump_program_binaries(_retval);
  }
EOF

str = <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, infos) && !pfn_notify) {
    dump_program_build_infos(program);
  }
EOF
COMMANDS.add_epilogue 'clBuildProgram', str
COMMANDS.add_epilogue 'clCompileProgram', str
COMMANDS.add_epilogue 'clLinkProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_build, infos) && _retval && !pfn_notify) {
    dump_program_build_infos(_retval);
  }
EOF

COMMANDS.add_epilogue 'clCreateKernel', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, kernel_info)) {
    dump_kernel_info(_retval);
  }
EOF

COMMANDS.add_epilogue 'clCreateKernel', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info) && _retval != NULL) {
    dump_kernel_arguments(program, _retval);
  }
EOF

COMMANDS.add_prologue 'clCreateKernelsInProgram', <<EOF
  cl_uint n_k = 0;
  if (tracepoint_enabled(lttng_ust_opencl_arguments, kernel_info) && !num_kernels_ret && kernels) {
    num_kernels_ret = &n_k;
  }
EOF

COMMANDS.add_epilogue 'clCreateKernelsInProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, kernel_info) && _retval == CL_SUCCESS && kernels) {
    for (cl_uint i = 0; i < *num_kernels_ret; i++) {
      dump_kernel_info(kernels[i]);
    }
  }
EOF

COMMANDS.add_prologue 'clCreateKernelsInProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info) && !num_kernels_ret && kernels) {
    num_kernels_ret = &n_k;
  }
EOF

COMMANDS.add_epilogue 'clCreateKernelsInProgram', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_arguments, argument_info) && _retval == CL_SUCCESS && kernels) {
    for (cl_uint i = 0; i < *num_kernels_ret; i++) {
      dump_kernel_arguments(program, kernels[i]);
    }
  }
EOF

COMMANDS.add_prologue 'clGetDeviceIDs', <<EOF
  cl_uint n_e;
  if (tracepoint_enabled(lttng_ust_opencl_devices, device_name) && !num_devices && devices) {
    num_devices = &n_e;
  }
EOF

COMMANDS.add_prologue 'clGetDeviceIDs', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_devices, device_timer) && !num_devices && devices) {
    num_devices = &n_e;
  }
EOF

COMMANDS.add_epilogue 'clGetDeviceIDs', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_devices, device_name) && _retval == CL_SUCCESS && devices) {
    for (cl_uint i = 0; i < *num_devices; i++) {
      dump_device_name(devices[i]);
    }
  }
EOF

COMMANDS.add_epilogue 'clGetDeviceIDs', <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_devices, device_timer) && _retval == CL_SUCCESS && devices) {
    for (cl_uint i = 0; i < *num_devices; i++) {
      dump_device_timer(devices[i]);
    }
  }
EOF

# clGetExtensionFunctionAddress[ForPlatform] hand back the tracer's own
# wrapper for a core function named like an extension, and a libffi closure
# around the real pointer for an extension function.
def wrap_extension_lookup(ext_method)
  args = ext_method == 'clGetExtensionFunctionAddress' ? 'func_name' : 'platform, func_name'

  COMMANDS.add_prologue ext_method, CORE_EXTENSION_COMMANDS.collect { |c| <<EOF }.join("  else\n")
  if (strcmp(func_name, "#{c.name}") == 0) {
    tracepoint(lttng_ust_opencl, #{ext_method}_#{STOP}, #{args}, (void *)(intptr_t)#{OPENCL_POINTER_NAMES[c]}, 0);
    return (void *)(intptr_t)(&#{c.name});
  }
EOF

  COMMANDS.add_epilogue ext_method, ffi_closure_epilogue(
    EXTENSION_COMMANDS, condition: '_retval != NULL', provider: :lttng_ust_opencl,
                        name_var: 'func_name', prefix: 'opencl', real: '_retval', slot: '&_retval', found: <<~EOF
                          tracepoint(lttng_ust_opencl, #{ext_method}_#{STOP}, #{args}, _retval, _duration);
                          return closure->c_ptr;
                        EOF
  )
end

wrap_extension_lookup('clGetExtensionFunctionAddressForPlatform')
wrap_extension_lookup('clGetExtensionFunctionAddress')

# Create event profiling code
COMMANDS.each do |c|
  if opencl_event?(c)
    if !opencl_returns_event?(c)
      c.add_prologue <<EOF
  int _profile_release_event = 0;
  int _event_profiling = 0;
  cl_event _profiling_event = NULL;
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling)) {
    if (event == NULL) {
      event = &_profiling_event;
      _profile_release_event = 1;
    }
    _event_profiling = 1;
  }
EOF
      c.add_epilogue <<EOF
  if (_event_profiling) {
    if (_retval == CL_SUCCESS) {
      int _set_retval = #{OPENCL_POINTER_NAMES[COMMANDS['clSetEventCallback']]}(*event, CL_COMPLETE, event_notify, NULL);
      do_tracepoint(lttng_ust_opencl_profiling, event_profiling, _set_retval, *event);
    }
    if(_profile_release_event) {
      #{OPENCL_POINTER_NAMES[COMMANDS['clReleaseEvent']]}(*event);
      event = NULL;
    }
  }
EOF
    elsif c.name != 'clCreateUserEvent'
      c.add_epilogue <<EOF
  if (tracepoint_enabled(lttng_ust_opencl_profiling, event_profiling) ) {
    int _set_retval = #{OPENCL_POINTER_NAMES[COMMANDS['clSetEventCallback']]}(_retval, CL_COMPLETE, event_notify, NULL);
    do_tracepoint(lttng_ust_opencl_profiling, event_profiling, _set_retval, _retval);
  }
EOF
    end
  end
end

str = <<EOF
  if (do_dump && _enqueue_counter >= dump_start && _enqueue_counter <= dump_end) {
    if (_retval == CL_SUCCESS) {
      cl_event ev = dump_kernel_buffers(command_queue, kernel, _enqueue_counter, event);
      if (_dump_release_event) {
        #{OPENCL_POINTER_NAMES[COMMANDS['clReleaseEvent']]}(*event);
        event = NULL;
        if (ev != NULL) {
          #{OPENCL_POINTER_NAMES[COMMANDS['clReleaseEvent']]}(ev);
        }
      } else if ( ev != NULL ) {
        if (event != NULL) {
          if (*event != NULL)
            #{OPENCL_POINTER_NAMES[COMMANDS['clReleaseEvent']]}(*event);
          *event = ev;
        }
      }
    } else {
      if (_dump_release_event) {
        #{OPENCL_POINTER_NAMES[COMMANDS['clReleaseEvent']]}(*event);
        event = NULL;
      }
    }
  }
EOF
COMMANDS.add_epilogue 'clEnqueueNDRangeKernel', str
COMMANDS.add_epilogue 'clEnqueueNDRangeKernelINTEL', str
