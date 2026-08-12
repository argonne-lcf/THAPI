require 'ze_validator_zemodel'
require 'ze_library'



# A command queue is created with an (ordinal, index)
# pair -- which engine group, and which queue within that group. Asking oob index segfaults
def check_valid_index_for_ordinal(state,ctx,queue_handle,ordinal,index)
  #puts "entered"
  if state.device_properties
    command_queue_prop = state.device_properties["devices"][0]["command_queue_groups"]
    command_queue_prop.each do |prop|
      #find matching ordinal, and check whether the index is oob
      if prop["ordinal"] == ordinal && (index >= prop["numQueues"] || index < 0)
        state.print_usage_error(ctx, "command queue (#{state.get_handle_str(queue_handle)}) with ordinal = #{ordinal} was created " +
                                     "with index = #{index}. Index value should be: 0<= index < #{prop["numQueues"]}")
      end
    end
  end
end

# Checking whether the application ever called zeDeviceGetCommandQueueGroupProperties
# before calling command queue/list create. Not calling it implies hardcoded ordinals
def check_group_property_queued(state, ctx, defi, device)
  if !(device.cmd_queue_group_properties_queried) && state.print_tracker["check_group_property"] == 0
    state.print_tracker["check_group_property"] = 1
    state.print_usage_error(ctx,"command queue group wasn't queried. Hardcoded group properties may break the code on different devices")
  end
end


# returns the copy ordinals if retrieved from the ze_device_property.json
def copy_only_ordinals(state)
  return [1, 2] unless state.device_properties
  state.device_properties["devices"][0]["command_queue_groups"]
       .select { |prop| prop["type"] == "copy" }
       .map    { |prop| prop["ordinal"] }
end

# checks whether a command list attached to a copy-only engine receives a kernel
def check_valid_ordinal(state, ctx, defi, cqg_ordinal)
  copy_only_ords = copy_only_ordinals(state)
  if copy_only_ords.include?(cqg_ordinal) && state.print_tracker["check_valid_ordinal"] == 0
    state.print_tracker["check_valid_ordinal"] = 1
    kernels = state.find_objects(ctx, 'kernel')
    kernel_handle = state.find_param(ctx, 'hKernel')
    kernel_name = "UNKNOWN" #kernel name wasn't passed, so mark it as unknown
    command_list_handle = state.find_param(ctx, 'hCommandList')
    if kernels[kernel_handle]
      kernel_name = kernels[kernel_handle].name
    end
    state.print_usage_error(ctx, "Launching kernel (#{kernel_name}) to a command list with Copy Ordinal: #{state.get_handle_str(command_list_handle)}")
  end
end

#list of compute launches
COMPUTE_LAUNCH_APIS = ['zeCommandListAppendLaunchKernel',
                       'zeCommandListAppendLaunchCooperativeKernel'].freeze
def command_list_has_kernel_launch?(cmd_list)
  cmd_list && cmd_list.ops.any? { |op| op.kind == :launch && COMPUTE_LAUNCH_APIS.include?(op.api) }
end

#Checks whether a command list that has a compute kernel gets submitted to a command queue that is attached to a copy only engine.
def check_copy_only_queue_submission(state, ctx, queue, cmd_list)
  return unless queue && queue.desc
  return unless command_list_has_kernel_launch?(cmd_list)
  queue_ordinal = queue.desc[:ordinal]
  return unless copy_only_ordinals(state).include?(queue_ordinal)
  key = "copyq-submit-#{state.get_handle_str(queue.handle)}-#{state.get_handle_str(cmd_list.handle)}"
  return unless state.print_tracker[key] == 0
  state.print_tracker[key] = 1
  state.print_usage_error(ctx, "command list #{state.get_handle_str(cmd_list.handle)} contains a compute kernel " \
                               "launch but was submitted to command queue #{state.get_handle_str(queue.handle)} " \
                               "with copy-only ordinal #{queue_ordinal}")
end

# Checks whether the kernel module's context matches that of the command list's.
def check_kernel_list_context_match(state, ctx, defi)
  command_lists = state.find_objects(ctx, 'command_list')
  kernels = state.find_objects(ctx, 'kernel')
  cmd_list = command_lists[defi['hCommandList']]
  kernel = kernels[defi['hKernel']]
  return unless cmd_list && cmd_list.context && kernel
  mod = kernel.module
  return unless mod && mod.context
  return if mod.context == cmd_list.context
  key = "kernel-list-ctx-#{state.get_handle_str(cmd_list.handle)}-#{state.get_handle_str(kernel.handle)}"
  return unless state.print_tracker[key] == 0
  state.print_tracker[key] = 1
  state.print_usage_error(ctx,
    "kernel #{state.get_handle_str(kernel.handle)} (from module " \
    "#{state.get_handle_str(mod.handle)} on context #{state.get_handle_str(mod.context.handle)}) " \
    "does not share the context of command list #{state.get_handle_str(cmd_list.handle)} " \
    "(context #{state.get_handle_str(cmd_list.context.handle)})")
end

# USAGE CHECK: the handle passed to a kernel launch was never produced by a
# zeKernelCreate we saw -- an uninitialized, stale, or wrong variable.
def check_kernel_created(state, ctx, defi)
  kernels = state.find_objects(ctx, 'kernel')
  kernel_handle = defi['hKernel']
  unless kernels[kernel_handle]
    state.print_usage_error(ctx, "kernel: #{state.get_handle_str(kernel_handle)} wasn't created. Consider calling zeKernelCreate")
  end
end

#Checks for using fence without reset
def check_fence_misuse(state, ctx, defi)
  fence_handle = defi['hFence']
  fence = get_fence(state,ctx,fence_handle)
  if fence && (fence.status == fence.signaled || fence.status == fence.in_use)
    state.print_usage_error(ctx, "Used fence: #{state.get_handle_str(fence_handle)} twice without resetting it")
  end
end

# Check whether the queue handed to ExecuteCommandLists was never created (or
# was already destroyed).
def check_valid_command_queue(state,ctx,defi, cmd_queues, cmd_queue_ptr)
  cmd_queue = cmd_queues[cmd_queue_ptr]
  unless cmd_queue 
    state.print_usage_error(ctx, "Invalid commandQueue (#{state.get_handle_str(cmd_queue_ptr)}) was handed to zeCommandQueueExecuteCommandLists")
  end

end

# Checks for three things: submitting nothing at all, submitting a handle that was never created,
# or submitting an IMMEDIATE list -- immediate lists carry their own implicit
# queue and execute at append time, so passing one to a queue is invalid.
def check_valid_command_lists(state, ctx, defi)
  command_lists = defi['phCommandLists_vals']
  known_command_lists = state.find_objects(ctx, 'command_list')
  if command_lists.nil? || command_lists.empty?
    state.print_usage_error(ctx, "No valid commandlist was chosen at zeCommandQueueExecuteCommandLists")
  end

  command_lists.each do |command_list_handle|
    if !(known_command_lists[command_list_handle])
      state.print_usage_error(ctx, "Invalid commandlist (#{command_list_handle}) was handed to zeCommandQueueExecuteCommandLists")
    elsif known_command_lists[command_list_handle] && known_command_lists[command_list_handle].immediate
        state.print_usage_error(ctx, "Immediate Command List was chosen for the Command Queue: #{state.get_handle_str(command_queue_handle)}")
    end
  end
end



# Resolve a fence handle to its model object (nil if unknown).
def get_fence(state,context,fence_handle)
  fences = state.find_objects(context, 'fence')
  fence = fences[fence_handle] #returns fence
end

# Resolve the Level Zero context handle that owns a command list. 
def cmd_list_ctx_handle(state, ctx, cmd_list_handle)
  cmd_list = state.find_objects(ctx, 'command_list')[cmd_list_handle]
  cmd_list && cmd_list.context ? cmd_list.context.handle : nil
end

# retrieves the wait event handles at the current state
def wait_event_handles(state, ctx)
  handles = state.find_param(ctx, 'phWaitEvents_vals') ||
            state.find_param(ctx, 'phEvents_vals') || []
  handles.reject { |h| h.nil? || h == 0 }
end

# Record one op onto its command list. A regular (non-immediate) list
def record_op(state, ctx, cmd_list_handle, op)
  cmd_list = state.find_objects(ctx, 'command_list')[cmd_list_handle]
  return unless cmd_list
  if cmd_list.immediate
    # An immediate list never reaches zeCommandQueueExecuteCommandLists, so
    # check its events' context here (against the list's own context) before the op
    # is scheduled.
    check_event_pool_immediate_list_context_match(state, ctx, cmd_list, op)
    state.enqueue_immediate_op(ctx, op, cmd_list_handle)
  else
    cmd_list.ops << op
  end
end

# Record a memory-copy op (zeCommandListAppendMemoryCopy / MemoryFill).
# recording is necessary because the per-call context is
# gone by the time the op is replayed.
def record_copy_op(state, ctx, api, dst_key, src_key)
  cmd_list_handle = state.find_param(ctx, 'hCommandList')
  op = ZEModel::RecordedOp.new(:copy,
        signal: state.find_param(ctx, 'hSignalEvent'),
        waits: wait_event_handles(state, ctx),
        params: { api: api,
                  ctx_handle: cmd_list_ctx_handle(state, ctx, cmd_list_handle),
                  dst: (dst_key ? state.find_param(ctx, dst_key) : nil),
                  src: (src_key ? state.find_param(ctx, src_key) : nil),
                  size: state.find_param(ctx, 'size') })
  record_op(state, ctx, cmd_list_handle, op)
end

# Records a zeCommandListAppendMemoryRangesBarrier op. 
def record_ranges_barrier_op(state, ctx)
  cmd_list_handle = state.find_param(ctx, 'hCommandList')
  bases = state.find_param(ctx, 'pRanges_vals') || []
  sizes = state.find_param(ctx, 'pRangeSizes_vals') || []
  ranges = bases.each_with_index.map { |base, i| { base: base, size: sizes[i] } }
  op = ZEModel::RecordedOp.new(:ranges_barrier,
        signal: state.find_param(ctx, 'hSignalEvent'),
        waits: wait_event_handles(state, ctx),
        params: { api: 'zeCommandListAppendMemoryRangesBarrier',
                  ctx_handle: cmd_list_ctx_handle(state, ctx, cmd_list_handle),
                  ranges: ranges })
  record_op(state, ctx, cmd_list_handle, op)
end

# Check if a command list was closed before launching anything on it (called at the execute command lists, for non-immediate command queues)
def check_command_list_closed(state, ctx, defi)
  command_queue_handle = defi['hCommandQueue']
  command_lists = defi['phCommandLists_vals'] || []
  known_command_lists = state.find_objects(ctx, 'command_list')
  command_lists.each do |command_list_handle|
    cmd_list = known_command_lists[command_list_handle]
    next unless cmd_list
    if cmd_list.status == ZEModel::CommandList.class_variable_get(:@@INITIALIZED)
      state.print_usage_error(ctx, "commandlist: #{state.get_handle_str(command_list_handle)} wasn't closed before executing on #{state.get_handle_str(command_queue_handle)}")
    elsif cmd_list.status == ZEModel::CommandList.class_variable_get(:@@DESTROYED)
      state.print_usage_error(ctx, "commandlist: #{state.get_handle_str(command_list_handle)} was already destroyed #{state.get_handle_str(command_queue_handle)}")
    end
  end
end


# check if the command list reset is valid or not.
# Invalid calls: reset on destroyed lists, reset on immeidate lists, and reset on command lists that are already exeucting.
def check_command_list_reset(state, ctx, defi)
  handle = defi['hCommandList']
  cmd_list = state.find_objects(ctx, 'command_list')[handle]

  if cmd_list.status == ZEModel::CommandList.class_variable_get(:@@DESTROYED)
    key = "clreset-destroyed-#{state.get_handle_str(handle)}"
    if state.print_tracker[key] == 0
      state.print_tracker[key] = 1
      state.print_usage_error(ctx, "command list #{state.get_handle_str(handle)} was already destroyed before zeCommandListReset")
    end
    return
  end

  if cmd_list.immediate
    key = "clreset-immediate-#{state.get_handle_str(handle)}"
    if state.print_tracker[key] == 0
      state.print_tracker[key] = 1
      state.print_usage_error(ctx, "zeCommandListReset called on immediate command list #{state.get_handle_str(handle)}; " \
                                   "immediate command lists cannot be reset")
    end
  end

  if state.command_list_in_flight?(ctx, handle)
    key = "clreset-inflight-#{state.get_handle_str(handle)}"
    if state.print_tracker[key] == 0
      state.print_tracker[key] = 1
      state.print_usage_error(ctx, "command list #{state.get_handle_str(handle)} is being reset while a prior " \
                                   "zeCommandQueueExecuteCommandLists submission is still in-flight; the device may " \
                                   "still be executing it (undefined behavior)")
    end
  end
end

# checks whether zeKernelCreate was given a null module handle.
def check_valid_module(state,ctx,defi)
  module_handle = state.find_param(ctx, 'hModule')
  if !module_handle || module_handle == 0
    state.print_usage_error(ctx, "Improper hModule was handed")
  end
end


# Checks if the fence's queue and the command list is on the same context.
def check_list_and_fence_have_matching_context(state,ctx,defi,cmd_list,fence)
  if fence 
    unless cmd_list && fence.command_queue &&
          cmd_list.context == fence.command_queue.context
      list_handle = cmd_list ? state.get_handle_str(cmd_list.handle) : "nullptr"
      fence_handle = fence
      state.print_usage_error(ctx, "Mismatching context between command list #{list_handle} and fence #{fence_handle}")
    end
  end
end

# Checks for context between queue and the fence.
# Stronger than a context match, as it checks for the matching of the queue.
def check_fence_and_queue_compatibility(state,ctx,defi,cmd_queue,fence)
  if fence 
    unless cmd_queue && cmd_queue == fence.command_queue
      queue_handle = cmd_queue ? state.get_handle_str(cmd_queue.handle) : "nullptr"
      fence_handle = fence
      state.print_usage_error(ctx, "Associated command queue (#{state.get_handle_str(fence.command_queue.handle)}) of fence #{fence_handle} " +
                                   "is different from the one that was provided #{queue_handle}")
    end
  end
end

# Check the contxt bettween the queue and the list
def check_list_and_queue_have_matching_context(state,ctx,defi,cmd_list, cmd_queue)
  unless cmd_queue && cmd_list && cmd_list.context == cmd_queue.context
    queue_handle = cmd_queue ? state.get_handle_str(cmd_queue.handle) : "nullptr"
    list_handle = cmd_list ? state.get_handle_str(cmd_list.handle) : "nullptr"
    state.print_usage_error(ctx, "Mismatching context between command queue #{queue_handle} and command list #{list_handle}")
  end
end

# List of operations to collect the events from
EVENT_OP_KINDS = [:copy, :launch, :signal, :wait, :reset].freeze

# retrieves the events in a given op
def event_handles_in_op(op)
  return [] unless EVENT_OP_KINDS.include?(op.kind)
  handles = []
  handles << op.signal if op.signal
  handles.concat(op.waits) if op.waits
  handles
end

# returns the distinct event handles a command list references across all of its
# recorded ops that are subject to the same-context requirement.
def event_handles_in_list(cmd_list)
  cmd_list.ops.flat_map { |op| event_handles_in_op(op) }.uniq
end

#Check if all events share the same context
def check_events_share_context(state, ctx, event_handles, ref_context, ref_kind, ref_handle)
  return unless ref_context
  events = state.find_objects(ctx, 'event')
  event_handles.uniq.each do |h|
    ev = events[h]
    next unless ev && ev.event_pool && ev.event_pool.context
    next if ev.event_pool.context == ref_context
    key = "evpool-#{ref_kind}-ctx-#{state.get_handle_str(ref_handle)}-#{state.get_handle_str(h)}"
    next unless state.print_tracker[key] == 0
    state.print_tracker[key] = 1
    state.print_usage_error(ctx,
      "event #{state.get_handle_str(h)} (from event pool " \
      "#{state.get_handle_str(ev.event_pool.handle)} on context " \
      "#{state.get_handle_str(ev.event_pool.context.handle)}) does not share the context of " \
      "#{ref_kind} #{state.get_handle_str(ref_handle)} " \
      "(context #{state.get_handle_str(ref_context.handle)})")
  end
end

# Check if event pool's context matches the command queue's context
def check_event_pool_list_context_match(state, ctx, cmd_list)
  return unless cmd_list
  check_events_share_context(state, ctx, event_handles_in_list(cmd_list),
                             cmd_list.context, 'command list', cmd_list.handle)
end

# Check if event pool's context matches the immediate command list's context
def check_event_pool_immediate_list_context_match(state, ctx, cmd_list, op)
  return unless cmd_list && cmd_list.context
  check_events_share_context(state, ctx, event_handles_in_op(op),
                             cmd_list.context, 'immediate command list', cmd_list.handle)
end


#Find the memory allocation containing the ptr
def find_allocation_containing(allocations, ptr)
  allocations.each_value.find { |m| m.base && m.base <= ptr && ptr < m.base + m.size }
end

# Check whether the copy's endpoints have enough space to support the requested size 
def check_copy_endpoint_oob(state, ctx, allocations, ptr, size, api, role)
  return if ptr.nil? || ptr == 0 || size.nil?
  mem = allocations[ptr] || find_allocation_containing(allocations, ptr)
  return unless mem
  offset = ptr - mem.base
  available = mem.size - offset
  if available < size
    state.print_usage_error(ctx, "#{api}: #{role} memory #{state.get_handle_str(ptr)} only has #{available} " \
                                 "bytes available from this offset but the copy needs #{size} bytes")
  end
end

# Performs the oob check for copy for both endpoints (src and dst)
def check_oob_copy(state, ctx, params)
  api = params[:api] || 'zeCommandListAppendMemoryCopy'
  size = params[:size]
  allocations = state.memory_allocations(ctx, params[:ctx_handle])
  check_copy_endpoint_oob(state, ctx, allocations, params[:dst], size, api, 'destination')
  check_copy_endpoint_oob(state, ctx, allocations, params[:src], size, api, 'source')
end

# Check if the copy is from/to a nullptr
def check_null_copy_ptr(state, ctx, api, endpoints)
  endpoints.each do |role, ptr|
    if ptr.nil? || ptr == 0
      state.print_usage_error(ctx, "#{api}: #{role} pointer is nullptr")
    end
  end
end

# Deletes the address with a new allocation
# An address might be reused after a free. In this case, we need to update the validator's state as well.
def mark_reallocated(state, ctx, ctx_handle, handle, size)
  freed = state.freed_memory_allocations(ctx, ctx_handle)
  return if freed.empty?
  freed.delete_if { |_addr, m| ranges_overlap?(m.base, m.size, handle, size) }
end

# Finds the freed allocation that contains the ptr
def find_freed_allocation_containing(freed, ptr)
  freed.each_value.find { |m| m.base && m.base <= ptr && ptr < m.base + m.size }
end

# ADDED: use-after-free check for one endpoint (dst/src) of a copy/fill. If the
# pointer does NOT resolve to a live allocation but DOES fall inside an
# allocation that was already zeMemFree'd, report a use-after-free. A pointer
# that matches neither is left alone (unknown / untraced -- nothing to assert).
def check_uaf_endpoint(state, ctx, live, freed, ptr, api, role)
  return if ptr.nil? || ptr == 0
  #still live (exact base or an offset within a live allocation) -> fine
  return if live[ptr] || find_allocation_containing(live, ptr)
  mem = freed[ptr] || find_freed_allocation_containing(freed, ptr)
  return unless mem
  #dedup: the same freed pointer can be seen both at append-entry and again at
  #deferred execute time -- report it once per (api, pointer).
  key = "uaf-#{api}-#{state.get_handle_str(ptr)}"
  return unless state.print_tracker[key] == 0
  state.print_tracker[key] = 1
  offset = ptr - mem.base
  where = offset == 0 ? "" : " (offset #{offset} into the freed allocation)"
  state.print_memory_error(ctx, "#{api}: #{role} memory #{state.get_handle_str(ptr)}#{where} was already " \
                                "freed#{mem.freed_by ? " by #{mem.freed_by}" : ""}; use-after-free")
end

# Performs the actual checks for when an API uses a memory that has been freed
def check_use_after_free(state, ctx, params)
  api  = params[:api] || 'zeCommandListAppendMemoryCopy'
  # CHANGED: resolve both maps within the copy's own context (see check_oob_copy).
  live  = state.memory_allocations(ctx, params[:ctx_handle])
  freed = state.freed_memory_allocations(ctx, params[:ctx_handle])
  return if freed.empty?
  check_uaf_endpoint(state, ctx, live, freed, params[:dst], api, 'destination')
  check_uaf_endpoint(state, ctx, live, freed, params[:src], api, 'source')
end

# calls the check_use_after_free only if the wait events have been satisfied
def check_use_after_free_on_append(state, ctx, params, waits)
  return unless state.waits_satisfied?(ctx, waits)
  check_use_after_free(state, ctx, params)
end

# Checks for uaf on memory ranges barrier
def check_uaf_ranges_barrier(state, ctx, params)
  api   = params[:api] || 'zeCommandListAppendMemoryRangesBarrier'
  live  = state.memory_allocations(ctx, params[:ctx_handle])
  freed = state.freed_memory_allocations(ctx, params[:ctx_handle])
  return if freed.empty?
  (params[:ranges] || []).each do |r|
    check_uaf_endpoint(state, ctx, live, freed, r[:base], api, 'range')
  end
end

# returns true if [a, a+asize) and [b, b+bsize) overlap.
def ranges_overlap?(a, asize, b, bsize)
  return false unless a && b && asize && bsize
  a < b + bsize && b < a + asize
end

# ADDED: free-while-in-flight check. Called from zeMemFree BEFORE the allocation
# is removed. If any copy/fill op still pending in an in-flight deferred command
# list references (overlaps) the allocation being freed, the device may still
# read/write it after the free -- report it. mem is the ZEModel::Memory about to
# be freed.
def check_free_in_flight(state, ctx, mem)
  return unless mem
  # ADDED: the buffer being freed belongs to one context; only an in-flight copy
  # in that SAME context can alias it. Comparing across contexts would be a false
  # positive now that addresses may repeat between contexts.
  mem_ctx_handle = mem.context ? mem.context.handle : nil
  state.each_inflight_copy_op(ctx) do |unit, op|
    p = op.params
    next unless p[:ctx_handle] == mem_ctx_handle
    hit = [[p[:dst], 'destination'], [p[:src], 'source']].find do |ptr, _role|
      ptr && ptr != 0 && ranges_overlap?(mem.base, mem.size, ptr, p[:size])
    end
    next unless hit
    _ptr, role = hit
    state.print_memory_error(ctx, "memory #{state.get_handle_str(mem.base)} is being freed while still in use as " \
                                  "the #{role} of an in-flight #{p[:api] || 'copy'} on #{unit.label}; the device " \
                                  "may access freed memory")
  end
end

def check_ptrs_have_same_context(state,ctx,params)
  # CHANGED: resolve within the copy's context sub-map. NOTE: this is currently a
  # stub (empty body) and unregistered. With allocations now keyed by context,
  # both endpoints found in one sub-map necessarily share a context by
  # construction, so a genuine cross-context-copy check would instead have to
  # search every context's sub-map for each pointer -- left for when this is implemented.
  allocations = state.memory_allocations(ctx, params[:ctx_handle])
  if allocations[params[:dst]] && allocations[params[:src]] && (allocations[params[:dst]].context != allocations[params[:src]].context)

  end
end

# ADDED: search a single context sub-map for the allocation matching ptr -- an
# exact base hit first, then an allocation whose [base, base+size) range contains
# ptr (offset copy). Returns the Memory or nil.
def find_memory_in_submap(submap, ptr)
  submap[ptr] || find_allocation_containing(submap, ptr)
end

# ADDED: locate the KNOWN allocation for ptr across this process's per-context
# allocation sub-maps, preferring the command list's own context. Returns
# [memory, ctx_handle], or [nil, nil] if ptr matches no tracked allocation.
# Preferring the list's context keeps the check false-positive-free under address
# aliasing: L0 addresses are unique only within a context, so the same numeric
# address can exist in several contexts. If ptr resolves in the list's own context
# we return that (a correct, in-context copy) and stop; only if it resolves solely
# in a foreign context do we surface a mismatch.
#
# NOTE: IPC-imported memory is intentionally NOT handled here -- the validator does
# not model zeMemOpenIpcHandle, so such pointers are simply "not found" and skipped
# (no callback registers them). Only pointers we positively tracked are considered.
def find_known_memory_preferring_context(state, ctx, ptr, prefer_ctx_handle)
  return [nil, nil] if ptr.nil? || ptr == 0
  all_maps = state.get_process(ctx).memory_allocations
  if prefer_ctx_handle && all_maps.key?(prefer_ctx_handle)
    mem = find_memory_in_submap(all_maps[prefer_ctx_handle], ptr)
    return [mem, prefer_ctx_handle] if mem
  end
  all_maps.each do |cth, submap|
    next if cth == prefer_ctx_handle
    mem = find_memory_in_submap(submap, ptr)
    return [mem, cth] if mem
  end
  [nil, nil]
end

# ADDED: report one endpoint (dst/src) of a copy/fill only when its pointer
# resolves to a KNOWN allocation on a DIFFERENT context than the command list. The
# spec requires the command list and the copied memory to share a context
# (zeCommandListAppendMemory{Copy,Fill}: "the command list and events were created,
# and the memory was allocated, on the same context"). No false positives: a ptr
# found in the list's own context is accepted, and a ptr found in NO tracked
# context -- system/malloc host memory, an untracked/IPC allocation, or memory from
# before tracing started -- is skipped (nothing is asserted about unknown pointers,
# mirroring the OOB/UAF checks). Deduped once per (command list, endpoint, pointer).
def check_ptr_endpoint_list_context(state, ctx, list_ctx_handle, list_handle, ptr, api, role)
  return if ptr.nil? || ptr == 0
  return if list_ctx_handle.nil? # unknown command list context (mid-stream) -> skip
  mem, found_ctx = find_known_memory_preferring_context(state, ctx, ptr, list_ctx_handle)
  return unless mem                      # unknown pointer -> skip (no false alarm)
  return if found_ctx == list_ctx_handle # correctly in the list's context -> fine
  key = "ptr-list-ctx-#{state.get_handle_str(list_handle)}-#{role}-#{state.get_handle_str(ptr)}"
  return unless state.print_tracker[key] == 0
  state.print_tracker[key] = 1
  mem_ctx_str = mem.context ? state.get_handle_str(mem.context.handle) : state.get_handle_str(found_ctx)
  state.print_usage_error(ctx,
    "#{api}: #{role} memory #{state.get_handle_str(ptr)} was allocated on context #{mem_ctx_str} " \
    "but command list #{state.get_handle_str(list_handle)} is on context #{state.get_handle_str(list_ctx_handle)}; " \
    "the command list and copied memory must share a context")
end

# ADDED: append-entry check that a copy/fill's KNOWN memory endpoints were
# allocated on the same context as the command list. Runs at ENTRY (a cross-context
# copy can be rejected inside the append, which then emits no _exit event), reading
# the input pointers directly from defi. Context is a static property of the
# pointer and the list, so unlike OOB/UAF this needs no deferral to execute time.
def check_copy_ptr_list_context(state, ctx, api, list_handle, endpoints)
  list_ctx_handle = cmd_list_ctx_handle(state, ctx, list_handle)
  endpoints.each do |role, ptr|
    check_ptr_endpoint_list_context(state, ctx, list_ctx_handle, list_handle, ptr, api, role)
  end
end

# ADDED: detect misuse of an event that is signaled while already signaled, with
# no intervening reset. Mirrors the fence double-signal check. Two shapes:
#   * reuse-no-reset -- the prior signal WAS observed by the host (e.g. it
#                       synchronized on the event) and the event is reused as a
#                       signal target without a reset first.
#   * double-signal  -- the prior signal was never observed; two signalers target
#                       the same event with no consumer between them.
# Called just before an op applies its own signal, in execution order, so any
# intervening reset/observe has already been recorded. `who` names the signaler.
def check_event_signal_reuse(state, ctx, handle, who)
  ev = state.event_by_handle(ctx, handle)
  return unless ev && ev.signaled?
  if ev.observed
    state.print_usage_error(ctx, "event #{state.get_handle_str(handle)} was reused as a signal target by #{who} " \
                                 "without calling zeEventHostReset/zeCommandListAppendEventReset after it was " \
                                 "signaled#{ev.signaled_by ? " by #{ev.signaled_by}" : ""}")
  else
    state.print_usage_error(ctx, "event #{state.get_handle_str(handle)} was signaled by #{who} before being reset " \
                                 "or consumed#{ev.signaled_by ? " (already signaled by #{ev.signaled_by})" : ""}; " \
                                 "concurrent signals of the same event are undefined")
  end
end

# ADDED: report wait-events that were never signaled by end of trace -- a
# deferred op that could never complete (missing signal or deadlock).
def report_unsignaled_waits(state, ctx, waits)
  (waits || []).each do |h|
    ev = state.event_by_handle(ctx, h)
    next unless ev && !ev.signaled?
    state.print_usage_error(ctx, "event #{state.get_handle_str(h)} was never signaled; a deferred command list " \
                                 "operation could not complete (possible deadlock or missing signal)")
  end
end

# ---------------------------------------------------------------------------
# DEADLOCK DETECTION.
#
# GPU work is ordered by events, and it is easy to write an event graph that
# can never make progress. The classic shape:
#
#     command list A:  wait(evB) ... signal(evA)
#     command list B:  wait(evA) ... signal(evB)
#
# Neither can start, because each is waiting for something the other only
# produces after it has itself started. On real hardware this manifests as an
# application that simply hangs, with no error from the runtime.
#
# The validator sees this for free from its deferred-execution machinery: a
# DeferredUnit that is still parked when the trace ends could never proceed.
# The two functions below classify why:
#   check_circular_deadlock       - a cycle ACROSS units (the shape above)
#   check_in_order_self_deadlock  - a cycle WITHIN one in-order list
# ---------------------------------------------------------------------------

# ADDED: detect a circular event dependency (deadlock) among the deferred units
# still stuck at end of trace, and report the FIRST cycle found. Builds a
# wait-for graph -- unit U points to unit V when U is blocked on an event that
# only V can still signal (it is in V's pending_signals) -- then searches for one
# cycle. A cycle means every unit on it waits for an event another unit on the
# cycle only signals after finishing, so none can ever start (e.g. clA waits
# evB/signals evA while clB waits evA/signals evB).
#
# Only the first cycle is reported: cycles overlap and share units, so emitting
# every one found would restate a single underlying bug many times over.
def check_circular_deadlock(state, units)
  stuck = units.select { |u| u.blocked_on && !u.blocked_on.empty? }
  return if stuck.empty?

  # event handle -> units that may still signal it
  signalers = Hash.new { |h, k| h[k] = [] }
  stuck.each { |u| u.pending_signals.each { |ev| signalers[ev] << u } }

  # adjacency: U -> V if U waits on an event V still owes
  succ = Hash.new { |h, k| h[k] = [] }
  stuck.each do |u|
    u.blocked_on.each do |ev|
      signalers[ev].each { |v| succ[u] << v unless v.equal?(u) }
    end
  end

  # DFS; stop at the first cycle and report only that one
  # Textbook cycle detection by depth-first search with two marker sets:
  #   on_path - units on the current DFS branch. Reaching one again closes a
  #             cycle, and the cycle is the tail of `path` from that unit on.
  #   visited - units fully explored on some earlier branch; no need to redo
  #             them, which keeps this linear in the size of the graph.
  # The lambda returns true once a cycle is found, so the recursion and the
  # outer loop both unwind immediately.
  path = []
  on_path = {}
  visited = {}
  found = nil
  dfs = lambda do |u|
    return true if found
    on_path[u] = true
    path.push(u)
    succ[u].uniq.each do |v|
      if on_path[v]
        found = path[path.index(v)..] # the cycle, from v back to the current node
        break
      elsif !visited[v]
        break if dfs.call(v)
      end
    end
    path.pop
    on_path[u] = false
    visited[u] = true
    !found.nil?
  end
  #the graph may be disconnected, so start a search from each stuck unit until
  #one of them turns up a cycle
  stuck.each { |u| break if dfs.call(u); }
  report_deadlock_cycle(state, found) if found
end

# ADDED: report one deadlock cycle, naming each unit AND the specific command
# (op) it is stuck on -- e.g. "command_list 0x..::zeCommandListAppendMemoryCopy".
# The blocked command is the unit's current_op (the cursor is parked on it and
# blocked_on holds exactly that op's unsatisfied waits), so the chain reads
# <list>::<blocking API> -> <list>::<blocking API> -> ... back to the first.
def deadlock_node_label(state, unit)
  op = unit.current_op
  # op.api is set for every op that can carry waits (copy/launch/barrier/wait);
  # fall back to the op kind for anything else so the label is never blank.
  api = op ? (op.api || op.kind.to_s) : 'unknown'
  waits = unit.blocked_on.map { |h| state.get_handle_str(h) }.join(', ')
  "#{unit.label}::#{api} (waiting on event #{waits})"
end

def report_deadlock_cycle(state, cycle)
  ctx = cycle.first.context
  desc = cycle.map { |u| deadlock_node_label(state, u) }.join(" -> ")
  # close the loop for readability
  desc << " -> #{deadlock_node_label(state, cycle.first)}"
  state.print_deadlock_error(ctx, "circular event dependency among command list operations; none can start: #{desc}")
end

# ADDED: detect an intra-list deadlock in an IN-ORDER command list. Such a list
# runs its ops strictly in append order (op N+1 cannot start until op N
# completes), so if the op the unit is parked on waits on an event that only a
# LATER op in the SAME list will signal, that later op can never be reached --
# the list deadlocks on itself. The cross-list detector cannot see this because
# it drops self-edges. Run at end-of-trace (flush): a unit still parked here was
# never rescued by an external host signal, so the wait is genuinely unmet.
#
# unit.pending_signals holds exactly the events signaled by ops at/after the
# cursor, so blocked_on & pending_signals = waits only a later op in this list
# owes -- the self-deadlock condition -- with no extra bookkeeping.
def check_in_order_self_deadlock(state, units)
  units.each do |unit|
    next unless unit.in_order
    next if unit.blocked_on.nil? || unit.blocked_on.empty?
    self_waits = unit.blocked_on & unit.pending_signals
    self_waits.each do |ev|
      #the later op in this same list that would signal ev (but never runs)
      later = unit.ops[(unit.cursor + 1)..]&.find { |o| o.signal == ev }
      report_in_order_self_deadlock(state, unit, ev, later)
    end
  end
end

# ADDED: report one intra-list self-deadlock in the op-level arrow format:
#   <list>::<waiting op> (waits on event 0xE) ->
#   <list>::<signaling op> (signals event 0xE later in the same in-order list)
def report_in_order_self_deadlock(state, unit, ev, signaling_op)
  waiting = unit.current_op
  waiting_api = waiting ? (waiting.api || waiting.kind.to_s) : 'unknown'
  signaling_api = signaling_op ? (signaling_op.api || signaling_op.kind.to_s) : 'unknown'
  ev_str = state.get_handle_str(ev)
  desc = "#{unit.label}::#{waiting_api} (waits on event #{ev_str}) -> " \
         "#{unit.label}::#{signaling_api} (signals event #{ev_str} later in the same in-order list)"
  state.print_deadlock_error(unit.context,
    "in-order command list cannot complete; an earlier command waits on an event a later " \
    "command in the same list signals: #{desc}")
end

# USAGE CHECK: every Level Zero descriptor struct begins with an `stype` field
# naming its own type (ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC and so on). Setting
# it wrong -- usually by copy-pasting the initialization of a different
# descriptor -- does not fail today: current drivers ignore it. But the field
# exists so the runtime can version and extend structs, so a wrong stype is a
# latent bug that a future driver is entitled to reject. Reported once per
# expected stype.
def check_struct_stype_misuse(state,ctx,defi,expected_stype, observed_stype)
  if expected_stype != observed_stype && state.print_tracker[expected_stype] == 0
      state.print_tracker[expected_stype] = 1
      state.print_usage_error(ctx,"\nExpected stype of #{expected_stype}\nbut #{observed_stype} was observed.")
  end
end
