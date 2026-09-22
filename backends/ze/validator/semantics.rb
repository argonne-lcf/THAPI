# frozen_string_literal: true

require 'ze/validator/model'
require 'ze_library'
require 'rgl/adjacency'
require 'rgl/traversal'

# The engine groups recorded for a device, or nil when the trace carries none.
#
# Must not use plain indexing: state.device_properties defaults a missing key to
# {}, which would both pollute the map and make "topology unknown" look
# identical to "device has no engine groups".
def device_command_queue_groups(state, device_handle)
  return nil unless device_handle

  all = state.device_properties
  return nil unless all.key?(device_handle)

  groups = all[device_handle]
  groups.empty? ? nil : groups
end

# Checks for oob index. A command queue is created with an (ordinal, index)
# pair -- which engine group, and which queue within that group.
# Silent when the topology is unknown: this only runs after zeCommandQueueCreate
# already failed, so a "could not check" note would just be noise there.
def check_valid_index_for_ordinal(state, ctx, device_handle, cmd_q_handle, ordinal, index)
  groups = device_command_queue_groups(state, device_handle)
  return if groups.nil?

  groups.each do |ordinal_key, info|
    # find matching ordinal, and check whether the index is oob
    next unless ordinal_key == ordinal && (index >= info['numQueues'] || index.negative?)
    state.report(:queue_index_out_of_range, ctx,
                 "command queue (#{state.get_handle_str(cmd_q_handle)}) with ordinal = #{ordinal} was created " \
                 "with index = #{index}. Index value should be: 0<= index < #{info['numQueues']}")
  end
end

# Checking whether the application ever called zeDeviceGetCommandQueueGroupProperties
# before calling command queue/list create. Not calling it implies hardcoded ordinals
def check_group_property_queued(state, ctx, _payload, device)
  return if device.cmd_queue_group_properties_queried

  state.report(:command_queue_group_not_queried, ctx,
               "command queue group wasn't queried. Hardcoded group properties may break the code on different devices",
               key: 'check_group_property')
end

# The copy-only ordinals of the command list's device, or nil when the trace
# does not carry that device's engine topology.
#
# nil is not "this device has no copy-only engine": it means the question cannot
# be answered and the caller must skip instead of guessing. THAPI emits the
# command_queue_group tracepoint only for root devices, and only when the
# properties channel is enabled, so a list created on a sub-device lands here.
def copy_only_ordinals(state, cmd_list)
  groups = device_command_queue_groups(state, cmd_list&.device&.handle)
  return nil unless groups

  groups.filter_map do |ordinal, prop|
    flags = prop['flags']
    ordinal if flags.include?(:ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) &&
               !flags.include?(:ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE)
  end
end

# Records that a copy-engine check could not run for want of topology. Deduped
# per device, so one trace reports each unknown device once whichever check hit
# it first.
def report_unknown_engine_topology(state, ctx, cmd_list)
  handle = cmd_list&.device ? state.get_handle_str(cmd_list.device.handle) : 'unknown'
  state.report(:engine_topology_unknown, ctx,
               "copy-engine checks skipped for device #{handle}: the trace carries no command queue " \
               'group properties for it (THAPI records them for root devices only, and only when the ' \
               'properties channel is enabled)',
               key: "engine-topology-#{handle}")
end

# checks whether a command list attached to a copy-only engine receives a kernel
def check_valid_ordinal(state, ctx, _payload, cqg_ordinal, cmd_list)
  copy_only = copy_only_ordinals(state, cmd_list)
  if copy_only.nil?
    report_unknown_engine_topology(state, ctx, cmd_list)
    return
  end
  return unless copy_only.include?(cqg_ordinal)

  kernels = state.find_objects(ctx, 'kernel')
  kernel_handle = state.find_param(ctx, 'hKernel')
  command_list_handle = state.find_param(ctx, 'hCommandList')
  kernel_name = kernels[kernel_handle]&.name || 'UNKNOWN'
  state.report(:kernel_on_copy_only_list, ctx,
               "launching kernel (#{kernel_name}) on command list #{state.get_handle_str(command_list_handle)}, " \
               "which is bound to copy-only ordinal #{cqg_ordinal}",
               key: "copy-ordinal-#{state.get_handle_str(command_list_handle)}-#{kernel_name}")
end

# list of compute launches
COMPUTE_LAUNCH_APIS = %w[zeCommandListAppendLaunchKernel
                         zeCommandListAppendLaunchCooperativeKernel].freeze

def command_list_has_kernel_launch?(cmd_list)
  return false unless cmd_list

  cmd_list.ops.any? { |op| op.kind == :launch && COMPUTE_LAUNCH_APIS.include?(op.api) }
end

# Checks whether a command list that has a compute kernel gets submitted to a command queue that is attached to a copy only engine.
def check_copy_only_queue_submission(state, ctx, queue, cmd_list)
  return unless queue.desc && command_list_has_kernel_launch?(cmd_list)

  queue_ordinal = queue.desc[:ordinal]
  copy_only = copy_only_ordinals(state, cmd_list)
  if copy_only.nil?
    report_unknown_engine_topology(state, ctx, cmd_list)
    return
  end
  return unless copy_only.include?(queue_ordinal)

  state.report(:compute_list_on_copy_only_queue, ctx,
               "command list #{state.get_handle_str(cmd_list.handle)} contains a compute kernel " \
               "launch but was submitted to command queue #{state.get_handle_str(queue.handle)} " \
               "with copy-only ordinal #{queue_ordinal}",
               key: "copyq-submit-#{state.get_handle_str(queue.handle)}-#{state.get_handle_str(cmd_list.handle)}")
end

# Checks whether the kernel module's context matches that of the command list's.
def check_kernel_list_context_match(state, ctx, payload)
  command_lists = state.find_objects(ctx, 'command_list')
  kernels = state.find_objects(ctx, 'kernel')
  cmd_list = command_lists[payload['hCommandList']]
  kernel = kernels[payload['hKernel']]
  return unless cmd_list&.context && kernel

  mod = kernel.module
  return unless mod&.context && mod.context != cmd_list.context

  state.report(:kernel_list_context_mismatch, ctx,
               "kernel #{state.get_handle_str(kernel.handle)} (from module " \
               "#{state.get_handle_str(mod.handle)} on context #{state.get_handle_str(mod.context.handle)}) " \
               "does not share the context of command list #{state.get_handle_str(cmd_list.handle)} " \
               "(context #{state.get_handle_str(cmd_list.context.handle)})",
               key: "kernel-list-ctx-#{state.get_handle_str(cmd_list.handle)}-#{state.get_handle_str(kernel.handle)}")
end

# Checks if the kernel was created
def check_kernel_created(state, ctx, payload)
  kernels = state.find_objects(ctx, 'kernel')
  kernel_handle = payload['hKernel']
  return if kernels[kernel_handle]

  state.report(:kernel_not_created, ctx,
               "kernel #{state.get_handle_str(kernel_handle)} wasn't created. Consider calling zeKernelCreate")
end

# Checks for using fence without reset
def check_fence_misuse(state, ctx, payload)
  fence_handle = payload['hFence']
  fence = get_fence(state, ctx, fence_handle)
  return unless fence && (fence.status == fence.signaled || fence.status == fence.in_use)

  state.report(:fence_reuse_without_reset, ctx,
               "fence #{state.get_handle_str(fence_handle)} was used twice without being reset")
end

# Checks for synchronizing on a fence that is already signaled.
def check_fence_sync_without_reset(state, ctx, fence_handle, fence)
  return unless fence && fence.status == fence.signaled

  state.report(:fence_reuse_without_reset, ctx,
               "fence #{state.get_handle_str(fence_handle)} was synchronized again without being reset")
end

# Check whether the queue handed to ExecuteCommandLists was never created (or was already destroyed).
def check_valid_command_queue(state, ctx, _payload, cmd_queues, cmd_queue_ptr)
  cmd_queue = cmd_queues[cmd_queue_ptr]
  return if cmd_queue

  state.report(:unknown_command_queue, ctx,
               "command queue #{state.get_handle_str(cmd_queue_ptr)} handed to " \
               'zeCommandQueueExecuteCommandLists was never created (or was already destroyed)',
               key: "unknown-queue-#{state.get_handle_str(cmd_queue_ptr)}")
end

# Checks for submitting nothing, submitting a handle that was never created, or
# submitting an immediate list, which carries its own queue.
def check_valid_command_lists(state, ctx, payload)
  command_queue_handle = payload['hCommandQueue']
  command_lists = payload['phCommandLists_vals']
  known_command_lists = state.find_objects(ctx, 'command_list')
  if command_lists.nil? || command_lists.empty?
    state.report(:no_command_list_submitted, ctx,
                 'no command list was submitted to zeCommandQueueExecuteCommandLists')
    return
  end

  command_lists.each do |command_list_handle|
    cmd_list = known_command_lists[command_list_handle]
    if !cmd_list
      state.report(:unknown_command_list, ctx,
                   "command list #{state.get_handle_str(command_list_handle)} handed to " \
                   'zeCommandQueueExecuteCommandLists was never created (or was already destroyed)',
                   key: "unknown-list-#{state.get_handle_str(command_list_handle)}")
    elsif cmd_list.immediate
      state.report(:immediate_list_submitted, ctx,
                   "immediate command list #{state.get_handle_str(command_list_handle)} was submitted to " \
                   "command queue #{state.get_handle_str(command_queue_handle)}; an immediate list carries its own queue",
                   key: "immediate-submit-#{state.get_handle_str(command_list_handle)}")
    end
  end
end

# Resolve a fence handle to its model object (nil if unknown).
def get_fence(state, context, fence_handle)
  fences = state.find_objects(context, 'fence')
  fences[fence_handle] # returns fence
end

# Resolve the Level Zero context handle that owns a command list.
def cmd_list_ctx_handle(state, ctx, cmd_list_handle)
  cmd_list = state.find_objects(ctx, 'command_list')[cmd_list_handle]
  cmd_list&.context&.handle
end

# retrieves the wait event handles at the current state
def wait_event_handles(state, ctx)
  handles = state.find_param(ctx, 'phWaitEvents_vals') ||
            state.find_param(ctx, 'phEvents_vals') || []
  handles.reject(&:zero?)
end

# Record one op onto a command list
def record_op(state, ctx, cmd_list_handle, op)
  cmd_list = state.find_objects(ctx, 'command_list')[cmd_list_handle]
  return unless cmd_list

  if cmd_list.immediate
    check_event_pool_immediate_list_context_match(state, ctx, cmd_list, op)
    state.enqueue_immediate_op(ctx, op, cmd_list_handle, in_order: cmd_list.in_order)
  else
    cmd_list.ops << op
  end
end

# Record a memory-copy op (zeCommandListAppendMemoryCopy / MemoryFill).
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
def check_command_list_closed(state, ctx, payload)
  command_queue_handle = payload['hCommandQueue']
  command_lists = payload['phCommandLists_vals'] || []
  known_command_lists = state.find_objects(ctx, 'command_list')
  command_lists.each do |command_list_handle|
    cmd_list = known_command_lists[command_list_handle]
    next unless cmd_list

    if cmd_list.status == ZEModel::CommandList.class_variable_get(:@@INITIALIZED)
      state.report(:command_list_not_closed, ctx,
                   "command list #{state.get_handle_str(command_list_handle)} wasn't closed before being executed " \
                   "on command queue #{state.get_handle_str(command_queue_handle)}",
                   key: "not-closed-#{state.get_handle_str(command_list_handle)}")
    elsif cmd_list.status == ZEModel::CommandList.class_variable_get(:@@DESTROYED)
      state.report(:command_list_already_destroyed, ctx,
                   "command list #{state.get_handle_str(command_list_handle)} was already destroyed when submitted " \
                   "to command queue #{state.get_handle_str(command_queue_handle)}",
                   key: "submit-destroyed-#{state.get_handle_str(command_list_handle)}")
    end
  end
end

# check if the command list reset is valid or not.
# Invalid calls: reset on destroyed lists, reset on immeidate lists, and reset on command lists that are already exeucting.
def check_command_list_reset(state, ctx, payload)
  handle = payload['hCommandList']
  cmd_list = state.find_objects(ctx, 'command_list')[handle]
  return unless cmd_list

  if cmd_list.status == ZEModel::CommandList.class_variable_get(:@@DESTROYED)
    state.report(:command_list_reset_after_destroy, ctx,
                 "command list #{state.get_handle_str(handle)} was already destroyed before zeCommandListReset",
                 key: "clreset-destroyed-#{state.get_handle_str(handle)}")
    return
  end

  if cmd_list.immediate
    state.report(:command_list_reset_immediate, ctx,
                 "zeCommandListReset called on immediate command list #{state.get_handle_str(handle)}; " \
                 'immediate command lists cannot be reset',
                 key: "clreset-immediate-#{state.get_handle_str(handle)}")
  end

  return unless state.command_list_in_flight?(ctx, handle)

  state.report(:command_list_reset_in_flight, ctx,
               "command list #{state.get_handle_str(handle)} is being reset while a prior " \
               'zeCommandQueueExecuteCommandLists submission is still in-flight; the device may ' \
               'still be executing it (undefined behavior)',
               key: "clreset-inflight-#{state.get_handle_str(handle)}")
end

# checks whether zeKernelCreate was given a null module handle.
def check_valid_module(state, ctx, _payload)
  module_handle = state.find_param(ctx, 'hModule')
  return unless !module_handle || module_handle.zero?

  state.report(:null_module_handle, ctx, 'a null hModule was handed to zeKernelCreate')
end

# Checks whether zeEventCreate was given an event pool that was never created.
def check_valid_event_pool(state, ctx, payload)
  pool_handle = payload['hEventPool']
  return unless !pool_handle || pool_handle.zero?

  state.report(:null_event_pool_handle, ctx, 'a null hEventPool was handed to zeEventCreate')
end

# Checks if the fence's queue and the command list is on the same context.
def check_list_and_fence_have_matching_context(state, ctx, _payload, cmd_list, fence)
  return unless cmd_list&.context && fence&.command_queue&.context && cmd_list.context != fence.command_queue.context

  list_handle = state.get_handle_str(cmd_list.handle)
  fence_handle = state.get_handle_str(fence.handle)
  state.report(:list_fence_context_mismatch, ctx,
               "mismatching context between command list #{list_handle} and fence #{fence_handle}",
               key: "list-fence-ctx-#{list_handle}-#{fence_handle}")
end

# Checks for context between queue and the fence.
# Stronger than a context match, as it checks for the matching of the queue.
def check_fence_and_queue_compatibility(state, ctx, _payload, cmd_queue, fence)
  return unless fence && cmd_queue != fence.command_queue

  queue_handle = state.get_handle_str(cmd_queue.handle)
  fence_handle = state.get_handle_str(fence.handle)
  state.report(:fence_queue_mismatch, ctx,
               "fence #{fence_handle} was created on command queue " \
               "#{state.get_handle_str(fence.command_queue.handle)} but was submitted to #{queue_handle}",
               key: "fence-queue-#{fence_handle}-#{queue_handle}")
end

# Check the context between the queue and the list
def check_list_and_queue_have_matching_context(state, ctx, _payload, cmd_list, cmd_queue)
  return if cmd_list && cmd_list.context == cmd_queue.context

  queue_handle = state.get_handle_str(cmd_queue.handle)
  list_handle = cmd_list ? state.get_handle_str(cmd_list.handle) : 'nullptr'
  state.report(:list_queue_context_mismatch, ctx,
               "mismatching context between command queue #{queue_handle} and command list #{list_handle}",
               key: "list-queue-ctx-#{queue_handle}-#{list_handle}")
end

# List of operations to collect the events from
EVENT_OP_KINDS = %i[copy launch signal wait reset].freeze

# retrieves the events in a given op
def event_handles_in_op(op)
  handles = []
  if EVENT_OP_KINDS.include?(op.kind)
    handles << op.signal if op.signal
    handles.concat(op.waits)
  end
  handles
end

# returns the distinct event handles a command list references across all of its
# recorded ops that are subject to the same-context requirement.
def event_handles_in_list(cmd_list)
  cmd_list.ops.flat_map { |op| event_handles_in_op(op) }.uniq
end

# Check if all events share the same context
def check_events_share_context(state, ctx, event_handles, ref_context, ref_kind, ref_handle)
  return unless ref_context

  events = state.find_objects(ctx, 'event')
  event_handles.uniq.each do |h|
    ev = events[h]
    next if !(ev && ev.event_pool && ev.event_pool.context) || ev.event_pool.context == ref_context

    state.report(:event_list_context_mismatch, ctx,
                 "event #{state.get_handle_str(h)} (from event pool " \
                 "#{state.get_handle_str(ev.event_pool.handle)} on context " \
                 "#{state.get_handle_str(ev.event_pool.context.handle)}) does not share the context of " \
                 "#{ref_kind} #{state.get_handle_str(ref_handle)} " \
                 "(context #{state.get_handle_str(ref_context.handle)})",
                 key: "evpool-#{ref_kind}-ctx-#{state.get_handle_str(ref_handle)}-#{state.get_handle_str(h)}")
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
  return unless cmd_list.context

  check_events_share_context(state, ctx, event_handles_in_op(op),
                             cmd_list.context, 'immediate command list', cmd_list.handle)
end

# The MemoryAllocation whose range holds 'ptr', or nil.
# allocations is an AllocationMap
def find_allocation(allocations, ptr)
  allocations[ptr]
end

# Records a live allocation.
def track_allocation(state, ctx, allocations, mem)
  allocations.insert(mem)
rescue ZEModel::AllocationMap::OverlapError => e
  state.report(:overlapping_allocation, ctx,
               "allocation #{state.get_handle_str(mem.base)} of #{mem.size} bytes overlaps a " \
               'live allocation of the same context; the allocator should return disjoint ranges',
               key: "overlap-#{state.get_handle_str(mem.base)}-#{mem.size}")
  e.entries.each { |old| allocations.delete(old.base) }
  allocations.insert(mem)
end

# Records a freed allocation.
def track_freed_allocation(allocations, mem)
  allocations.insert(mem)
rescue ZEModel::AllocationMap::OverlapError => e
  e.entries.each { |old| allocations.delete(old.base) }
  allocations.insert(mem)
end

# Drops an allocation from a map. Callers reach this only with an allocation the
# map is holding, so its base is a live key: this is free(ptr).
def untrack_allocation(allocations, mem)
  allocations.delete(mem.base)
end

# Check whether the copy's endpoints have enough space to support the requested size
# Deduped so an append checked at entry is not reported again when it executes.
def check_copy_endpoint_oob(state, ctx, allocations, ptr, size, api, role)
  return unless ptr && ptr != 0 && size

  mem = find_allocation(allocations, ptr)
  return unless mem

  offset = ptr - mem.base
  available = mem.size - offset
  return unless available < size

  state.report(:out_of_bounds_copy, ctx,
               "#{api}: #{role} memory #{state.get_handle_str(ptr)} only has #{available} " \
               "bytes available from this offset but the copy needs #{size} bytes",
               key: "oob-#{api}-#{role}-#{state.get_handle_str(ptr)}-#{size}")
end

# Performs the oob check for copy for both endpoints (src and dst)
def check_oob_copy(state, ctx, params)
  api = params[:api]
  size = params[:size]
  allocations = state.memory_allocations(ctx, params[:ctx_handle])
  return unless allocations

  check_copy_endpoint_oob(state, ctx, allocations, params[:dst], size, api, 'destination')
  check_copy_endpoint_oob(state, ctx, allocations, params[:src], size, api, 'source')
end

# Check if the copy is from/to a nullptr
def check_null_copy_ptr(state, ctx, api, endpoints)
  endpoints.each do |role, ptr|
    next unless ptr.nil? || ptr.zero?

    state.report(:null_copy_pointer, ctx, "#{api}: #{role} pointer is nullptr")
  end
end

# Drops the freed allocations the new one reuses.
def mark_reallocated(freed, handle, size)
  freed.overlapping(handle, size).each { |old| freed.delete(old.base) }
end

# Checks for use-after-free on an address
def check_uaf_endpoint(state, ctx, live, freed, ptr, api, role)
  return unless ptr && ptr != 0 && find_allocation(live, ptr).nil?

  mem = find_allocation(freed, ptr)
  return unless mem

  offset = ptr - mem.base
  where = offset.zero? ? '' : " (offset #{offset} into the freed allocation)"
  state.report(:use_after_free, ctx,
               "#{api}: #{role} memory #{state.get_handle_str(ptr)}#{where} was already " \
               "freed#{" by Process #{mem.freed_by}" if mem.freed_by}; use-after-free",
               key: "uaf-#{api}-#{state.get_handle_str(ptr)}")
end

# Checks for when an API uses a memory that has been freed
def check_use_after_free(state, ctx, params)
  api = params[:api]
  live  = state.memory_allocations(ctx, params[:ctx_handle])
  freed = state.freed_memory_allocations(ctx, params[:ctx_handle])

  return unless live && freed

  check_uaf_endpoint(state, ctx, live, freed, params[:dst], api, 'destination')
  check_uaf_endpoint(state, ctx, live, freed, params[:src], api, 'source')
end

# calls the check_use_after_free only if the wait events have been satisfied
def check_use_after_free_on_append(state, ctx, params, waits)
  return unless state.waits_satisfied?(ctx, waits)

  check_use_after_free(state, ctx, params)
end

# Calls check_oob_copy at append time, so an append that crashes the driver (and
# so emits no _exit) is still checked. Gated on the waits like the uaf check.
def check_oob_copy_on_append(state, ctx, params, waits)
  return unless state.waits_satisfied?(ctx, waits)

  check_oob_copy(state, ctx, params)
end

# Checks for uaf on memory ranges barrier
def check_uaf_ranges_barrier(state, ctx, params)
  api   = params[:api]
  live  = state.memory_allocations(ctx, params[:ctx_handle])
  freed = state.freed_memory_allocations(ctx, params[:ctx_handle])

  return unless live && freed

  params[:ranges].each do |r|
    check_uaf_endpoint(state, ctx, live, freed, r[:base], api, 'range')
  end
end

# returns true if [a, a+asize) and [b, b+bsize) overlap.
def ranges_overlap?(a, asize, b, bsize)
  a < b + bsize && b < a + asize
end

# Checks for whether memory was deleted during execution of a command list
def check_free_in_flight(state, ctx, mem)
  mem_ctx_handle = mem.context.handle
  state.each_inflight_copy_op(ctx) do |unit, op|
    p = op.params
    next unless p[:ctx_handle] == mem_ctx_handle

    hit = [[p[:dst], 'destination'], [p[:src], 'source']].find do |ptr, _role|
      ptr && ptr != 0 && ranges_overlap?(mem.base, mem.size, ptr, p[:size])
    end
    next unless hit

    _ptr, role = hit
    state.report(:free_while_in_flight, ctx,
                 "memory #{state.get_handle_str(mem.base)} is being freed while still in use as " \
                 "the #{role} of an in-flight #{p[:api] || 'copy'} on #{unit.label}; the device " \
                 'may access freed memory',
                 key: "free-inflight-#{state.get_handle_str(mem.base)}-#{unit.label}-#{role}")
  end
end

# Finds the live allocation holding 'ptr' in one Level Zero context, or the one
# containing it, or nil. An unknown context simply holds nothing.
def find_allocation_in_context(state, ctx, ctx_handle, ptr)
  allocations = state.memory_allocations(ctx, ctx_handle)
  allocations && find_allocation(allocations, ptr)
end

# Returns [memory, ctx_handle] for ptr, preferring the passed context (usually command list's context).
def find_known_memory(state, ctx, ptr, prefer_ctx_handle)
  if prefer_ctx_handle
    mem = find_allocation_in_context(state, ctx, prefer_ctx_handle, ptr)
    return [mem, prefer_ctx_handle] if mem
  end
  state.get_process(ctx).contexts.each do |cth, context_obj|
    next if cth == prefer_ctx_handle

    mem = find_allocation(context_obj.memory_allocations, ptr)
    return [mem, cth] if mem
  end
  [nil, nil]
end

# Checks that one copy/fill endpoint was allocated on the command list's
# context. Untracked pointers are skipped; deduped per (list, endpoint, ptr).
def check_ptr_endpoint_list_context(state, ctx, list_ctx_handle, list_handle, ptr, api, role)
  # unknown command list context -> skip
  return unless ptr && ptr != 0 && list_ctx_handle

  mem, found_ctx = find_known_memory(state, ctx, ptr, list_ctx_handle)
  # unknown pointer -> skip (no false alarm)
  return unless mem && found_ctx != list_ctx_handle

  mem_ctx_str = state.get_handle_str(mem.context.handle)
  state.report(:memory_list_context_mismatch, ctx,
               "#{api}: #{role} memory #{state.get_handle_str(ptr)} was allocated on context #{mem_ctx_str} " \
               "but command list #{state.get_handle_str(list_handle)} is on context #{state.get_handle_str(list_ctx_handle)}; " \
               'the command list and copied memory must share a context',
               key: "ptr-list-ctx-#{state.get_handle_str(list_handle)}-#{role}-#{state.get_handle_str(ptr)}")
end

# Checks a copy/fill's endpoints against the command list's context. Runs at
# entry: a cross-context copy can be rejected inside the append.
def check_copy_ptr_list_context(state, ctx, api, list_handle, endpoints)
  list_ctx_handle = cmd_list_ctx_handle(state, ctx, list_handle)
  endpoints.each do |role, ptr|
    check_ptr_endpoint_list_context(state, ctx, list_ctx_handle, list_handle, ptr, api, role)
  end
end

# Checks for an event signaled while already signaled with no reset between:
# reuse-no-reset if the host observed the prior signal, double-signal if not.
def check_event_signal_reuse(state, ctx, handle, who)
  ev = state.event_by_handle(ctx, handle)
  return unless ev&.signaled

  if ev.observed
    state.report(:event_reuse_without_reset, ctx,
                 "event #{state.get_handle_str(handle)} was reused as a signal target by #{who} " \
                 'without calling zeEventHostReset/zeCommandListAppendEventReset after it was ' \
                 "signaled#{" by #{ev.signaled_by}" if ev.signaled_by}",
                 key: "event-reuse-#{state.get_handle_str(handle)}-#{who}")
  else
    state.report(:event_concurrent_signal, ctx,
                 "event #{state.get_handle_str(handle)} was signaled by #{who} before being reset " \
                 "or consumed#{" (already signaled by #{ev.signaled_by})" if ev.signaled_by}; " \
                 'concurrent signals of the same event are undefined',
                 key: "event-double-signal-#{state.get_handle_str(handle)}-#{who}")
  end
end

# Reports wait-events never signaled by end of trace, i.e. a deferred op that
# could never complete.
def report_unsignaled_waits(state, ctx, waits)
  waits.each do |h|
    ev = state.event_by_handle(ctx, h)
    next unless ev && !ev.signaled

    state.report(:unsignaled_wait_event, ctx,
                 "event #{state.get_handle_str(h)} was never signaled; a deferred command list " \
                 'operation could not complete (possible deadlock or missing signal)',
                 key: "unsignaled-#{state.get_handle_str(h)}")
  end
end

# Checks for a circular event dependency across the units still stuck at end of
# trace, reporting the first cycle found since cycles overlap and share units.
def check_circular_deadlock(state, units)
  stuck = units.reject { |u| u.blocked_on.empty? }
  return if stuck.empty?

  # event handle -> units that may still signal it
  signalers = Hash.new { |h, k| h[k] = [] }
  stuck.each { |u| u.pending_signals.each { |ev| signalers[ev] << u } }

  # adjacency: U -> V if U waits on an event V still owes
  graph = RGL::DirectedAdjacencyGraph.new
  stuck.each do |u|
    graph.add_vertex(u)
    u.blocked_on.each do |ev|
      signalers[ev].each { |v| graph.add_edge(u, v) unless v.equal?(u) }
    end
  end

  # A back edge closes a cycle: its target is still on the current branch, which
  # path tracks. Stop at the first cycle since cycles overlap and share units.
  path = []
  found = nil
  visitor = RGL::DFSVisitor.new(graph)
  visitor.set_examine_vertex_event_handler { |u| path.push(u) }
  visitor.set_finish_vertex_event_handler { |_u| path.pop }
  visitor.set_back_edge_event_handler { |_u, v| found ||= path[path.index(v)..] }
  graph.depth_first_search(visitor) { |_u| }
  report_deadlock_cycle(state, found) if found
end

# Labels one node of a deadlock cycle as "<list>::<blocking API>".
def deadlock_node_label(state, unit)
  op = unit.current_op
  # fall back to the op kind so the label is never blank
  api = op ? (op.api || op.kind.to_s) : 'unknown'
  waits = unit.blocked_on.map { |h| state.get_handle_str(h) }.join(', ')
  "#{unit.label}::#{api} (waiting on event #{waits})"
end

def report_deadlock_cycle(state, cycle)
  ctx = cycle.first.context
  desc = cycle.map { |u| deadlock_node_label(state, u) }.join(' -> ')
  # close the loop for readability
  desc << " -> #{deadlock_node_label(state, cycle.first)}"
  state.report_proc(:circular_event_dependency, ctx,
                    "circular event dependency among command list operations; none can start: #{desc}")
end

# Checks for an in-order list parked on an event only a later op in the same
# list signals. The cross-list detector misses this since it drops self-edges.
def check_in_order_self_deadlock(state, units)
  units.each do |unit|
    next unless unit.in_order && !unit.blocked_on.empty?

    self_waits = unit.blocked_on & unit.pending_signals
    self_waits.each do |ev|
      # the later op in this same list that would signal ev (but never runs)
      later = unit.ops[(unit.cursor + 1)..]&.find { |o| o.signal == ev }
      report_in_order_self_deadlock(state, unit, ev, later)
    end
  end
end

# Reports one intra-list self-deadlock as <waiting op> -> <signaling op>.
def report_in_order_self_deadlock(state, unit, ev, signaling_op)
  waiting = unit.current_op
  waiting_api = waiting ? (waiting.api || waiting.kind.to_s) : 'unknown'
  signaling_api = signaling_op ? (signaling_op.api || signaling_op.kind.to_s) : 'unknown'
  ev_str = state.get_handle_str(ev)
  desc = "#{unit.label}::#{waiting_api} (waits on event #{ev_str}) -> " \
         "#{unit.label}::#{signaling_api} (signals event #{ev_str} later in the same in-order list)"
  state.report_proc(:in_order_self_deadlock, unit.context,
                    'in-order command list cannot complete; an earlier command waits on an event a later ' \
                    "command in the same list signals: #{desc}",
                    key: "self-deadlock-#{unit.label}-#{ev_str}")
end

# Checks a descriptor's stype. Current drivers ignore a wrong one, but it is a
# latent bug a future driver may reject. Reported once per expected stype.
def check_struct_stype_misuse(state, ctx, _payload, expected_stype, observed_stype)
  return if expected_stype == observed_stype

  state.report(:descriptor_stype_mismatch, ctx,
               "expected stype #{expected_stype} but #{observed_stype} was observed",
               key: expected_stype.to_s)
end
