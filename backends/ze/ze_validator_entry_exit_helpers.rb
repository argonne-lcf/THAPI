# frozen_string_literal: true

require 'ze_validator_zemodel'
require 'ze_library'
require 'rgl/adjacency'
require 'rgl/traversal'

# Checks for oob index. A command queue is created with an (ordinal, index)
# pair -- which engine group, and which queue within that group.
def check_valid_index_for_ordinal(state, ctx, queue_handle, ordinal, index)
  return unless state.device_properties

  command_queue_prop = state.device_properties['devices'][0]['command_queue_groups']
  command_queue_prop.each do |prop|
    # find matching ordinal, and check whether the index is oob
    next unless prop['ordinal'] == ordinal && (index >= prop['numQueues'] || index.negative?)

    state.print_usage_error(ctx, "command queue (#{state.get_handle_str(queue_handle)}) with ordinal = #{ordinal} was created " \
                                 "with index = #{index}. Index value should be: 0<= index < #{prop['numQueues']}")
  end
end

# Checking whether the application ever called zeDeviceGetCommandQueueGroupProperties
# before calling command queue/list create. Not calling it implies hardcoded ordinals
def check_group_property_queued(state, ctx, _payload, device)
  return unless !device.cmd_queue_group_properties_queried && state.print_tracker['check_group_property'].zero?

  state.print_tracker['check_group_property'] = 1
  state.print_usage_error(ctx,
                          "command queue group wasn't queried. Hardcoded group properties may break the code on different devices")
end

# returns the copy ordinals if retrieved from the ze_device_property.json
def copy_only_ordinals(state)
  return [1, 2] unless state.device_properties

  state.device_properties['devices'][0]['command_queue_groups']
       .filter_map { |prop| prop['ordinal'] if prop['type'] == 'copy' }
end

# checks whether a command list attached to a copy-only engine receives a kernel
def check_valid_ordinal(state, ctx, _payload, cqg_ordinal)
  copy_only_ords = copy_only_ordinals(state)
  return unless copy_only_ords.include?(cqg_ordinal) && state.print_tracker['check_valid_ordinal'].zero?

  state.print_tracker['check_valid_ordinal'] = 1
  kernels = state.find_objects(ctx, 'kernel')
  kernel_handle = state.find_param(ctx, 'hKernel')
  kernel_name = 'UNKNOWN' # kernel name wasn't passed, so mark it as unknown
  command_list_handle = state.find_param(ctx, 'hCommandList')
  kernel_name = kernels[kernel_handle].name if kernels[kernel_handle]
  state.print_usage_error(ctx,
                          "Launching kernel (#{kernel_name}) to a command list with Copy Ordinal: #{state.get_handle_str(command_list_handle)}")
end

# list of compute launches
COMPUTE_LAUNCH_APIS = %w[zeCommandListAppendLaunchKernel
                         zeCommandListAppendLaunchCooperativeKernel].freeze

def command_list_has_kernel_launch?(cmd_list)
  cmd_list&.ops&.any? { |op| op.kind == :launch && COMPUTE_LAUNCH_APIS.include?(op.api) }
end

# Checks whether a command list that has a compute kernel gets submitted to a command queue that is attached to a copy only engine.
def check_copy_only_queue_submission(state, ctx, queue, cmd_list)
  return unless queue&.desc && command_list_has_kernel_launch?(cmd_list)

  queue_ordinal = queue.desc[:ordinal]
  return unless copy_only_ordinals(state).include?(queue_ordinal)

  key = "copyq-submit-#{state.get_handle_str(queue.handle)}-#{state.get_handle_str(cmd_list.handle)}"
  return unless state.print_tracker[key].zero?

  state.print_tracker[key] = 1
  state.print_usage_error(ctx, "command list #{state.get_handle_str(cmd_list.handle)} contains a compute kernel " \
                               "launch but was submitted to command queue #{state.get_handle_str(queue.handle)} " \
                               "with copy-only ordinal #{queue_ordinal}")
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

  key = "kernel-list-ctx-#{state.get_handle_str(cmd_list.handle)}-#{state.get_handle_str(kernel.handle)}"
  return unless state.print_tracker[key].zero?

  state.print_tracker[key] = 1
  state.print_usage_error(ctx,
                          "kernel #{state.get_handle_str(kernel.handle)} (from module " \
                          "#{state.get_handle_str(mod.handle)} on context #{state.get_handle_str(mod.context.handle)}) " \
                          "does not share the context of command list #{state.get_handle_str(cmd_list.handle)} " \
                          "(context #{state.get_handle_str(cmd_list.context.handle)})")
end

# Checks if the kernel was created
def check_kernel_created(state, ctx, payload)
  kernels = state.find_objects(ctx, 'kernel')
  kernel_handle = payload['hKernel']
  return if kernels[kernel_handle]

  state.print_usage_error(ctx,
                          "kernel: #{state.get_handle_str(kernel_handle)} wasn't created. Consider calling zeKernelCreate")
end

# Checks for using fence without reset
def check_fence_misuse(state, ctx, payload)
  fence_handle = payload['hFence']
  fence = get_fence(state, ctx, fence_handle)
  return unless fence && (fence.status == fence.signaled || fence.status == fence.in_use)

  state.print_usage_error(ctx, "Used fence: #{state.get_handle_str(fence_handle)} twice without resetting it")
end

# Check whether the queue handed to ExecuteCommandLists was never created (or was already destroyed).
def check_valid_command_queue(state, ctx, _payload, cmd_queues, cmd_queue_ptr)
  cmd_queue = cmd_queues[cmd_queue_ptr]
  return if cmd_queue

  state.print_usage_error(ctx,
                          "Invalid commandQueue (#{state.get_handle_str(cmd_queue_ptr)}) was handed to zeCommandQueueExecuteCommandLists")
end

# Checks for submitting nothing, submitting a handle that was never created, or
# submitting an immediate list, which carries its own queue.
def check_valid_command_lists(state, ctx, payload)
  command_lists = payload['phCommandLists_vals']
  known_command_lists = state.find_objects(ctx, 'command_list')
  if command_lists.nil? || command_lists.empty?
    state.print_usage_error(ctx, 'No valid commandlist was chosen at zeCommandQueueExecuteCommandLists')
  end

  command_lists.each do |command_list_handle|
    if !known_command_lists[command_list_handle]
      state.print_usage_error(ctx,
                              "Invalid commandlist (#{command_list_handle}) was handed to zeCommandQueueExecuteCommandLists")
    elsif known_command_lists[command_list_handle]&.immediate
      state.print_usage_error(ctx,
                              "Immediate Command List was chosen for the Command Queue: #{state.get_handle_str(command_queue_handle)}")
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
  handles.reject { |h| h.nil? || h.zero? }
end

# Record one op onto a command list
def record_op(state, ctx, cmd_list_handle, op)
  cmd_list = state.find_objects(ctx, 'command_list')[cmd_list_handle]
  return unless cmd_list

  if cmd_list.immediate
    check_event_pool_immediate_list_context_match(state, ctx, cmd_list, op)
    state.enqueue_immediate_op(ctx, op, cmd_list_handle)
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
      state.print_usage_error(ctx,
                              "commandlist: #{state.get_handle_str(command_list_handle)} wasn't closed before executing on #{state.get_handle_str(command_queue_handle)}")
    elsif cmd_list.status == ZEModel::CommandList.class_variable_get(:@@DESTROYED)
      state.print_usage_error(ctx,
                              "commandlist: #{state.get_handle_str(command_list_handle)} was already destroyed #{state.get_handle_str(command_queue_handle)}")
    end
  end
end

# check if the command list reset is valid or not.
# Invalid calls: reset on destroyed lists, reset on immeidate lists, and reset on command lists that are already exeucting.
def check_command_list_reset(state, ctx, payload)
  handle = payload['hCommandList']
  cmd_list = state.find_objects(ctx, 'command_list')[handle]

  if cmd_list.status == ZEModel::CommandList.class_variable_get(:@@DESTROYED)
    key = "clreset-destroyed-#{state.get_handle_str(handle)}"
    if state.print_tracker[key].zero?
      state.print_tracker[key] = 1
      state.print_usage_error(ctx,
                              "command list #{state.get_handle_str(handle)} was already destroyed before zeCommandListReset")
    end
    return
  end

  if cmd_list.immediate
    key = "clreset-immediate-#{state.get_handle_str(handle)}"
    if state.print_tracker[key].zero?
      state.print_tracker[key] = 1
      state.print_usage_error(ctx, "zeCommandListReset called on immediate command list #{state.get_handle_str(handle)}; " \
                                   'immediate command lists cannot be reset')
    end
  end

  return unless state.command_list_in_flight?(ctx, handle)

  key = "clreset-inflight-#{state.get_handle_str(handle)}"
  return unless state.print_tracker[key].zero?

  state.print_tracker[key] = 1
  state.print_usage_error(ctx, "command list #{state.get_handle_str(handle)} is being reset while a prior " \
                               'zeCommandQueueExecuteCommandLists submission is still in-flight; the device may ' \
                               'still be executing it (undefined behavior)')
end

# checks whether zeKernelCreate was given a null module handle.
def check_valid_module(state, ctx, _payload)
  module_handle = state.find_param(ctx, 'hModule')
  return unless !module_handle || module_handle.zero?

  state.print_usage_error(ctx, 'Improper hModule was handed')
end

# Checks if the fence's queue and the command list is on the same context.
def check_list_and_fence_have_matching_context(state, ctx, _payload, cmd_list, fence)
  return unless cmd_list&.context && fence&.command_queue&.context && cmd_list.context != fence.command_queue.context

  list_handle = cmd_list ? state.get_handle_str(cmd_list.handle) : 'nullptr'
  fence_handle = fence
  state.print_usage_error(ctx, "Mismatching context between command list #{list_handle} and fence #{fence_handle}")
end

# Checks for context between queue and the fence.
# Stronger than a context match, as it checks for the matching of the queue.
def check_fence_and_queue_compatibility(state, ctx, _payload, cmd_queue, fence)
  return unless fence && cmd_queue && cmd_queue != fence.command_queue

  queue_handle = cmd_queue ? state.get_handle_str(cmd_queue.handle) : 'nullptr'
  fence_handle = fence
  state.print_usage_error(ctx, "Associated command queue (#{state.get_handle_str(fence.command_queue.handle)}) of fence #{fence_handle} " \
                               "is different from the one that was provided #{queue_handle}")
end

# Check the context between the queue and the list
def check_list_and_queue_have_matching_context(state, ctx, _payload, cmd_list, cmd_queue)
  return if cmd_queue && cmd_list && cmd_list.context == cmd_queue.context

  queue_handle = cmd_queue ? state.get_handle_str(cmd_queue.handle) : 'nullptr'
  list_handle = cmd_list ? state.get_handle_str(cmd_list.handle) : 'nullptr'
  state.print_usage_error(ctx,
                          "Mismatching context between command queue #{queue_handle} and command list #{list_handle}")
end

# List of operations to collect the events from
EVENT_OP_KINDS = %i[copy launch signal wait reset].freeze

# retrieves the events in a given op
def event_handles_in_op(op)
  handles = []
  if EVENT_OP_KINDS.include?(op.kind)
    handles << op.signal if op.signal
    handles.concat(op.waits) if op.waits
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

    key = "evpool-#{ref_kind}-ctx-#{state.get_handle_str(ref_handle)}-#{state.get_handle_str(h)}"
    next unless state.print_tracker[key].zero?

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
  return unless cmd_list&.context

  check_events_share_context(state, ctx, event_handles_in_op(op),
                             cmd_list.context, 'immediate command list', cmd_list.handle)
end

# Find the allocation based at exactly ptr, else the one containing it.
# O(log n) per look up.
def find_allocation(allocations, ptr)
  mem = allocations.bsearch { |x| x <=> ptr }
  return mem if mem

  idx = allocations.bsearch_index { |m| m.base > ptr }
  candidate = if idx
                idx.zero? ? nil : allocations[idx - 1]
              else
                allocations.last
              end
  candidate if candidate && ptr < candidate.base + candidate.size
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

  key = "oob-#{api}-#{role}-#{state.get_handle_str(ptr)}-#{size}"
  return unless state.print_tracker[key].zero?

  state.print_tracker[key] = 1
  state.print_usage_error(ctx, "#{api}: #{role} memory #{state.get_handle_str(ptr)} only has #{available} " \
                               "bytes available from this offset but the copy needs #{size} bytes")
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
    state.print_usage_error(ctx, "#{api}: #{role} pointer is nullptr") if ptr.nil? || ptr.zero?
  end
end

# Deletes the address with a new allocation
# An address might be reused after a free. In this case, we need to update the validator's state as well.
def mark_reallocated(state, ctx, ctx_handle, handle, size)
  freed = state.freed_memory_allocations(ctx, ctx_handle)
  return if freed.empty?

  freed.delete_if { |m| ranges_overlap?(m.base, m.size, handle, size) }
end

# Checks for use-after-free on an address
def check_uaf_endpoint(state, ctx, live, freed, ptr, api, role)
  return unless ptr && ptr != 0 && find_allocation(live, ptr).nil?

  mem = find_allocation(freed, ptr)
  return unless mem

  key = "uaf-#{api}-#{state.get_handle_str(ptr)}"
  return unless state.print_tracker[key].zero?

  state.print_tracker[key] = 1
  offset = ptr - mem.base
  where = offset.zero? ? '' : " (offset #{offset} into the freed allocation)"
  state.print_memory_error(ctx, "#{api}: #{role} memory #{state.get_handle_str(ptr)}#{where} was already " \
                                "freed#{" by #{mem.freed_by}" if mem.freed_by}; use-after-free")
end

# Checks for when an API uses a memory that has been freed
def check_use_after_free(state, ctx, params)
  api = params[:api] || 'zeCommandListAppendMemoryCopy'
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

# Calls check_oob_copy at append time, so an append that crashes the driver (and
# so emits no _exit) is still checked. Gated on the waits like the uaf check.
def check_oob_copy_on_append(state, ctx, params, waits)
  return unless state.waits_satisfied?(ctx, waits)

  check_oob_copy(state, ctx, params)
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

# Checks for whether memory was deleted during execution of a command list
def check_free_in_flight(state, ctx, mem)
  return unless mem

  mem_ctx_handle = mem.context&.handle
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
                                  'may access freed memory')
  end
end

# Finds the memory object in the validator that matches the ptr, or the object that contains the ptr
def find_memory_in_submap(submap, ptr)
  find_allocation(submap, ptr)
end

# Returns [memory, ctx_handle] for ptr, preferring the passed context (usually command list's context).
def find_known_memory(state, ctx, ptr, prefer_ctx_handle)
  return [nil, nil] unless ptr && ptr != 0

  all_maps = state.get_process(ctx).memory_allocations
  if prefer_ctx_handle && all_maps.key?(prefer_ctx_handle)
    mem = find_memory_in_submap(all_maps[prefer_ctx_handle], ptr)
    return [mem, prefer_ctx_handle] if mem
  end
  all_maps.each do |cth, submap|
    if cth != prefer_ctx_handle
      mem = find_memory_in_submap(submap, ptr)
      return [mem, cth] if mem
    end
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

  key = "ptr-list-ctx-#{state.get_handle_str(list_handle)}-#{role}-#{state.get_handle_str(ptr)}"
  return unless state.print_tracker[key].zero?

  state.print_tracker[key] = 1
  mem_ctx_str = mem.context ? state.get_handle_str(mem.context.handle) : state.get_handle_str(found_ctx)
  state.print_usage_error(ctx,
                          "#{api}: #{role} memory #{state.get_handle_str(ptr)} was allocated on context #{mem_ctx_str} " \
                          "but command list #{state.get_handle_str(list_handle)} is on context #{state.get_handle_str(list_ctx_handle)}; " \
                          'the command list and copied memory must share a context')
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
    state.print_usage_error(ctx, "event #{state.get_handle_str(handle)} was reused as a signal target by #{who} " \
                                 'without calling zeEventHostReset/zeCommandListAppendEventReset after it was ' \
                                 "signaled#{" by #{ev.signaled_by}" if ev.signaled_by}")
  else
    state.print_usage_error(ctx, "event #{state.get_handle_str(handle)} was signaled by #{who} before being reset " \
                                 "or consumed#{" (already signaled by #{ev.signaled_by})" if ev.signaled_by}; " \
                                 'concurrent signals of the same event are undefined')
  end
end

# Reports wait-events never signaled by end of trace, i.e. a deferred op that
# could never complete.
def report_unsignaled_waits(state, ctx, waits)
  (waits || []).each do |h|
    ev = state.event_by_handle(ctx, h)
    next unless ev && !ev.signaled

    state.print_usage_error(ctx, "event #{state.get_handle_str(h)} was never signaled; a deferred command list " \
                                 'operation could not complete (possible deadlock or missing signal)')
  end
end

# Checks for a circular event dependency across the units still stuck at end of
# trace, reporting the first cycle found since cycles overlap and share units.
def check_circular_deadlock(state, units)
  stuck = units.select { |u| u.blocked_on && !u.blocked_on.empty? }
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
  state.print_deadlock_error(ctx, "circular event dependency among command list operations; none can start: #{desc}")
end

# Checks for an in-order list parked on an event only a later op in the same
# list signals. The cross-list detector misses this since it drops self-edges.
def check_in_order_self_deadlock(state, units)
  units.each do |unit|
    next unless unit.in_order && unit.blocked_on && !unit.blocked_on.empty?

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
  state.print_deadlock_error(unit.context,
                             'in-order command list cannot complete; an earlier command waits on an event a later ' \
                             "command in the same list signals: #{desc}")
end

# Checks a descriptor's stype. Current drivers ignore a wrong one, but it is a
# latent bug a future driver may reject. Reported once per expected stype.
def check_struct_stype_misuse(state, ctx, _payload, expected_stype, observed_stype)
  return unless expected_stype != observed_stype && state.print_tracker[expected_stype].zero?

  state.print_tracker[expected_stype] = 1
  state.print_usage_error(ctx, "\nExpected stype of #{expected_stype}\nbut #{observed_stype} was observed.")
end
