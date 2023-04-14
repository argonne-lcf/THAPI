require 'yaml'
require 'pp'
require_relative '../utils/yaml_ast'
require_relative '../utils/LTTng'
require_relative '../utils/command.rb'
require_relative '../utils/meta_parameters'

if ENV["SRC_DIR"]
  SRC_DIR = ENV["SRC_DIR"]
else
  SRC_DIR = "."
end

RESULT_NAME = "zeResult"

$ze_api_yaml = YAML::load_file("ze_api.yaml")
$zet_api_yaml = YAML::load_file("zet_api.yaml")
$zes_api_yaml = YAML::load_file("zes_api.yaml")
$zel_api_yaml = YAML::load_file("zel_api.yaml")

$ze_api = YAMLCAst.from_yaml_ast($ze_api_yaml)
$zet_api = YAMLCAst.from_yaml_ast($zet_api_yaml)
$zes_api = YAMLCAst.from_yaml_ast($zes_api_yaml)
$zel_api = YAMLCAst.from_yaml_ast($zel_api_yaml)

ze_funcs_e = $ze_api["functions"]
zet_funcs_e = $zet_api["functions"]
zes_funcs_e = $zes_api["functions"]
zel_funcs_e = $zel_api["functions"]

ze_types_e = $ze_api["typedefs"]
zet_types_e = $zet_api["typedefs"]
zes_types_e = $zes_api["typedefs"]
zel_types_e = $zel_api["typedefs"]

all_types = ze_types_e + zet_types_e + zes_types_e + zel_types_e
all_structs = $ze_api["structs"] + $zet_api["structs"] + $zes_api["structs"] + $zel_api["structs"]

ZE_OBJECTS = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }
all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && ZE_OBJECTS.include?(t.type.name)
    ZE_OBJECTS.push t.name
  end
}
ZE_INT_SCALARS = %w(uintptr_t size_t int8_t uint8_t int16_t uint16_t int32_t uint32_t int64_t uint64_t char)
ZE_FLOAT_SCALARS = %w(float double)
ZE_SCALARS = ZE_INT_SCALARS + ZE_FLOAT_SCALARS

all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && ZE_INT_SCALARS.include?(t.type.name)
    ZE_INT_SCALARS.push t.name
  end
}

ZE_ENUM_SCALARS = all_types.select { |t| t.type.kind_of? YAMLCAst::Enum }.collect { |t| t.name }
STRUCT_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.collect { |t| t.name } + [ "zet_core_callbacks_t", "zel_core_callbacks_t" ]
ZE_UNION_TYPES = all_types.select { |t| t.type.kind_of? YAMLCAst::Union }.collect { |t| t.name }
ZE_POINTER_TYPES = all_types.select { |t| t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct) }.collect { |t| t.name }

STRUCT_MAP = {}
all_types.select { |t| t.type.kind_of? YAMLCAst::Struct }.each { |t|
  if t.type.members
    STRUCT_MAP[t.name] = t.type.members
  else
    STRUCT_MAP[t.name] = all_structs.find { |str| str.name == t.type.name }.members
  end
}

INIT_FUNCTIONS = /zeInit|zeLoaderInit/

FFI_TYPE_MAP =  {
 "uint8_t" => "ffi_type_uint8",
 "int8_t" => "ffi_type_sint8",
 "uint16_t" => "ffi_type_uint16",
 "int16_t" => "ffi_type_sint16",
 "uint32_t" => "ffi_type_uint32",
 "int32_t" => "ffi_type_sint32",
 "uint64_t" => "ffi_type_uint64",
 "int64_t" => "ffi_type_sint64",
 "float" => "ffi_type_float",
 "double" => "ffi_type_double",
 "uintptr_t" => "ffi_type_pointer",
 "size_t" => "ffi_type_pointer",
 "ze_bool_t" => "ffi_type_uint8"
}

ZE_OBJECTS.each { |o|
  FFI_TYPE_MAP[o] = "ffi_type_pointer"
}

ZE_ENUM_SCALARS.each { |e|
  FFI_TYPE_MAP[e] = "ffi_type_sint32"
}

ZE_FLOAT_SCALARS_MAP = {"float" => "uint32_t", "double" => "uint64_t"}

module YAMLCAst
  class Declaration
    def lttng_type
      r = type.lttng_type
      r.name = name
      case type
      when Struct, Union
        r.expression = "&#{name}"
      when CustomType
        case type.name
        when *STRUCT_TYPES, *ZE_UNION_TYPES
          r.expression = "&#{name}"
        else
          r.expression = name
        end
      else
        r.expression = name
      end
      r
    end
  end

  class Type
    def lttng_type
      raise "Unsupported type #{self}!"
    end
  end

  class Int
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Float
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_float
      ev.type = name
      ev
    end
  end

  class Char
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Bool
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = name
      ev
    end
  end

  class Struct
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_array_text
      ev.type = :uint8_t
      ev.length = "sizeof(struct #{name})"
      ev
    end

    def [](name)
      members.find { |m| m.name == name }
    end
  end

  class Union
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_array_text
      ev.type = :uint8_t
      ev.length = "sizeof(union #{name})"
      ev
    end
  end

  class Enum
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer
      ev.type = "enum #{name}"
      ev
    end
  end

  class CustomType
    def lttng_type
      ev = LTTng::TracepointField::new
      case name
      when *ZE_OBJECTS, *ZE_POINTER_TYPES
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = "uintptr_t"
      when *ZE_INT_SCALARS
        ev.macro = :ctf_integer
        ev.type = name
      when *ZE_ENUM_SCALARS
        ev.macro = :ctf_integer
        ev.type = :int32_t
      when *STRUCT_TYPES, *ZE_UNION_TYPES
        ev.macro = :ctf_array_text
        ev.type = :uint8_t
        ev.length = "sizeof(#{name})"
      else
        super
      end
      ev
    end
  end

  class Pointer
    def lttng_type
      ev = LTTng::TracepointField::new
      ev.macro = :ctf_integer_hex
      ev.type = :uintptr_t
      ev.cast = "uintptr_t"
      ev
    end
  end

  class Array
    def lttng_type(length: nil, length_type: nil)
      ev = LTTng::TracepointField::new
      if length
        ev.length = length
      elsif self.length
        ev.length = self.length
      else
        ev.macro = :ctf_integer_hex
        ev.type = :uintptr_t
        ev.cast = "uintptr_t"
        return ev
      end
      if length_type
        lttng_arr_type = "sequence"
        ev.length_type = length_type
      else
        lttng_arr_type = "array"
      end
      case type
      when YAMLCAst::Pointer
        ev.macro = :"ctf_#{lttng_arr_type}_hex"
        ev.type = :uintptr_t
      when YAMLCAst::Int
        ev.macro = :"ctf_#{lttng_arr_type}"
        ev.type = type.name
      when YAMLCAst::Float
        ev.macro = :"ctf_#{lttng_arr_type}_hex"
        ev.type = ZE_FLOAT_SCALARS_MAP[type.name]
      when YAMLCAst::Char
        ev.macro = :"ctf_#{lttng_arr_type}_text"
        ev.type = type.name
      when YAMLCAst::CustomType
        case type.name
        when *ZE_OBJECTS, *ZE_POINTER_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_hex"
          ev.type = :uintptr_t
        when *ZE_INT_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = type.name
        when *ZE_ENUM_SCALARS
          ev.macro = :"ctf_#{lttng_arr_type}"
          ev.type = :int32_t
        when *STRUCT_TYPES, *ZE_UNION_TYPES
          ev.macro = :"ctf_#{lttng_arr_type}_text"
          ev.type = :uint8_t
          if ev.length
            ev.length = "(#{ev.length}) * sizeof(#{type.name})"
          end
        else
          super
        end
      else
        super
      end
      ev
    end
  end
end

$ze_meta_parameters = YAML::load_file(File.join(SRC_DIR, "ze_meta_parameters.yaml"))
$ze_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}
$zet_meta_parameters = YAML::load_file(File.join(SRC_DIR, "zet_meta_parameters.yaml"))
$zet_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$zes_meta_parameters = YAML::load_file(File.join(SRC_DIR, "zes_meta_parameters.yaml"))
$zes_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$zel_meta_parameters = YAML::load_file(File.join(SRC_DIR, "zel_meta_parameters.yaml"))
$zel_meta_parameters["meta_parameters"].each  { |func, list|
  list.each { |type, *args|
    register_meta_parameter func, Kernel.const_get(type), *args
  }
}

$ze_commands = ze_funcs_e.collect { |func|
  Command::new(func)
}

$zet_commands = zet_funcs_e.collect { |func|
  Command::new(func)
}

$zes_commands = zes_funcs_e.collect { |func|
  Command::new(func)
}

$zel_commands = zel_funcs_e.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

ZE_POINTER_NAMES = ($ze_commands + $zet_commands + $zes_commands + $zel_commands).collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}.to_h

register_epilogue "zeDeviceGet", <<EOF
  if (_do_state()) {
    if (_retval == ZE_RESULT_SUCCESS && phDevices && pCount) {
      for (uint32_t i = 0; i < *pCount; i++)
        _register_ze_device(phDevices[i], hDriver, NULL);
    }
  }
EOF

register_epilogue "zeDeviceGetSubDevices", <<EOF
  if (_do_state()) {
    if (_retval == ZE_RESULT_SUCCESS && phSubdevices && pCount) {
      for (uint32_t i = 0; i < *pCount; i++)
        _register_ze_device(phSubdevices[i], NULL, hDevice);
    }
  }
EOF

register_epilogue "zeCommandListCreate", <<EOF
  if (_do_state()) {
    if (_retval == ZE_RESULT_SUCCESS && phCommandList && *phCommandList) {
      _on_create_command_list(*phCommandList, hContext, hDevice, 0);
    }
  }
EOF

register_epilogue "zeCommandListCreateImmediate", <<EOF
  if (_do_state()) {
    if (_retval == ZE_RESULT_SUCCESS && phCommandList && *phCommandList) {
      _on_create_command_list(*phCommandList, hContext, hDevice, 1);
    }
  }
EOF

register_epilogue "zeCommandListReset", <<EOF
  if (_do_profile && hCommandList)
    _on_reset_command_list(hCommandList);
EOF

register_epilogue "zeCommandListDestroy", <<EOF
  if (_do_state()) {
    if (_retval == ZE_RESULT_SUCCESS && hCommandList) {
      _on_destroy_command_list(hCommandList);
    }
  }
EOF

register_epilogue "zeCommandQueueExecuteCommandLists", <<EOF
  if (_do_profile) {
    if (_retval == ZE_RESULT_SUCCESS && numCommandLists > 0) {
      _on_execute_command_lists(numCommandLists, phCommandLists);
    }
  }
EOF

register_prologue "zeEventPoolCreate", <<EOF
  ze_event_pool_desc_t _new_desc;
  if (_do_profile && desc && !(desc->flags & ZE_EVENT_POOL_FLAG_IPC)) {
    _new_desc = *desc;
    _new_desc.flags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    _new_desc.flags |= ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    desc = &_new_desc;
  }
EOF

register_prologue "zeEventCreate", <<EOF
  ze_event_desc_t _new_desc;
  if (_do_profile && desc) {
    _new_desc = *desc;
    _new_desc.signal |= ZE_EVENT_SCOPE_FLAG_HOST;
    _new_desc.wait |= ZE_EVENT_SCOPE_FLAG_HOST;
    desc = &_new_desc;
  }
EOF

register_epilogue "zeEventCreate", <<EOF
  if (_do_profile && _retval == ZE_RESULT_SUCCESS) {
    _on_created_event(*phEvent);
  }
EOF

register_prologue "zeEventDestroy", <<EOF
  if (_do_profile && hEvent) {
    _on_destroy_event(hEvent);
  }
EOF

register_prologue "zeEventHostReset", <<EOF
  if (_do_profile && hEvent) {
    _on_reset_event(hEvent);
  }
EOF

register_epilogue "zeContextDestroy", <<EOF
  if (_do_profile && hContext) {
    _on_destroy_context(hContext);
  }
EOF

# Dump memory info if required
memory_info_dump = lambda { |ptr_name|
  "_dump_memory_info(hCommandList, #{ptr_name})"
}

memory_info_prologue = lambda { |ptr_names|
  s = <<EOF
  if (tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) || tracepoint_enabled(lttng_ust_ze_properties, memory_info_range)) {
    #{ptr_names.collect { |ptr_name| memory_info_dump.call(ptr_name) }.join(";\n    ")};
  }
EOF
}

register_prologue "zeCommandListAppendMemoryRangesBarrier", <<EOF
  if ((tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) ||
       tracepoint_enabled(lttng_ust_ze_properties, memory_info_range)) &&
      numRanges && pRangeSizes && pRanges && hCommandList)
    for (uint32_t _i = 0; _i < numRanges; _i++)
      _dump_memory_info(hCommandList, "pRanges[_i]");
EOF

register_prologue "zeCommandListAppendMemoryCopy", memory_info_prologue.call(["dstptr", "srcptr"])
register_prologue "zeCommandListAppendMemoryFill", memory_info_prologue.call(["ptr"])
register_prologue "zeCommandListAppendMemoryCopyRegion", memory_info_prologue.call(["dstptr", "srcptr"])
register_prologue "zeCommandListAppendMemoryCopyFromContext", <<EOF
  if (tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) ||
      tracepoint_enabled(lttng_ust_ze_properties, memory_info_range)) {
    if (hCommandList)
      _dump_memory_info(hCommandList, dstptr);
    if (hContextSrc)
      _dump_memory_info_ctx(hContextSrc, srcptr);
  }
EOF
register_prologue "zeCommandListAppendImageCopyToMemory", memory_info_prologue.call(["dstptr"])
register_prologue "zeCommandListAppendImageCopyFromMemory", memory_info_prologue.call(["srcptr"])
register_prologue "zeCommandListAppendMemoryPrefetch", memory_info_prologue.call(["ptr"])
register_prologue "zeCommandListAppendMemAdvise", memory_info_prologue.call(["ptr"])
register_prologue "zeCommandListAppendQueryKernelTimestamps", memory_info_prologue.call(["dstptr"])
register_prologue "zeCommandListAppendWriteGlobalTimestamp", memory_info_prologue.call(["dstptr"])
register_prologue "zeCommandListAppendImageCopyToMemoryExt", memory_info_prologue.call(["dstptr"])
register_prologue "zeCommandListAppendImageCopyFromMemoryExt", memory_info_prologue.call(["srcptr"])

# WARNING: there seems to be no way to profile if
# zeCommandListAppendEventReset is used or at least
# not very cleanly is used....
profiling_prologue = lambda { |event_name|
  <<EOF
  struct _ze_event_h * _ewrapper = NULL;
  if (_do_profile && !#{event_name}) {
    _ewrapper = _get_profiling_event(hCommandList);
    if (_ewrapper)
      #{event_name} = _ewrapper->event;
  }
EOF
}

profiling_epilogue = lambda { |event_name|
  <<EOF
  if (_do_profile && #{event_name}) {
    if (_retval == ZE_RESULT_SUCCESS) {
      _register_ze_event(#{event_name}, hCommandList, _ewrapper);
      tracepoint(lttng_ust_ze_profiling, event_profiling, #{event_name});
    } else if (_ewrapper)
      PUT_ZE_EVENT(_ewrapper);
  }
EOF
}

paranoid_drift_prologue = <<EOF
  if (_paranoid_drift &&
      ZE_DEVICE_GET_GLOBAL_TIMESTAMPS_PTR &&
      tracepoint_enabled(lttng_ust_ze_properties, device_timer))
    _dump_command_list_device_timer(hCommandList);
EOF

[ "zeCommandListAppendLaunchKernel",
  "zeCommandListAppendBarrier",
  "zeCommandListAppendLaunchCooperativeKernel",
  "zeCommandListAppendLaunchKernelIndirect",
  "zeCommandListAppendLaunchMultipleKernelsIndirect",
  "zeCommandListAppendMemoryRangesBarrier",
  "zeCommandListAppendMemoryCopy",
  "zeCommandListAppendMemoryFill",
  "zeCommandListAppendMemoryCopyRegion",
  "zeCommandListAppendMemoryCopyFromContext",
  "zeCommandListAppendImageCopy",
  "zeCommandListAppendImageCopyRegion",
  "zeCommandListAppendImageCopyToMemory",
  "zeCommandListAppendImageCopyFromMemory",
  "zeCommandListAppendQueryKernelTimestamps",
  "zeCommandListAppendWriteGlobalTimestamp",
  "zeCommandListAppendImageCopyToMemoryExt",
  "zeCommandListAppendImageCopyFromMemoryExt" ].each { |c|
    register_prologue c, profiling_prologue.call("hSignalEvent")
    register_prologue c, paranoid_drift_prologue
    register_epilogue c, profiling_epilogue.call("hSignalEvent")
}

[ "zeCommandListAppendSignalEvent" ].each { |c|
    register_prologue c, profiling_prologue.call("hEvent")
    register_epilogue c, profiling_epilogue.call("hEvent")
}

#WARNING
# zeModuleGetKernelNames, returns an array of strings.
# This is problematic for lttng.

register_prologue "zeModuleCreate", <<EOF
  int _build_log_release = 0;
  ze_module_build_log_handle_t _hBuildLog = NULL;
  if (tracepoint_enabled(lttng_ust_ze_build, log)) {
    if (phBuildLog == NULL) {
      phBuildLog = &_hBuildLog;
      _build_log_release = 1;
    }
  }
EOF

register_epilogue "zeModuleCreate", <<EOF
  if (tracepoint_enabled(lttng_ust_ze_build, log) && (_retval == ZE_RESULT_SUCCESS || _retval == ZE_RESULT_ERROR_MODULE_BUILD_FAILURE) && *phBuildLog) {
    _dump_build_log(*phBuildLog);
    if (_build_log_release) {
      ZE_MODULE_BUILD_LOG_DESTROY_PTR(*phBuildLog);
      phBuildLog = NULL;
    }
  }
EOF

register_prologue "zeModuleDynamicLink", <<EOF
  int _link_log_release = 0;
  ze_module_build_log_handle_t _hLinkLog = NULL;
  if (tracepoint_enabled(lttng_ust_ze_build, log)) {
    if (phLinkLog == NULL) {
      phLinkLog = &_hLinkLog;
      _link_log_release = 1;
    }
  }
EOF

register_epilogue "zeModuleDynamicLink", <<EOF
  if (tracepoint_enabled(lttng_ust_ze_build, log) && (_retval == ZE_RESULT_SUCCESS || _retval == ZE_RESULT_ERROR_MODULE_LINK_FAILURE) && *phLinkLog) {
    _dump_build_log(*phLinkLog);
    if (_link_log_release) {
      ZE_MODULE_BUILD_LOG_DESTROY_PTR(*phLinkLog);
      phLinkLog = NULL;
    }
  }
EOF

register_epilogue "zeKernelCreate", <<EOF
 if (tracepoint_enabled(lttng_ust_ze_properties, kernel) && (_retval == ZE_RESULT_SUCCESS)) {
    _dump_kernel_properties(*phKernel);
 }
EOF
