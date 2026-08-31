require 'yaml'
require 'pp'
require_relative '../../utils/api_model'
require_relative '../../utils/yaml_ast_lttng'
require_relative '../../utils/LTTng'
require_relative '../../utils/command'
require_relative '../../utils/meta_parameters'
require 'set'

SRC_DIR = ENV['SRC_DIR'] || '.'

# The namespaces THAPI traces, each declared by its own header. zer has no
# generated api.yaml yet, so it is an empty model rather than a missing key:
# every namespace answers, and the loops below stay uniform.
APIS = {
  ze: ApiModel.load_file('ze_api.yaml'),
  zet: ApiModel.load_file('zet_api.yaml'),
  zes: ApiModel.load_file('zes_api.yaml'),
  zel: ApiModel.load_file('zel_api.yaml'),
  zer: ApiModel.new,
  zex: ApiModel.load_file('zex_api.yaml'),
}.freeze

# The self-describing structs of one namespace: those whose first member is a
# tag naming the struct's own type. That tag is what lets the tracer decode a
# `void *` at runtime, so it is what makes a struct worth its own tracepoint.
#
# Level Zero spells the tag `stype`; the name is a parameter because the
# pattern is not Level Zero's. Any API that tags its extensible structs the
# same way answers this question by passing its own spelling.
def tagged_structs(api, tag: 'stype')
  api.types.select do |t|
    t.type.is_a?(YAMLCAst::Struct) &&
      (struct = api.struct_named(t.type.name)) &&
      struct.members.first.name == tag
  end.map(&:name)
end

# Those a caller can be handed. The `<ns>_base_` types are the tag's own base
# classes: a tracepoint exists for each, but no API call ever passes one, so
# nothing dispatches on them.
def concrete_tagged_structs(namespace, api)
  tagged_structs(api).reject { |n| n.start_with?("#{namespace}_base_") }.to_set
end

# Every namespace as one API, the same thing `API` names in every other
# backend. The derivations have to see all of them at once: a zet typedef
# routinely names a ze struct.
API = APIS.values.reduce(:+)

gen_ffi_type_map(API.types, API.type_classes)

CONTEXT = BackendContext.new(
  result_name: 'zeResult',
  # zesInit is included here so that a pure-Sysman program (one that only calls
  # zesInit, never zeInit) still triggers the tracer initialization.
  # Ideally we would split this into INIT_ZE_FUNCTIONS / INIT_ZES_FUNCTIONS
  # so each namespace initializes its own symbols.
  init_functions: /zeInit|zeLoaderInit|zeInitDrivers|zesInit/,
  struct_map: API.struct_map,
  type_classes: API.type_classes
)

STRUCT_TYPE_CONVERSION_TABLE = {
  'ZE_STRUCTURE_TYPE_IMAGE_MEMORY_PROPERTIES_EXP' => 'ZE_STRUCTURE_TYPE_IMAGE_MEMORY_EXP_PROPERTIES',
  'ZE_STRUCTURE_TYPE_IMAGE_PITCHED_EXP_DESC' => 'ZE_STRUCTURE_TYPE_PITCHED_IMAGE_EXP_DESC',
  'ZE_STRUCTURE_TYPE_IMAGE_BINDLESS_EXP_DESC' => 'ZE_STRUCTURE_TYPE_BINDLESS_IMAGE_EXP_DESC',
  'ZE_STRUCTURE_TYPE_DEVICE_PITCHED_ALLOC_EXP_PROPERTIES' => 'ZE_STRUCTURE_TYPE_PITCHED_ALLOC_DEVICE_EXP_PROPERTIES',
  'ZE_STRUCTURE_TYPE_CONTEXT_POWER_SAVING_HINT_EXP_DESC' => 'ZE_STRUCTURE_TYPE_POWER_SAVING_HINT_EXP_DESC',
  'ZE_STRUCTURE_TYPE_EVENT_POOL_COUNTER_BASED_EXP_DESC' => 'ZE_STRUCTURE_TYPE_COUNTER_BASED_EVENT_POOL_EXP_DESC',
  'ZE_STRUCTURE_TYPE_KERNEL_MAX_GROUP_SIZE_PROPERTIES_EXT' => 'ZE_STRUCTURE_TYPE_KERNEL_MAX_GROUP_SIZE_EXT_PROPERTIES',
  'ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32_HANDLE' => 'ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMPORT_WIN32',
  'ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_WIN32_HANDLE' => 'ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_WIN32',
  'ZE_STRUCTURE_TYPE_COMMAND_LIST_APPEND_LAUNCH_KERNEL_PARAM_COOPERATIVE_DESC' => 'ZE_STRUCTURE_TYPE_COMMAND_LIST_APPEND_PARAM_COOPERATIVE_DESC',
  'ZE_STRUCTURE_TYPE_DEVICE_CACHE_LINE_SIZE_EXT' => 'ZE_STRUCTURE_TYPE_DEVICE_CACHELINE_SIZE_EXT',
  'ZE_STRUCTURE_TYPE_KERNEL_ALLOCATION_EXP_PROPERTIES' => 'ZE_STRUCTURE_TYPE_KERNEL_ALLOCATION_PROPERTIES',
  'ZET_STRUCTURE_TYPE_EXPORT_DMA_BUF_EXP_PROPERTIES' => 'ZET_STRUCTURE_TYPE_EXPORT_DMA_EXP_PROPERTIES',
  'ZES_STRUCTURE_TYPE_MEM_PAGE_OFFLINE_STATE_EXP' => 'ZES_STRUCTURE_TYPE_MEMORY_PAGE_OFFLINE_STATE_EXP',
}

# Structs whose stype tag is not a member of ze_structure_type_t.
#
# - zex tags structures with a uint32_t alias (level_zero/ze_stypes.h) rather
#   than an enum, we don't handle that.
# - zet_metric_source_id_exp_t's tag is simply absent from the spec.
STRUCT_TYPE_REJECT = Set.new(%w[zet_metric_source_id_exp_t
                                zex_device_module_register_file_exp_t])

# Each namespace declares its meta-parameters in its own file, so the list
# follows APIS rather than restating it. zer's file exists but stays out until
# zer has a generated api.yaml to match it against.
meta_parameters = load_meta_parameters(*(APIS.keys - [:zer]).collect { |ns| "#{ns}_meta_parameters.yaml" })

# One group per namespace, because each namespace has its own LTTng provider.
COMMANDS = CommandIndex.new(APIS.to_h do |ns, api|
  [:"lttng_ust_#{ns}", api.functions.collect do |func|
    Command.new(func, context: CONTEXT, meta_parameters: meta_parameters[func.name])
  end]
end)

check_meta_parameters(meta_parameters, COMMANDS)

# zex is called through libffi rather than dlsym, so its pointer keeps the name
# from the header instead of the upper-snake macro the other namespaces get.
zex_commands = COMMANDS.groups[:lttng_ust_zex]
ze_pointer_names = (COMMANDS.to_a - zex_commands).collect do |c|
  [c, upper_snake_case(c.pointer_name)]
end
ze_pointer_names += zex_commands.collect do |c|
  [c, c.pointer_name]
end
ZE_POINTER_NAMES = ze_pointer_names.to_h

COMMANDS.add_epilogue 'zeCommandListCreate', <<EOF
  if (_do_state()) {
    if (_retval == ZE_RESULT_SUCCESS && phCommandList && *phCommandList && desc) {
      bool _io = (desc->flags & ZE_COMMAND_LIST_FLAG_IN_ORDER) != 0;
      _on_create_command_list(*phCommandList, hContext, /*immediate=*/false, _io);
    }
  }
EOF

COMMANDS.add_epilogue 'zeCommandListCreateImmediate', <<EOF
  if (_do_state()) {
    if (_retval == ZE_RESULT_SUCCESS && phCommandList && *phCommandList && altdesc) {
      bool _io = (altdesc->flags & ZE_COMMAND_QUEUE_FLAG_IN_ORDER) != 0;
      _on_create_command_list(*phCommandList, hContext, /*immediate=*/true, _io);
    }
  }
EOF

# Reset hook: the L0 spec
# (https://oneapi-src.github.io/level-zero-spec/level-zero/latest/core/api.html#zecommandlistreset)
# says the user must have synchronized first, so our slots are drained — but
# for a REGULAR cl "drained" is not "reclaimed" (_slot_release is a no-op for
# regular cls; their inj is baked into the cl body for reuse across Executes).
# Reset wipes that body, so we reclaim the slots/slabs/events now. Without it
# the stale slots are re-published on the next Execute (over-count) and slabs
# leak. The cl stays registered, empty for reuse.
COMMANDS.add_epilogue 'zeCommandListReset', <<EOF
  if (_do_profile && _retval == ZE_RESULT_SUCCESS && hCommandList)
    _on_reset_command_list(hCommandList);
EOF

# Destroy hook: the same spec rule applies for the GPU side (no in-flight
# work on the cl), but we still need to clean up OUR host-side state —
# slot slabs, per-slot waits, and tracer-owned events that haven't
# already gone back to the pool. Otherwise every cl create/destroy cycle
# leaks all of the above.
#
# Gated on _do_state() to stay symmetric with the create hooks: create
# registers a cl whenever _do_state() holds (profiling OR memory-info), so
# destroy must unregister under the same condition. Using the narrower
# _do_profile here would leak the registration in a memory-info-only config,
# and leave a stale entry that a handle-recycled create later collides with.
COMMANDS.add_epilogue 'zeCommandListDestroy', <<EOF
  if (_do_state() && _retval == ZE_RESULT_SUCCESS && hCommandList)
    _on_destroy_command_list(hCommandList);
EOF

# zeContextDestroy prologue: tear down our own L0 objects that live
# inside this context (per-ctx event pools/events) before the user destroys
# it, so we don't leak our allocations.
COMMANDS.add_prologue 'zeContextDestroy', <<EOF
  if (_do_profile && hContext)
    _on_destroy_context(hContext);
EOF

# All Execute bookkeeping runs in the PROLOGUE (before L0 submit) as one
# critical section: force-sync-prior + drain (read timing, reset our injected
# event) + re-instantiate + claim in_flight_q. Draining a replayed regular cl's
# previous instance BEFORE this submission re-signals the same baked injected
# event is what keeps #N-1's timing from being clobbered by #N, and serializes
# the same cl reused concurrently from another thread.
COMMANDS.add_prologue 'zeCommandQueueExecuteCommandLists', <<EOF
  if (_do_profile && numCommandLists > 0 && phCommandLists)
    _on_execute_command_lists_prologue(numCommandLists, phCommandLists, hCommandQueue, hFence);
EOF

# Sync hooks: walk dependency edges from the synced anchor and drain
# everything reachable. Each sync API has a different anchor.
COMMANDS.add_epilogue 'zeCommandQueueSynchronize', <<EOF
  if (_do_profile && _retval == ZE_RESULT_SUCCESS)
    _on_sync(_ZE_SYNC_QUEUE, hCommandQueue);
EOF

COMMANDS.add_epilogue 'zeEventHostSynchronize', <<EOF
  if (_do_profile && _retval == ZE_RESULT_SUCCESS && hEvent)
    _on_sync(_ZE_SYNC_EVENT, hEvent);
EOF

# zeEventQueryStatus is a non-blocking poll, but a ZE_RESULT_SUCCESS return means
# the signaling append has completed — the same fact zeEventHostSynchronize blocks
# for. Apps that observe completion by polling QueryStatus (never HostSynchronize)
# would otherwise never drain those slots, leaking one injected event per
# signalled append. Safe: _slot_drain no-ops on already-drained slots, so repeated
# SUCCESS polls of the same event drain once.
COMMANDS.add_epilogue 'zeEventQueryStatus', <<EOF
  if (_do_profile && _retval == ZE_RESULT_SUCCESS && hEvent)
    _on_sync(_ZE_SYNC_EVENT, hEvent);
EOF

COMMANDS.add_epilogue 'zeCommandListHostSynchronize', <<EOF
  if (_do_profile && _retval == ZE_RESULT_SUCCESS && hCommandList)
    _on_sync(_ZE_SYNC_CL, hCommandList);
EOF

# The Append prologue swaps the user's signal event for our injected event, so
# the user's own event ends up carrying the barrier/signal op timing, not the
# kernel's. If the user queries their event's kernel timestamp themselves,
# serve back the kernel result we stashed at drain so they see kernel timing.
COMMANDS.add_epilogue 'zeEventQueryKernelTimestamp', <<EOF
  if (_do_profile && hEvent && dstptr &&
      _on_query_kernel_timestamp(hEvent, dstptr))
    _retval = ZE_RESULT_SUCCESS;
EOF

# Fence sync: the fence the user passed to Execute is stamped onto each cl
# (in_flight_fence), so a fence wait drains exactly the cls that Execute
# submitted.
COMMANDS.add_epilogue 'zeFenceHostSynchronize', <<EOF
  if (_do_profile && _retval == ZE_RESULT_SUCCESS && hFence)
    _on_sync(_ZE_SYNC_FENCE, hFence);
EOF

# zeFenceQueryStatus is a non-blocking poll, but a ZE_RESULT_SUCCESS return means
# that Execute's cls have completed — the same fact zeFenceHostSynchronize blocks
# for. An app that observes completion by polling the fence (never the blocking
# wait) would otherwise never drain the last Execute's slots. Safe like the event
# QueryStatus hook: _slot_drain no-ops on already-drained slots and runs under the
# state mutex, so repeated SUCCESS polls drain once.
COMMANDS.add_epilogue 'zeFenceQueryStatus', <<EOF
  if (_do_profile && _retval == ZE_RESULT_SUCCESS && hFence)
    _on_sync(_ZE_SYNC_FENCE, hFence);
EOF

# Evict our per-event state once the destroy SUCCEEDS: the driver recycles
# handle addresses, so a fresh event can reuse this one's. Without eviction the
# new event inherits the dead one's stashed kernel timing and a dangling latest-
# signaled slot pointer. Epilogue gated on _retval — a failed destroy (e.g. a bad
# handle) leaves the event alive, its address can't be recycled, data must stay.
COMMANDS.add_epilogue 'zeEventDestroy', <<EOF
  if (_do_profile && _retval == ZE_RESULT_SUCCESS && hEvent)
    _on_destroy_event(hEvent);
EOF

# Dump memory info if required
memory_info_dump = lambda { |ptr_name|
  "_dump_memory_info(hCommandList, #{ptr_name})"
}

memory_info_prologue = lambda { |ptr_names|
  <<EOF
  if (_do_paranoid_memory_location &&
      (tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) || tracepoint_enabled(lttng_ust_ze_properties, memory_info_range))) {
    #{ptr_names.collect { |ptr_name| memory_info_dump.call(ptr_name) }.join(";\n    ")};
  }
EOF
}

COMMANDS.add_prologue 'zeCommandListAppendMemoryRangesBarrier', <<EOF
  if (_do_paranoid_memory_location &&
      (tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) ||
       tracepoint_enabled(lttng_ust_ze_properties, memory_info_range)) &&
      numRanges && pRangeSizes && pRanges && hCommandList)
    for (uint32_t _i = 0; _i < numRanges; _i++)
      _dump_memory_info(hCommandList, "pRanges[_i]");
EOF

COMMANDS.add_prologue 'zeCommandListAppendMemoryCopy', memory_info_prologue.call(%w[dstptr srcptr])
COMMANDS.add_prologue 'zeCommandListAppendMemoryFill', memory_info_prologue.call(['ptr'])
COMMANDS.add_prologue 'zeCommandListAppendMemoryCopyRegion', memory_info_prologue.call(%w[dstptr srcptr])
COMMANDS.add_prologue 'zeCommandListAppendMemoryCopyFromContext', <<EOF
  if (_do_paranoid_memory_location &&
      (tracepoint_enabled(lttng_ust_ze_properties, memory_info_properties) ||
       tracepoint_enabled(lttng_ust_ze_properties, memory_info_range))) {
    if (hCommandList)
      _dump_memory_info(hCommandList, dstptr);
    if (hContextSrc)
      _dump_memory_info_ctx(hContextSrc, srcptr);
  }
EOF
COMMANDS.add_prologue 'zeCommandListAppendImageCopyToMemory', memory_info_prologue.call(['dstptr'])
COMMANDS.add_prologue 'zeCommandListAppendImageCopyFromMemory', memory_info_prologue.call(['srcptr'])
COMMANDS.add_prologue 'zeCommandListAppendMemoryPrefetch', memory_info_prologue.call(['ptr'])
COMMANDS.add_prologue 'zeCommandListAppendMemAdvise', memory_info_prologue.call(['ptr'])
COMMANDS.add_prologue 'zeCommandListAppendQueryKernelTimestamps', memory_info_prologue.call(['dstptr'])
COMMANDS.add_prologue 'zeCommandListAppendWriteGlobalTimestamp', memory_info_prologue.call(['dstptr'])
COMMANDS.add_prologue 'zeCommandListAppendImageCopyToMemoryExt', memory_info_prologue.call(['dstptr'])
COMMANDS.add_prologue 'zeCommandListAppendImageCopyFromMemoryExt', memory_info_prologue.call(['srcptr'])

# WARNING: there seems to be no way to profile if
# zeCommandListAppendEventReset is used or at least
# not very cleanly is used....
#   prologue: always inject _einj. Save user's signal (may be NULL).
#             Swap user's signal -> our injected event.
#   epilogue: on success, call _record_append, which records the slot for
#             drain and re-exposes the user's original signal. The
#             event_profiling tracepoint is attributed to the user's
#             original signal (or inj when user passed NULL).
#   on sync (queue/event/fence/cl-host): drain the recorded slots.
profiling_prologue = lambda { |event_name|
  <<EOF
  ze_event_handle_t _user_signal = #{event_name};
  struct _ze_event_h * _einj = NULL;
  /* Resolved from the cl's stored context (no per-Append
   * zeCommandListGetContextHandle) and threaded to _record_append (epilogue). */
  ze_context_handle_t _ctx = NULL;
  if (_do_profile) {
    _einj = _get_event(hCommandList, &_ctx);
    if (_einj)
      #{event_name} = _einj->event;
    /* If injection failed, fall through with the user's signal unchanged;
     * we won't be able to time this Append, but it still runs. */
  }
EOF
}

profiling_epilogue = lambda { |_event_name, waits_expr = 'phWaitEvents', n_waits_expr = 'numWaitEvents'|
  <<EOF
  if (_do_profile && _einj) {
    if (_retval == ZE_RESULT_SUCCESS) {
      ze_event_handle_t _attr = _user_signal ? _user_signal : _einj->event;
      _record_append(hCommandList, _ctx, _einj, _user_signal,
                     #{waits_expr}, #{n_waits_expr});
      tracepoint(lttng_ust_ze_profiling, event_profiling, _attr);
    } else {
      _put_event(_einj);
    }
  }
EOF
}

paranoid_drift_prologue = <<EOF
  if (_do_paranoid_drift &&
      ZE_DEVICE_GET_GLOBAL_TIMESTAMPS_PTR &&
      tracepoint_enabled(lttng_ust_ze_properties, device_timer))
    _dump_command_list_device_timer(hCommandList);
EOF

%w[zeCommandListAppendLaunchKernel
   zeCommandListAppendBarrier
   zeCommandListAppendLaunchCooperativeKernel
   zeCommandListAppendLaunchKernelIndirect
   zeCommandListAppendLaunchMultipleKernelsIndirect
   zeCommandListAppendMemoryRangesBarrier
   zeCommandListAppendMemoryCopy
   zeCommandListAppendMemoryFill
   zeCommandListAppendMemoryCopyRegion
   zeCommandListAppendMemoryCopyFromContext
   zeCommandListAppendImageCopy
   zeCommandListAppendImageCopyRegion
   zeCommandListAppendImageCopyToMemory
   zeCommandListAppendImageCopyFromMemory
   zeCommandListAppendWriteGlobalTimestamp
   zeCommandListAppendImageCopyToMemoryExt
   zeCommandListAppendImageCopyFromMemoryExt].each do |c|
  # zeCommandListAppendQueryKernelTimestamps intentionally NOT in this list
  # — it has no kernel to time
  COMMANDS.add_prologue c, profiling_prologue.call('hSignalEvent')
  COMMANDS.add_prologue c, paranoid_drift_prologue
  COMMANDS.add_epilogue c, profiling_epilogue.call('hSignalEvent')
end

['zeCommandListAppendSignalEvent'].each do |c|
  COMMANDS.add_prologue c, profiling_prologue.call('hEvent')
  COMMANDS.add_epilogue c, profiling_epilogue.call('hEvent', 'NULL', '0')
end

# WARNING
# zeModuleGetKernelNames, returns an array of strings.
# This is problematic for lttng.

COMMANDS.add_prologue 'zeModuleCreate', <<EOF
  int _build_log_release = 0;
  ze_module_build_log_handle_t _hBuildLog = NULL;
  if (tracepoint_enabled(lttng_ust_ze_build, log)) {
    if (phBuildLog == NULL) {
      phBuildLog = &_hBuildLog;
      _build_log_release = 1;
    }
  }
EOF

COMMANDS.add_epilogue 'zeModuleCreate', <<EOF
  if (tracepoint_enabled(lttng_ust_ze_build, log) && (_retval == ZE_RESULT_SUCCESS || _retval == ZE_RESULT_ERROR_MODULE_BUILD_FAILURE) && *phBuildLog) {
    _dump_build_log(*phBuildLog);
    if (_build_log_release) {
      ZE_MODULE_BUILD_LOG_DESTROY_PTR(*phBuildLog);
      phBuildLog = NULL;
    }
  }
EOF

COMMANDS.add_prologue 'zeModuleDynamicLink', <<EOF
  int _link_log_release = 0;
  ze_module_build_log_handle_t _hLinkLog = NULL;
  if (tracepoint_enabled(lttng_ust_ze_build, log)) {
    if (phLinkLog == NULL) {
      phLinkLog = &_hLinkLog;
      _link_log_release = 1;
    }
  }
EOF

COMMANDS.add_epilogue 'zeModuleDynamicLink', <<EOF
  if (tracepoint_enabled(lttng_ust_ze_build, log) && (_retval == ZE_RESULT_SUCCESS || _retval == ZE_RESULT_ERROR_MODULE_LINK_FAILURE) && *phLinkLog) {
    _dump_build_log(*phLinkLog);
    if (_link_log_release) {
      ZE_MODULE_BUILD_LOG_DESTROY_PTR(*phLinkLog);
      phLinkLog = NULL;
    }
  }
EOF

COMMANDS.add_epilogue 'zeKernelCreate', <<EOF
 if (tracepoint_enabled(lttng_ust_ze_properties, kernel) && (_retval == ZE_RESULT_SUCCESS)) {
    _dump_kernel_properties(*phKernel);
 }
EOF

COMMANDS.select do |c|
  c.name.match(/(ze|zet|zes|zel|zer)Get.*ProcAddrTable/)
end.each do |c|
  parent_type = c['pDdiTable'].type.type.to_s + '_'
  child_types = CONTEXT.struct_map.select { |k, _| k.match(parent_type) }
  str = <<EOF
  #{c.type} _retval;
  if (!_do_ddi_table_forward && !_in_loader_init && pDdiTable && version <= ZE_API_VERSION_CURRENT) {
EOF
  str << '   '
  str << child_types.reverse_each.collect { |k, v|
    version = k.match(/_t_(\d+)_(\d+)/)[1..2]
    sstr = " if (version >= ZE_MAKE_VERSION(#{version[0]},#{version[1]})) {\n"
    v.each { |m|
      sstr << "      pDdiTable->#{m.name} = #{m.type.to_s.sub(/_t$/, '').sub('_pfn', '')}_hid;\n"
    }
    sstr << "      _retval = ZE_RESULT_SUCCESS;\n"
    sstr << '    } else'
  }.join
  str << "\n"
  str << "      goto ukn;\n"
  str << "  } else {\n"
  str << "ukn:\n"
  c.add_prologue str
  c.add_epilogue <<EOF
  }
EOF
end

def get_ffi_type(a)
  if a.type.is_a?(YAMLCAst::Void)
    'ffi_type_void'
  elsif a.type.is_a?(YAMLCAst::Pointer)
    'ffi_type_pointer'
  elsif FFI_TYPE_MAP["#{a.type}"]
    FFI_TYPE_MAP["#{a.type}"]
  else
    raise "Unsupported type: #{a.type}"
  end
end

str = COMMANDS.groups[:lttng_ust_ze].select { |c| c.name.end_with?('Exp') }.map do |c|
  <<EOF
  if (strcmp(name, "#{c.name}") == 0) {
    *ppFunctionAddress = (void *)(intptr_t)#{ZE_POINTER_NAMES[c]};
    tracepoint(lttng_ust_ze, zeDriverGetExtensionFunctionAddress_exit, hDriver, name, ppFunctionAddress, ZE_RESULT_SUCCESS);
    *ppFunctionAddress = (void *)(intptr_t)#{c.name}_hid;
    return ZE_RESULT_SUCCESS;
  }
EOF
end.join(<<EOF)
  else
EOF

COMMANDS.add_prologue 'zeDriverGetExtensionFunctionAddress', str

str = <<EOF
  if (_retval == ZE_RESULT_SUCCESS && *ppFunctionAddress) {
EOF
str << zex_commands.collect { |c|
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
    sstr << <<EOF
          closure->types[#{i}] = &#{get_ffi_type(a)};
EOF
  }
  sstr << <<EOF
          if (ffi_prep_cif(&(closure->cif), FFI_DEFAULT_ABI, #{c.parameters.size}, &#{get_ffi_type(c)}, closure->types) == FFI_OK) {
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

COMMANDS.add_epilogue 'zeDriverGetExtensionFunctionAddress', str

COMMANDS.add_epilogue 'zeMemOpenIpcHandle', <<EOF
  if (_retval == ZE_RESULT_SUCCESS && pptr) {
    _dump_memory_info_ctx(hContext, *pptr);
  }
EOF

COMMANDS.add_epilogue 'zexMemOpenIpcHandles', <<EOF
  if (_retval == ZE_RESULT_SUCCESS && pptr) {
    _dump_memory_info_ctx(hContext, *pptr);
  }
EOF

COMMANDS.add_prologue 'zeLoaderInit', <<EOF
  _in_loader_init = 1;
EOF

COMMANDS.add_epilogue 'zeInit', <<EOF
  _in_loader_init = 0;
EOF
