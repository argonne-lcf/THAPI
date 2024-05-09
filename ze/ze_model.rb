require 'yaml'
require 'pp'
require_relative '../utils/yaml_ast_lttng'
require_relative '../utils/LTTng'
require_relative '../utils/command.rb'
require_relative '../utils/meta_parameters'
require 'set'

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
$zex_api_yaml = YAML::load_file("zex_api.yaml")

$ze_api = YAMLCAst.from_yaml_ast($ze_api_yaml)
$zet_api = YAMLCAst.from_yaml_ast($zet_api_yaml)
$zes_api = YAMLCAst.from_yaml_ast($zes_api_yaml)
$zel_api = YAMLCAst.from_yaml_ast($zel_api_yaml)
$zex_api = YAMLCAst.from_yaml_ast($zex_api_yaml)

ze_funcs_e = $ze_api["functions"]
zet_funcs_e = $zet_api["functions"]
zes_funcs_e = $zes_api["functions"]
zel_funcs_e = $zel_api["functions"]
zex_funcs_e = $zex_api["functions"]

typedefs = $ze_api["typedefs"] + $zet_api["typedefs"] + $zes_api["typedefs"] + $zel_api["typedefs"] + $zex_api["typedefs"]
structs = $ze_api["structs"] + $zet_api["structs"] + $zes_api["structs"] + $zel_api["structs"] + $zex_api["structs"]

find_all_types(typedefs)
gen_struct_map(typedefs, structs)
gen_ffi_type_map(typedefs)

INIT_FUNCTIONS = /zeInit|zeLoaderInit/

$struct_type_conversion_table = {
  "ZE_STRUCTURE_TYPE_IMAGE_MEMORY_PROPERTIES_EXP" => "ZE_STRUCTURE_TYPE_IMAGE_MEMORY_EXP_PROPERTIES",
  "ZE_STRUCTURE_TYPE_IMAGE_PITCHED_EXP_DESC" => "ZE_STRUCTURE_TYPE_PITCHED_IMAGE_EXP_DESC",
  "ZE_STRUCTURE_TYPE_IMAGE_BINDLESS_EXP_DESC" => "ZE_STRUCTURE_TYPE_BINDLESS_IMAGE_EXP_DESC",
  "ZE_STRUCTURE_TYPE_DEVICE_PITCHED_ALLOC_EXP_PROPERTIES" => "ZE_STRUCTURE_TYPE_PITCHED_ALLOC_DEVICE_EXP_PROPERTIES",
  "ZE_STRUCTURE_TYPE_CONTEXT_POWER_SAVING_HINT_EXP_DESC" => "ZE_STRUCTURE_TYPE_POWER_SAVING_HINT_EXP_DESC",
  "ZE_STRUCTURE_TYPE_EVENT_POOL_COUNTER_BASED_EXP_DESC" => "ZE_STRUCTURE_TYPE_COUNTER_BASED_EVENT_POOL_EXP_DESC",
  "ZE_STRUCTURE_TYPE_KERNEL_MAX_GROUP_SIZE_PROPERTIES_EXT" => "ZE_STRUCTURE_TYPE_KERNEL_MAX_GROUP_SIZE_EXT_PROPERTIES",
  "ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32_HANDLE" => "ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32",
  "ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_WIN32_HANDLE" => "ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_WIN32",
  "ZES_STRUCTURE_TYPE_MEM_PAGE_OFFLINE_STATE_EXP" => "ZES_STRUCTURE_TYPE_MEMORY_PAGE_OFFLINE_STATE_EXP",
}

$struct_type_reject = Set.new([
])

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

$zex_meta_parameters = YAML::load_file(File.join(SRC_DIR, "zex_meta_parameters.yaml"))
$zex_meta_parameters["meta_parameters"].each  { |func, list|
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

$zex_commands = zex_funcs_e.collect { |func|
  Command::new(func)
}

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

ze_pointer_names = ($ze_commands + $zet_commands + $zes_commands + $zel_commands).collect { |c|
  [c, upper_snake_case(c.pointer_name)]
}
ze_pointer_names += $zex_commands.collect { |c|
  [c, c.pointer_name]
}
ZE_POINTER_NAMES = ze_pointer_names.to_h

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
#memory_info_dump = lambda { |ptr_name|
#  "_dump_memory_info(hCommandList, #{ptr_name})"
#}
#
#memory_info_prologue = lambda { |ptr_names|
#  s = <<EOF
#  if (tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) || tracepoint_enabled(lttng_ust_ze_properties, memory_info_range)) {
#    #{ptr_names.collect { |ptr_name| memory_info_dump.call(ptr_name) }.join(";\n    ")};
#  }
#EOF
#}
#
#register_prologue "zeCommandListAppendMemoryRangesBarrier", <<EOF
#  if ((tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) ||
#       tracepoint_enabled(lttng_ust_ze_properties, memory_info_range)) &&
#      numRanges && pRangeSizes && pRanges && hCommandList)
#    for (uint32_t _i = 0; _i < numRanges; _i++)
#      _dump_memory_info(hCommandList, "pRanges[_i]");
#EOF

#register_prologue "zeCommandListAppendMemoryCopy", memory_info_prologue.call(["dstptr", "srcptr"])
#register_prologue "zeCommandListAppendMemoryFill", memory_info_prologue.call(["ptr"])
#register_prologue "zeCommandListAppendMemoryCopyRegion", memory_info_prologue.call(["dstptr", "srcptr"])
#register_prologue "zeCommandListAppendMemoryCopyFromContext", <<EOF
#  if (tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) ||
#      tracepoint_enabled(lttng_ust_ze_properties, memory_info_range)) {
#    if (hCommandList)
#      _dump_memory_info(hCommandList, dstptr);
#    if (hContextSrc)
#      _dump_memory_info_ctx(hContextSrc, srcptr);
#  }
#EOF
#register_prologue "zeCommandListAppendImageCopyToMemory", memory_info_prologue.call(["dstptr"])
#register_prologue "zeCommandListAppendImageCopyFromMemory", memory_info_prologue.call(["srcptr"])
#register_prologue "zeCommandListAppendMemoryPrefetch", memory_info_prologue.call(["ptr"])
#register_prologue "zeCommandListAppendMemAdvise", memory_info_prologue.call(["ptr"])
#register_prologue "zeCommandListAppendQueryKernelTimestamps", memory_info_prologue.call(["dstptr"])
#register_prologue "zeCommandListAppendWriteGlobalTimestamp", memory_info_prologue.call(["dstptr"])
#register_prologue "zeCommandListAppendImageCopyToMemoryExt", memory_info_prologue.call(["dstptr"])
#register_prologue "zeCommandListAppendImageCopyFromMemoryExt", memory_info_prologue.call(["srcptr"])

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

($ze_commands + $zet_commands + $zes_commands + $zel_commands).select { |c|
  c.name.match(/(ze|zet|zes|zel)Get.*ProcAddrTable/)
}.each { |c|
  parent_type = c['pDdiTable'].type.type.to_s + "_"
  child_types = STRUCT_MAP.select { |k, _| k.match(parent_type) }
  str = <<EOF
  #{c.type} _retval;
  if (pDdiTable && version <= ZE_API_VERSION_CURRENT) {
EOF
  str << "   "
  str << child_types.reverse_each.collect { |k, v|
    version = k.match(/_t_(\d+)_(\d+)/)[1..2]
    sstr = " if (version >= ZE_MAKE_VERSION(#{version[0]},#{version[1]})) {\n"
    v.each { |m|
      sstr << "      pDdiTable->#{m.name} = #{m.type.to_s.sub(/_t$/,"").sub("_pfn","")}_hid;\n"
    }
    sstr << "      _retval = ZE_RESULT_SUCCESS;\n"
    sstr << "    } else"
  }.join
  str << "\n"
  str << "      goto ukn;\n"
  str << "  } else {\n"
  str << "ukn:\n"
  register_prologue c.name, str
  register_epilogue c.name, <<EOF
  }
EOF
}

str = <<EOF
  if (_retval == ZE_RESULT_SUCCESS && *ppFunctionAddress) {
EOF
str << $zex_commands.collect { |c|
  sstr = <<EOF
    if (tracepoint_enabled(lttng_ust_zex, #{c.name}_#{START}) && strcmp(name, "#{c.name}") == 0) {
      struct ze_closure *closure = NULL;
      pthread_mutex_lock(&ze_closures_mutex);
      HASH_FIND_PTR(ze_closures, ppFunctionAddress, closure);
      pthread_mutex_unlock(&ze_closures_mutex);
      if (closure != NULL) {
        tracepoint(lttng_ust_ze, zeDriverGetExtensionFunctionAddress_exit, hDriver, name, ppFunctionAddress, _retval);
        *ppFunctionAddress = closure->c_ptr;
        return _retval;
      }
      closure = (struct ze_closure *)malloc(sizeof(struct ze_closure) + #{c.parameters.size} * sizeof(ffi_type *));
      if (closure != NULL) {
        closure->types = (ffi_type **)((intptr_t)closure + sizeof(struct ze_closure));
        closure->closure = ffi_closure_alloc(sizeof(ffi_closure), &(closure->c_ptr));
        if (closure->closure != NULL) {
          closure->ptr = *ppFunctionAddress;
EOF
  c.parameters.each_with_index { |a, i|
    if a.type.kind_of?(YAMLCAst::Pointer)
      ffi_type = "ffi_type_pointer"
    elsif FFI_TYPE_MAP["#{a.type}"]
      ffi_type = FFI_TYPE_MAP["#{a.type}"]
    else
      raise "Unsupported type: #{a.type}"
    end
    sstr << <<EOF
          closure->types[#{i}] = &#{ffi_type};
EOF
  }
  if c.type.kind_of?(YAMLCAst::Void)
    ffi_ret_type = "ffi_type_void"
  elsif c.type.kind_of?(YAMLCAst::Pointer)
    ffi_ret_type = "ffi_type_pointer"
  elsif FFI_TYPE_MAP["#{c.type}"]
    ffi_ret_type = FFI_TYPE_MAP["#{c.type}"]
  else
    raise "Unsupported type: #{c.type}"
  end
  sstr << <<EOF
          if (ffi_prep_cif(&(closure->cif), FFI_DEFAULT_ABI, #{c.parameters.size}, &#{ffi_ret_type}, closure->types) == FFI_OK) {
            if (ffi_prep_closure_loc(closure->closure, &(closure->cif), (void (*)(ffi_cif *, void *, void **, void *))#{c.name}_ffi, *ppFunctionAddress, closure->c_ptr) == FFI_OK) {
              pthread_mutex_lock(&ze_closures_mutex);
              HASH_ADD_PTR(ze_closures, ptr, closure);
              pthread_mutex_unlock(&ze_closures_mutex);
              tracepoint(lttng_ust_ze, zeDriverGetExtensionFunctionAddress_exit, hDriver, name, ppFunctionAddress, _retval);
              *ppFunctionAddress = closure->c_ptr;
              return _retval;
            }
          }
          ffi_closure_free(closure->closure);
        }
        free(closure);
      }
    }
EOF
}.join(<<EOF)
    else
EOF
str << <<EOF
  }
EOF

register_epilogue "zeDriverGetExtensionFunctionAddress", str

register_epilogue "zeMemOpenIpcHandle", <<EOF
  if (_retval == ZE_RESULT_SUCCESS && pptr) {
    _dump_memory_info_ctx(hContext, *pptr);
  }
EOF

register_epilogue "zexMemOpenIpcHandles", <<EOF
  if (_retval == ZE_RESULT_SUCCESS && pptr) {
    _dump_memory_info_ctx(hContext, *pptr);
  }
EOF
