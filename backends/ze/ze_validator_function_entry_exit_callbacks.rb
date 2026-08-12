require 'ze_validator_entry_exit_helpers'
require 'ze_validator_zemodel'
require 'ze_library'



$upon_entry = {} #called to modify program state on entry
$on_successful_exit = {} #called upon seeing exit functions with a successful return code
$on_erroneous_exit = {} #called upon seeing exit functions with a non-successful return code

# --- Memory residency -------------------------------------------------------
# Device allocations must be resident in GPU memory to be usable. These two APIs
# move them in and out explicitly; the model just tracks the flag.

$on_successful_exit["zeContextEvictMemory"] = lambda{|state, ctx, defi|
  mem_addr = state.find_param(ctx,"ptr")
  # CHANGED: scope the lookup to the evicting context (hContext is a param of
  # this API), so an address live in another context is not touched by mistake.
  memory_allocations = state.memory_allocations(ctx, state.find_param(ctx, 'hContext'))
  if memory_allocations[mem_addr]
    mem = memory_allocations[mem_addr]
    mem.resident = false
  end
}

#Check whether that memory is accessible by device?
$on_successful_exit["zeContextMakeMemoryResident"] = lambda {|state, ctx, defi|
  mem_addr = state.find_param(ctx,"ptr")
  # CHANGED: scope the lookup to this context (see zeContextEvictMemory).
  memory_allocations = state.memory_allocations(ctx, state.find_param(ctx, 'hContext'))
  if memory_allocations[mem_addr]
    mem = memory_allocations[mem_addr]
    mem.resident = true #Does the driver automatically evict memory if the virtual mem exceeds the physical mem?
  end
}


# --- Device introspection ---------------------------------------------------
# These two record only that the application ASKED. The portability checks care
# about the question being asked at all: code that never queries the device but
# still passes ordinals and group indices is working from hardcoded assumptions
# that will not survive a hardware change.

$on_successful_exit["zeDeviceGetProperties"] = lambda{|state, ctx, defi|
  device_ptr = state.find_param(ctx,'hDevice')
  devices = state.find_objects(ctx, 'device')
  devices[device_ptr].property_fetched = true
}

$on_successful_exit["zeDeviceGetCommandQueueGroupProperties"] = lambda{|state, ctx, defi|
  device_ptr = state.find_param(ctx,'hDevice')
  devices = state.find_objects(ctx, 'device')
  devices[device_ptr].cmd_queue_group_properties_queried = true
}

# --- Kernel launches --------------------------------------------------------
# Note the split that recurs for every append API below: VALIDATION at entry
# (because the call may crash), RECORDING at exit (because only a successful
# append will ever execute).

# CHANGED: validation moved to ENTRY. Launching a kernel on a copy-only ordinal
# can crash the process, so the append may emit no _exit event -- checking at
# exit (the old code, which also had a `stat.find_param` typo that would NameError)
# would miss it. Entry callbacks may read input params directly from defi.
$upon_entry["zeCommandListAppendLaunchKernel"] = lambda { |state, ctx, defi|
  #Retrieve the compute ordinal from the command list
  command_lists = state.find_objects(ctx, 'command_list')
  cmd_list = command_lists[defi['hCommandList']]
  cqg_ordinal = 0
  #a normal list carries the ordinal in desc; an immediate list in altdesc
  if cmd_list && cmd_list.desc
    cqg_ordinal = cmd_list.desc[:commandQueueGroupOrdinal]
  elsif cmd_list && cmd_list.altdesc
    cqg_ordinal =  cmd_list.altdesc[:ordinal]
  end
  #both checks must run even if the launch later aborts
  check_valid_ordinal(state, ctx, defi, cqg_ordinal)
  check_kernel_created(state, ctx, defi)
  #the kernel's module must be on the same context as the command list
  check_kernel_list_context_match(state, ctx, defi)
}

# CHANGED: op-recording (for deferred execution ordering) stays at EXIT, since
# only a launch that successfully appended actually executes later on the queue.
$on_successful_exit["zeCommandListAppendLaunchKernel"] = lambda { |state, ctx, defi|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:launch,
              signal: state.find_param(ctx, 'hSignalEvent'),
              waits: wait_event_handles(state, ctx),
              api: 'zeCommandListAppendLaunchKernel'))
}

# ADDED: zeCommandListReset returns a command list to its initial, empty,
# appendable state so it can be reused without destroy+recreate. Misuse checks run
# at ENTRY (a reset can be rejected/crash without emitting an _exit -- e.g. on an
# immediate list) reading the input handle from defi.
$upon_entry["zeCommandListReset"] = lambda { |state, ctx, defi|
  check_command_list_reset(state, ctx, defi)
}

# ADDED: on success the list is empty and open again. Clear the recorded ops so a
# later close/execute replays only ops appended after the reset (in-flight
# executions from before are unaffected -- enqueue_deferred_execution snapshotted
# a dup of the ops at submit time), and return the status to INITIALIZED so the
# closed-before-execute check applies to the reused list.
$on_successful_exit["zeCommandListReset"] = lambda { |state, ctx, defi|
  command_lists = state.find_objects(ctx, 'command_list')
  cmd_list = command_lists[state.find_param(ctx, 'hCommandList')]
  return unless cmd_list
  cmd_list.ops.clear
  cmd_list.status = ZEModel::CommandList.class_variable_get(:@@INITIALIZED)
}

#when command queue is executed, the associated fence's status is set to IN_USE
# Closing finalizes the list: no more appends, and it may now be submitted.
# check_command_list_closed later verifies this happened before any submission.
$on_successful_exit["zeCommandListClose"] = lambda { |state, ctx, defi|
  command_lists = state.find_objects(ctx, 'command_list')
  command_list_handle = state.find_param(ctx,"hCommandList")
  cmd_list = command_lists[command_list_handle]
  cmd_list.status = ZEModel::CommandList.class_variable_get(:@@CLOSED)
}

# CHANGED: validation at ENTRY (a cooperative launch can likewise abort without
# an exit). Also fixes the `stte` typo (undefined -> NameError) and nil-guards
# the command list.
$upon_entry["zeCommandListAppendLaunchCooperativeKernel"] = lambda { |state, ctx, defi|
  command_lists = state.find_objects(ctx, 'command_list')
  cmd_list = command_lists[defi['hCommandList']]
  check_group_property_queued(state,ctx,defi,cmd_list.device) if cmd_list
  #the kernel's module must be on the same context as the command list
  check_kernel_list_context_match(state, ctx, defi)
}

# CHANGED: recording at EXIT.
$on_successful_exit["zeCommandListAppendLaunchCooperativeKernel"] = lambda { |state, ctx, defi|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:launch,
              signal: state.find_param(ctx, 'hSignalEvent'),
              waits: wait_event_handles(state, ctx),
              api: 'zeCommandListAppendLaunchCooperativeKernel'))
}

# ---- ADDED: copy / event ops recorded onto the command list for deferred replay
# Each records the op in list order. For copies the out-of-bounds check is
# deferred until the op's wait-events are satisfied (see the scheduler in
# ze_validator_state_object.rb), so it runs against the memory state at the point
# the copy actually executes rather than at append or execute time. Exit
# callbacks read input params via find_param (defi holds only exit output).

# ADDED: use-after-free check at ENTRY. A copy/fill whose pointer was already
# freed can crash the driver inside the append, so no _exit event is emitted and
# an exit-only check would miss it (the trace shows only the entry, then a crash).
# Entry callbacks read input params directly from defi. This runs the same UAF
# check against the freed registry before the (possibly fatal) append.
$upon_entry['zeCommandListAppendMemoryCopy'] = lambda { |state, ctx, defi|
  check_null_copy_ptr(state, ctx, 'zeCommandListAppendMemoryCopy',
    { 'destination' => defi['dstptr'], 'source' => defi['srcptr'] })
  check_use_after_free_on_append(state, ctx,
    { api: 'zeCommandListAppendMemoryCopy',
      ctx_handle: cmd_list_ctx_handle(state, ctx, defi['hCommandList']),
      dst: defi['dstptr'], src: defi['srcptr'], size: defi['size'] },
    wait_event_handles(state, ctx))
  # known memory endpoints must be allocated on the command list's context
  check_copy_ptr_list_context(state, ctx, 'zeCommandListAppendMemoryCopy', defi['hCommandList'],
    { 'destination' => defi['dstptr'], 'source' => defi['srcptr'] })
}

$upon_entry['zeCommandListAppendMemoryFill'] = lambda { |state, ctx, defi|
  check_null_copy_ptr(state, ctx, 'zeCommandListAppendMemoryFill',
    { 'destination' => defi['ptr'] })
  check_use_after_free_on_append(state, ctx,
    { api: 'zeCommandListAppendMemoryFill',
      ctx_handle: cmd_list_ctx_handle(state, ctx, defi['hCommandList']),
      dst: defi['ptr'], src: nil, size: defi['size'] },
    wait_event_handles(state, ctx))
  # known memory endpoint must be allocated on the command list's context
  check_copy_ptr_list_context(state, ctx, 'zeCommandListAppendMemoryFill', defi['hCommandList'],
    { 'destination' => defi['ptr'] })
}

$on_successful_exit['zeCommandListAppendMemoryCopy'] = lambda { |state, ctx, defi|
  record_copy_op(state, ctx, 'zeCommandListAppendMemoryCopy', 'dstptr', 'srcptr')
}

$on_successful_exit['zeCommandListAppendMemoryFill'] = lambda { |state, ctx, defi|
  #a fill only touches the destination; model it as a copy with no source
  record_copy_op(state, ctx, 'zeCommandListAppendMemoryFill', 'ptr', nil)
}

# ADDED: a copy append that FAILED is never recorded and never executed, so the
# deferred check would never see it. But a copy whose size exceeds its
# destination/source allocation is out-of-bounds regardless of the error code --
# and a failing append is exactly where the driver rejects such a copy (e.g.
# ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY). Check it now, against the current memory
# state (the pointers are already allocated at append time).
$on_erroneous_exit['zeCommandListAppendMemoryCopy'] = lambda { |state, ctx, defi|
  params = { api: 'zeCommandListAppendMemoryCopy',
             ctx_handle: cmd_list_ctx_handle(state, ctx, state.find_param(ctx, 'hCommandList')),
             dst: state.find_param(ctx, 'dstptr'),
             src: state.find_param(ctx, 'srcptr'),
             size: state.find_param(ctx, 'size') }
  check_oob_copy(state, ctx, params)
  check_use_after_free(state, ctx, params)
}

$on_erroneous_exit['zeCommandListAppendMemoryFill'] = lambda { |state, ctx, defi|
  params = { api: 'zeCommandListAppendMemoryFill',
             ctx_handle: cmd_list_ctx_handle(state, ctx, state.find_param(ctx, 'hCommandList')),
             dst: state.find_param(ctx, 'ptr'),
             src: nil,
             size: state.find_param(ctx, 'size') }
  check_oob_copy(state, ctx, params)
  check_use_after_free(state, ctx, params)
}

$on_successful_exit['zeCommandListAppendMemoryCopyRegion'] = lambda { |state, ctx, defi|
  #region copies carry 2D/3D extents, so `size` is not a flat byte count; we only
  #record ordering + event effects and skip the flat OOB comparison for now
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:launch,
              signal: state.find_param(ctx, 'hSignalEvent'),
              waits: wait_event_handles(state, ctx),
              api: 'zeCommandListAppendMemoryCopyRegion'))
}

#A device-side signal: the event is signaled when this op executes (after waits).
$on_successful_exit['zeCommandListAppendSignalEvent'] = lambda { |state, ctx, defi|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:signal, signal: state.find_param(ctx, 'hEvent'),
              api: 'zeCommandListAppendSignalEvent'))
}

#A device-side wait: this op blocks the list until phEvents are signaled.
$on_successful_exit['zeCommandListAppendWaitOnEvents'] = lambda { |state, ctx, defi|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:wait, waits: wait_event_handles(state, ctx),
              api: 'zeCommandListAppendWaitOnEvents'))
}

#A device-side reset: returns the event to unsignaled when this op executes.
$on_successful_exit['zeCommandListAppendEventReset'] = lambda { |state, ctx, defi|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:reset, params: { reset_handle: state.find_param(ctx, 'hEvent') }))
}

#A barrier waits on its events and signals its completion event.
$on_successful_exit['zeCommandListAppendBarrier'] = lambda { |state, ctx, defi|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:barrier,
              signal: state.find_param(ctx, 'hSignalEvent'),
              waits: wait_event_handles(state, ctx),
              api: 'zeCommandListAppendBarrier'))
}

# ADDED: a memory-ranges barrier has the same event semantics as a plain barrier
# (waits on its events, signals its completion event), so record it in the op
# stream for the deferred scheduler and deadlock detection. It additionally names
# memory ranges whose coherency it guarantees; those ranges are snapshotted and
# validated against the allocation model when the barrier executes (see
# record_ranges_barrier_op / check_uaf_ranges_barrier).
$on_successful_exit['zeCommandListAppendMemoryRangesBarrier'] = lambda { |state, ctx, defi|
  record_ranges_barrier_op(state, ctx)
}

# ADDED: host-side event operations, effective immediately (in trace order).
#Signaling an already-signaled event without a reset is the same misuse we catch
#on device ops.
$on_successful_exit['zeEventHostSignal'] = lambda { |state, ctx, defi|
  handle = state.find_param(ctx, 'hEvent')
  check_event_signal_reuse(state, ctx, handle, 'zeEventHostSignal')
  state.signal_event(ctx, handle, 'zeEventHostSignal')
}

$on_successful_exit['zeEventHostReset'] = lambda { |state, ctx, defi|
  state.reset_event(ctx, state.find_param(ctx, 'hEvent'))
}

#The host waited until the event was signaled and observed it. This does NOT
#signal the event; it records that the signaled state was consumed, so a later
#signal without a reset reads as reuse-without-reset, not a double-signal.
$on_successful_exit['zeEventHostSynchronize'] = lambda { |state, ctx, defi|
  state.observe_event(ctx, state.find_param(ctx, 'hEvent'))
}

#A successful status query also observes the signaled state.
$on_successful_exit['zeEventQueryStatus'] = lambda { |state, ctx, defi|
  state.observe_event(ctx, state.find_param(ctx, 'hEvent'))
}

# ADDED: device-wide host synchronization points. The host waited for all
#submitted work, so every currently-signaled event has been consumed.
#zeCommandQueueSynchronize covers regular queues; zeCommandListHostSynchronize is
#the immediate-command-list analogue (an immediate list is its own implicit
#queue) -- giving immediate lists the same event-observation semantics.
$on_successful_exit['zeCommandQueueSynchronize'] = lambda { |state, ctx, defi|
  state.observe_all_signaled_events(ctx)
}

$on_successful_exit['zeCommandListHostSynchronize'] = lambda { |state, ctx, defi|
  state.observe_all_signaled_events(ctx)
}

# --- Submission -------------------------------------------------------------
# zeCommandQueueExecuteCommandLists is the busiest callback in the file, because
# submission is where most of the "these objects must belong together" rules
# finally become checkable: the queue, the lists, their events, and the fence
# are only brought into contact here.
#
# It is also ASYNCHRONOUS -- it returns as soon as the work is queued, long
# before the GPU runs it. Hence the split: validation at entry (below), and at
# exit the lists become deferred units to be replayed as their events fire.
$upon_entry["zeCommandQueueExecuteCommandLists"] = lambda { |state, ctx, defi|
  command_queues = state.find_objects(ctx, 'command_queue')
  command_queue_handle = defi['hCommandQueue']
  command_queue = command_queues[command_queue_handle]

  #check if any command list is null
  check_valid_command_lists(state,ctx,defi)
  check_valid_command_queue(state,ctx,defi,command_queues,command_queue_handle)
  #Check if command list was closed before executing it on the queue
  #ignore if it is the first execute call
  check_command_list_closed(state, ctx, defi)
  check_fence_misuse(state,ctx,defi)

  known_command_lists = state.find_objects(ctx, 'command_list')
  # CHANGED: was `state.find_objects(ctx, 'phCommandLists_vals')` -- that is not
  # an object store; the submitted list handles are the array payload itself.
  command_list_handles = defi['phCommandLists_vals'] || []

  fences = state.find_objects(ctx, 'fence')
  fence_handle = defi['hFence']
  fence = fences[fence_handle]

  if fence
    fence.status = fence.in_use #set this at the entry so that other command lists can view it
  end

  if command_queue
    check_group_property_queued(state,ctx,defi,command_queue.device)
    check_fence_and_queue_compatibility(state,ctx,defi,command_queue,fence)
    command_list_handles.each do |command_list_handle|
      check_list_and_queue_have_matching_context(state,ctx,defi,known_command_lists[command_list_handle],command_queue)
      check_list_and_fence_have_matching_context(state,ctx,defi,known_command_lists[command_list_handle],fence)
      # ADDED: a list with a compute kernel launch must not go to a copy-only queue
      check_copy_only_queue_submission(state,ctx,command_queue,known_command_lists[command_list_handle])
      # ADDED: events used by the list must come from an event pool on the queue's context
      check_event_pool_list_context_match(state,ctx,known_command_lists[command_list_handle])
    end
  else
    # CHANGED: was raise_internal_error, which aborted the whole validator on one
    # unknown queue handle (common if tracing started after zeCommandQueueCreate).
    # Report and continue so the deferred execution below still runs.
    state.print_usage_error(ctx, "command queue #{state.get_handle_str(command_queue_handle)} was not found ")
  end
}

# CHANGED: execute is asynchronous. On success, the submitted lists' recorded ops
# become deferred execution units (each list its own unit). Their memory copies
# are checked for out-of-bounds when their wait-events are signaled, not here (see
# the scheduler in ze_validator_state_object.rb). Previously this was empty.
$on_successful_exit["zeCommandQueueExecuteCommandLists"] = lambda { |state, ctx, defi|
  known_command_lists = state.find_objects(ctx, 'command_list')
  command_list_handles = state.find_param(ctx, 'phCommandLists_vals') || []
  command_lists = command_list_handles.map { |h| known_command_lists[h] }
  state.enqueue_deferred_execution(ctx, command_lists)
}

#When a fence signals the host, set the fence's status to signaled
$on_successful_exit["zeFenceHostSynchronize"] =  lambda { |state, ctx, defi|
  # CHANGED: was `get_fence(state,ctx,defi)` -- get_fence expects a handle, not
  # defi. Read the handle from the entry params and pass it.
  fence_handle = state.find_param(ctx,"hFence")
  fence = get_fence(state, ctx, fence_handle)
  if fence
    fence.status = fence.signaled
  else
    state.print_usage_error(ctx, "nullptr fence was used for zeFenceHostSynchronize")
  end
}

#should a double reset be considered as a usage error?
#Also, a fence can be shared throughout the threads and is modeled correctly (if you are wondering about whether the model treats fence associated with different thread-id differently).
$upon_entry["zeFenceReset"] =  lambda { |state, ctx, defi|
  # CHANGED: was ZEModel::Fence.get_fence(...) and @@INITIALIZED -- Fence has no
  # such class method or class variable (that method was removed from the model;
  # @@INITIALIZED belongs to CommandList). get_fence is a top-level helper, and a
  # reset fence returns to the not_signaled instance state. This is an entry
  # callback, so input params are available directly in defi.
  curr_fence = get_fence(state, ctx, defi['hFence'])
  return unless curr_fence
  curr_fence.status = curr_fence.not_signaled
}

# ============================================================================
# OBJECT LIFECYCLE CALLBACKS.
#
# The remainder of the file is largely mechanical and follows one pattern per
# object type:
#
#   CREATE  -> build the ZEModel object, file it in the process table AND in
#              its parent's table, and validate the descriptor's stype
#   DESTROY -> remove it from both tables, and report any children that should
#              have been destroyed first
#
# The two-table bookkeeping is what makes leak reporting possible: the process
# table drives the end-of-trace sweep, while the parent's table lets a destroy
# notice that, say, an event pool still holds live events.
#
# The `{ ... }` block passed to Hash#delete is Ruby's "key not found" handler --
# it fires when a destroy names a handle the model never recorded.
# ============================================================================

#Set the driver for the current context
# zeDriverGet reports all installed drivers at once, so this registers each
# handle it returns (skipping any already known -- the app may call it twice).
$on_successful_exit['zeDriverGet'] = lambda { |state, ctx, defi|
  drivers = state.get_process(ctx).drivers
  defi['phDrivers_vals'].each { |h|
    drivers[h] = ZEModel::Driver.new(h) unless drivers[h]
  }
}

#Set device
$on_successful_exit['zeDeviceGet'] = lambda { |state, ctx, defi|
  devices = state.find_objects(ctx, 'device')
  driver = state.find_object(ctx, 'driver', 'hDriver')
  if driver
    defi['phDevices_vals'].each { |h|
      unless devices[h]
        devices[h] = ZEModel::Device.new(h)
        driver.devices.push devices[h]
      end
    }
  end
}



$on_successful_exit['zeDeviceGetSubDevices'] = lambda { |state, ctx, defi|
  devices = state.find_objects(ctx, 'device')
  device = state.find_object(ctx, 'device', 'hDevice')
  defi['phSubdevices_vals'].each { |h|
    unless devices[h]
      devices[h] = ZEModel::SubDevice.new(h, device)
      device.sub_devices.push devices[h]
    end
  }
}

$on_successful_exit['zeContextCreate'] = lambda { |state, ctx, defi|
  contexts = state.find_objects(ctx, 'context')
  driver = state.find_object(ctx, 'driver', 'hDriver')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEContextDesc)
  handle = defi['phContext_val']
  contexts[handle] = ZEModel::Context.new(handle, driver, desc)
  check_struct_stype_misuse(state,ctx,defi,:ZE_STRUCTURE_TYPE_CONTEXT_DESC,desc[:stype])
}

$on_successful_exit['zeContextCreateEx'] = lambda { |state, ctx, defi|
  contexts = state.find_objects(ctx, 'context')
  devices = state.find_objects(ctx, 'device')
  driver = state.find_object(ctx, 'driver', 'hDriver')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEContextDesc)
  devs = state.find_param(ctx, 'phDevices_vals').collect { |h| devices[h] }
  devs = nil unless state.find_param(ctx, 'phDevices') != 0
  handle = defi['phContext_val']
  contexts[handle] = ZEModel::Context.new(handle, driver, desc, devs)
}

$on_successful_exit['zeContextDestroy'] = lambda { |state, ctx, defi|
  contexts = state.find_objects(ctx, 'context')
  contexts.delete(state.find_param(ctx, 'hContext')) { |h|
    raise_internal_error(ctx, "context #{state.get_handle_str(h)} does not exist")
  }
}

$on_successful_exit['zeEventPoolCreate'] = lambda { |state, ctx, defi|
  context = state.find_object(ctx, 'context', 'hContext')
  devices = state.find_objects(ctx, 'device')
  event_pools = state.find_objects(ctx, 'event_pool')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEEventPoolDesc)
  devs = state.find_param(ctx, 'phDevices_vals').collect { |h| devices[h] }
  devs = nil unless state.find_param(ctx, 'phDevices') != 0
  handle = defi['phEventPool_val']
  event_pools[handle] = ZEModel::EventPool.new(handle, context, desc, devs)
  context.event_pools[handle] = event_pools[handle]
  check_struct_stype_misuse(state,ctx,defi,:ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,desc[:stype])
}

# Destroying a pool while events carved out of it are still alive leaves those
# events dangling, so each survivor is reported.
$on_successful_exit['zeEventPoolDestroy'] = lambda { |state, ctx, defi|
  event_pools = state.find_objects(ctx, 'event_pool')
  handle = state.find_param(ctx, 'hEventPool')
  event_pool = event_pools.delete(handle) {
    state.object_not_found(ctx, 'event_pool', handle)
  }
  event_pool.context.event_pools.delete(handle) {
    state.object_not_found(ctx, 'event_pool', handle, 'context')
  }
  event_pool.events.each { |h, _|
    state.print_usage_error(ctx, "event #{state.get_handle_str(h)} was not destroyed prior to event_pool #{state.get_handle_str(handle)} destruction")
  }
}

# Events are carved out of a pool's fixed set of slots: desc[:index] picks one.
# Set#delete? returns nil when the index was not free, which means two live
# events claim the same slot -- they would then alias each other's signal state.
$on_successful_exit['zeEventCreate'] = lambda { |state, ctx, defi|
  events = state.find_objects(ctx, 'event')
  event_pool = state.find_object(ctx, 'event_pool', 'hEventPool')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEEventDesc)
  handle = defi['phEvent_val']
  events[handle] = ZEModel::Event.new(handle, event_pool, desc)
  if !event_pool.indices.delete?(desc[:index])
    state.print_usage_error(ctx, "event_pool #{state.get_handle_str(event_pool.handle)} index #{desc[:index]} is already used")
  end
  event_pool.events[handle] = events[handle]
  check_struct_stype_misuse(state,ctx,defi,:ZE_STRUCTURE_TYPE_EVENT_DESC,desc[:stype])
}

# Releasing an event returns its slot to the pool. Set#add? returning nil means
# the slot was already free -- a double destroy of the same index.
$on_successful_exit['zeEventDestroy'] = lambda { |state, ctx, defi|
  events = state.find_objects(ctx, 'event')
  handle = state.find_param(ctx, 'hEvent')
  event = events.delete(handle) {
    state.object_not_found(ctx, 'event', handle)
  }
  event_pool = event.event_pool
  event_pool.events.delete(handle) {
    state.object_not_found(ctx, 'event', handle, 'event_pool')
  }
  if !event_pool.indices.add?(event.desc[:index])
     state.print_usage_error(ctx, "event_pool #{state.get_handle_str(event_pool.handle)} index #{event.desc[:index]} is already freed")
  end
}

$on_successful_exit['zeCommandQueueCreate'] = lambda { |state, ctx, defi|
  command_queues = state.find_objects(ctx, 'command_queue')
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device', 'hDevice')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZECommandQueueDesc)
  handle = defi['phCommandQueue_val']
  command_queues[handle] = ZEModel::CommandQueue.new(handle, context, device, desc)
  context.command_queues[handle] = command_queues[handle]
  check_struct_stype_misuse(state,ctx,defi,:ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC,desc[:stype])
}

# A queue creation that FAILED is the likely symptom of an (ordinal, index) pair
# the device does not have, so this is the moment to check the index against the
# real topology and explain the failure.
$on_erroneous_exit['zeCommandQueueCreate'] = lambda { |state, ctx, defi|
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZECommandQueueDesc)
  handle = state.find_param(ctx, 'phCommandQueue')
  check_valid_index_for_ordinal(state,ctx,handle,desc[:ordinal],desc[:index])
}

$on_successful_exit['zeCommandQueueDestroy'] = lambda { |state, ctx, defi|
  command_queues = state.find_objects(ctx, 'command_queue')
  handle = state.find_param(ctx, 'hCommandQueue')
  command_queue = command_queues.delete(handle) {
    state.object_not_found(ctx, 'command_queue', handle)
  }
  command_queue.context.command_queues.delete(handle) {
    state.object_not_found(ctx, 'command_queue', handle, 'context')
  }
  command_queue.fences.each { |h, _|
    state.print_usage_error(ctx, "fence #{state.get_handle_str(h)} was not destroyed prior to command_queue #{state.get_handle_str(handle)} destruction")
  }
}

$on_successful_exit['zeFenceCreate'] = lambda { |state, ctx, defi|
  fences = state.find_objects(ctx, 'fence')
  command_queue = state.find_object(ctx, 'command_queue', 'hCommandQueue')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEFenceDesc)
  handle = defi['phFence_val']
  fence = ZEModel::Fence.new(handle, command_queue, desc)
  fences[handle] = fence
  command_queue.fences[handle] = fence
  check_struct_stype_misuse(state,ctx,defi,:ZE_STRUCTURE_TYPE_FENCE_DESC,desc[:stype])
}

$on_successful_exit['zeFenceDestroy'] = lambda { |state, ctx, defi|
  fences = state.find_objects(ctx, 'fence')
  handle = state.find_param(ctx, 'hFence')
  fence = fences.delete(handle) {
    state.object_not_found(ctx, 'fence', handle)
  }
  command_queue = fence.command_queue
  command_queue.fences.delete(handle) {
    state.object_not_found(ctx, 'fence', handle, 'command_queue')
  }
}

$on_successful_exit['zeCommandListCreate'] = lambda { |state, ctx, defi|
  command_lists = state.find_objects(ctx, 'command_list')
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device', 'hDevice')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZECommandListDesc)
  handle = defi['phCommandList_val']
  command_lists[handle] = ZEModel::CommandList.new(handle, context, device, desc, nil)
  # ADDED: remember whether this list is in-order. desc[:flags] decodes (via the
  # FFI zebitmask) to an array of symbols; IN_ORDER means appended ops run strictly
  # in order, enabling the intra-list self-deadlock check.
  command_lists[handle].in_order = !!(desc && desc[:flags].respond_to?(:include?) &&
                                      desc[:flags].include?(:ZE_COMMAND_LIST_FLAG_IN_ORDER))
  context.command_lists[handle] = command_lists[handle]
  check_struct_stype_misuse(state,ctx,defi,:ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,desc[:stype])
}

$on_successful_exit['zeCommandListCreateImmediate'] = lambda { |state, ctx, defi|
  command_lists = state.find_objects(ctx, 'command_list')
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device', 'hDevice')
  altdesc_val = state.find_param(ctx, 'altdesc_val')
  altdesc = state.to_struct(altdesc_val, ZE::ZECommandQueueDesc)
  handle = defi['phCommandList_val']
  check_group_property_queued(state,ctx,defi,device)
  command_lists[handle] = ZEModel::CommandList.new(handle, context, device, nil, altdesc)
  command_lists[handle].immediate = true #immdediate command lists cannot be passed to the execute command lists
  command_lists[handle].associated_ordinal = altdesc[:ordinal]
  # ADDED: immediate lists carry the queue desc (altdesc); its IN_ORDER flag is the
  # queue-level one. Immediate appends still CAN deadlock among themselves (e.g.
  # op1 waits A/signals B while op2 waits B/signals A) -- but each append is its
  # own single-op DeferredUnit, so such a cycle is a CROSS-unit cycle already
  # caught by check_circular_deadlock, not the single-unit case
  # check_in_order_self_deadlock handles. Recorded here for consistency.
  command_lists[handle].in_order = !!(altdesc && altdesc[:flags].respond_to?(:include?) &&
                                      altdesc[:flags].include?(:ZE_COMMAND_QUEUE_FLAG_IN_ORDER))
  context.command_lists[handle] = command_lists[handle]

  #immediate command list does not take in the list descriptor as an input
  check_struct_stype_misuse(state,ctx,defi,:ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC,altdesc[:stype])
}

$on_successful_exit['zeCommandListDestroy'] = lambda { |state, ctx, defi|
  command_lists = state.find_objects(ctx, 'command_list')
  handle = state.find_param(ctx, 'hCommandList')
  command_list = command_lists.delete(handle) {
    state.object_not_found(ctx, 'command_list', handle)
  }
  command_list.context.command_lists.delete(handle) {
    state.object_not_found(ctx, 'command_list', handle, 'context')
  }
}

$on_successful_exit['zeModuleCreate'] = lambda { |state, ctx, defi|
  modules = state.find_objects(ctx, 'module')
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device', 'hDevice')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEModuleDesc)
  handle = defi['phModule_val']
  mod = ZEModel::Module.new(handle, context, device, desc)
  modules[handle] = mod
  context.modules[handle] = mod
  build_log_handle = defi['phBuildLog_val']
  if build_log_handle != 0
    module_build_logs = state.find_objects(ctx, 'module_build_log')
    build_log = ZEModel::Module::BuildLog.new(build_log_handle, mod)
    module_build_logs[build_log_handle] = build_log
    context.module_build_logs[build_log_handle] = build_log
    modules[handle].build_log = build_log
  end
  check_struct_stype_misuse(state,ctx,defi,:ZE_STRUCTURE_TYPE_MODULE_DESC,desc[:stype])


}

$on_erroneous_exit['zeModuleCreate'] = lambda { |state, ctx, defi|
  build_log_handle = defi['phBuildLog_val']
  if build_log_handle != 0
    module_build_logs = state.find_objects(ctx, 'module_build_log')
    build_log = ZEModel::Module::BuildLog.new(build_log_handle)
    module_build_logs[build_log_handle] = build_log
    context.module_build_logs[build_log_handle] = build_log
  end
}

$on_successful_exit['zeModuleDestroy'] = lambda { |state, ctx, defi|
  modules = state.find_objects(ctx, 'module')
  handle = state.find_param(ctx, 'hModule')
  mod = modules.delete(handle) {
    state.object_not_found(ctx, 'module', handle)
  }
  mod.context.modules.delete(handle) {
    state.object_not_found(ctx, 'module', handle, 'context')
  }
  mod.kernels.each { |h, _|
    state.print_usage_error(ctx, "kernel #{state.get_handle_str(h)} was not destroyed prior to module #{state.get_handle_str(handle)} destruction")
  }
}

$on_erroneous_exit['zeModuleDynamicLink'] = $on_successful_exit['zeModuleDynamicLink'] = lambda { |state, ctx, defi|
  build_log_handle = defi['phLinkLog_val']
  if build_log_handle != 0
    module_build_logs = state.find_objects(ctx, 'module_build_log')
    build_log = ZEModel::Module::BuildLog.new(build_log_handle)
    module_build_logs[build_log_handle] = build_log
    context.module_build_logs[build_log_handle] = build_log
  end
}

$on_successful_exit['zeModuleBuildLogDestroy'] = lambda { |state, ctx, defi|
  module_build_logs = state.find_objects(ctx, 'module_build_log')
  handle = state.find_param(ctx, 'hModuleBuildLog')
  module_build_log = module_build_logs.delete(handle) {
    state.object_not_found(ctx, 'module_build_log', handle)
  }
  if module_build_log.module
    module_build_log.module.context.module_build_logs.delete(handle) {
      state.object_not_found(ctx, 'module_build_log', handle, 'context')
    }
    module_build_log.module.build_log = nil
  end
}

$upon_entry['zeKernelCreate'] = lambda {|state, ctx, defi|
  check_valid_module(state,ctx, defi)
}

$on_successful_exit['zeKernelCreate'] = lambda { |state, ctx, defi|
  kernels = state.find_objects(ctx, 'kernel')
  mod = state.find_object(ctx, 'module', 'hModule')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEKernelDesc)
  handle = defi['phKernel_val']
  kernelName = state.find_param(ctx, 'desc__pKernelName_val')
  kernel = ZEModel::Kernel.new(handle, mod, desc, kernelName)
  kernels[handle] = kernel
  mod.kernels[handle] = kernel
  check_struct_stype_misuse(state,ctx,defi,:ZE_STRUCTURE_TYPE_KERNEL_DESC, desc[:stype])
}

$on_successful_exit['zeKernelDestroy'] = lambda { |state, ctx, defi|
  kernels = state.find_objects(ctx, 'kernel')
  handle = state.find_param(ctx, 'hKernel')
  kernel = kernels.delete(handle) {
    state.object_not_found(ctx, 'kernel', handle)
  }
  mod = kernel.module
  mod.kernels.delete(handle) {
    state.object_not_found(ctx, 'kernel', handle, 'module')
  }
}

# REMOVED: a second, broken $on_successful_exit['zeCommandListAppendMemoryCopy']
# used to live here. It called add_api_call_to_cmd_list (undefined locals) and,
# being defined later, would have overridden the recorder above. The single
# recorder near the other append callbacks now handles this API.

# ============================================================================
# MEMORY ALLOCATION AND FREEING.
#
# Three allocation flavors, one free. Each allocator does the same four things:
#   1. resolve the Level Zero context -- allocations are keyed by it, since an
#      address is only guaranteed unique WITHIN a context
#   2. call mark_reallocated, because the driver may hand back an address that
#      was freed earlier; without this the stale freed-record would make the
#      fresh allocation look like a dangling pointer
#   3. build the Memory object and file it in the process's per-context map
#      (and, for device memory, on the owning Device as well)
#   4. validate the descriptor's stype
# ============================================================================

#The implementation of this (zeMemAllocDevice) function must be thread-safe
$on_successful_exit['zeMemAllocDevice'] = lambda { |state, ctx, defi|
  # memory is associated with devices
  ctx_handle = state.find_param(ctx, 'hContext') # ADDED: key allocations by context
  memory_allocations = state.memory_allocations(ctx, ctx_handle)
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device','hDevice')
  size = state.find_param(ctx,"size")
  device_desc_val = state.find_param(ctx,"device_desc_val")
  handle = defi['pptr_val']
  mark_reallocated(state, ctx, ctx_handle, handle, size) # ADDED: address may reuse a freed range in this context
  memory_allocation =  ZEModel::Memory.new(handle, context, size, device, "device")
  memory_allocations[handle] = memory_allocation
  device.memory_allocations[ctx_handle][handle] = memory_allocation # CHANGED: per-context device sub-map
  device_desc = state.to_struct(device_desc_val, ZE::ZEDeviceMemAllocDesc)
  check_struct_stype_misuse(state,ctx,defi,:ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC, device_desc[:stype])
}


$on_successful_exit['zeMemAllocShared'] = lambda { |state, ctx, defi|
  ctx_handle = state.find_param(ctx, 'hContext') # ADDED: key allocations by context
  memory_allocations = state.memory_allocations(ctx, ctx_handle)
  # finds the device and context objects associated with the params
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device','hDevice')
  # Passing nullptr as the device handle does not associate the shared allocation with any device.
  # For allocations with no associated device, ownership of the allocation is shared between the
  # host and all devices supporting cross-device shared access capabilities.
  # TODO: should add in code to add this mme allocation to all devices with that property
  size = state.find_param(ctx,"size")
  handle = defi['pptr_val']
  mark_reallocated(state, ctx, ctx_handle, handle, size) # ADDED: address may reuse a freed range in this context
  memory_allocation =  ZEModel::Memory.new(handle, context, size, device)
  memory_allocations[handle] = memory_allocation
  device.memory_allocations[ctx_handle][handle] = memory_allocation if device # CHANGED: per-context device sub-map
}

$on_successful_exit['zeMemAllocHost'] = lambda { |state, ctx, defi|
  # Host allocations are accessible by the host and all devices within the driver’s context.
  # TODO: add this memory allocation to all devices in the context
  ctx_handle = state.find_param(ctx, 'hContext') # ADDED: key allocations by context
  memory_allocations = state.memory_allocations(ctx, ctx_handle)
  context = state.find_object(ctx, 'context', 'hContext')
  size = state.find_param(ctx,"size")
  handle = defi['pptr_val']
  mark_reallocated(state, ctx, ctx_handle, handle, size) # ADDED: address may reuse a freed range in this context
  memory_allocation =  ZEModel::Memory.new(handle, context, size, nil, "host")
  memory_allocations[handle] = memory_allocation
}

# CHANGED: apply the free at ENTRY, not exit. On the program timeline the app
# relinquishes the buffer at the call to zeMemFree; nothing after that call may
# touch it. Applying the free at _exit is wrong when zeMemFree BLOCKS until the
# buffer is idle: a gated copy that reads the buffer can be released (by another
# thread signaling its wait event) and executed BETWEEN this call's _entry and
# _exit, so at _exit the copy has already drained (nothing looks in-flight) and,
# while the copy ran, the model had not yet marked the buffer freed (no UAF).
# Doing it at entry lets check_free_in_flight see the still-parked copy, and
# marks the buffer freed before that copy is later replayed, so the deferred UAF
# check fires too. If the free actually fails, the erroneous-exit handler below
# restores the allocation.
$upon_entry['zeMemFree'] = lambda { |state, ctx, defi|
  ctx_handle = defi['hContext'] # ADDED: allocations are keyed by freeing context
  memory_allocations = state.memory_allocations(ctx, ctx_handle)
  handle = defi['ptr']
  memory_allocation = memory_allocations[handle]
  next unless memory_allocation
  # flag if this buffer is still referenced by a copy/fill that has been
  # submitted but not yet completed (in-flight device work would touch freed mem)
  check_free_in_flight(state, ctx, memory_allocation)
  memory_allocations.delete(handle)
  owned = memory_allocation.owned_by
  owned.memory_allocations[ctx_handle].delete(handle) if owned # CHANGED: per-context device sub-map
  # keep the freed allocation in this context's freed registry so a later
  # copy/fill/kernel referencing this address is caught as use-after-free
  memory_allocation.freed_by = state.get_api_context(ctx)
  state.freed_memory_allocations(ctx, ctx_handle)[handle] = memory_allocation
}

# ADDED: the free was applied at entry; if the driver reported failure, the
# buffer is actually still alive -- move it back from the freed registry to the
# live set so it is not falsely flagged as use-after-free later.
$on_erroneous_exit['zeMemFree'] = lambda { |state, ctx, defi|
  ctx_handle = state.find_param(ctx, 'hContext') # ADDED: same context the entry freed under
  handle = state.find_param(ctx, "ptr")
  mem = state.freed_memory_allocations(ctx, ctx_handle).delete(handle)
  if mem
    mem.freed_by = nil
    state.memory_allocations(ctx, ctx_handle)[handle] = mem
    owned = mem.owned_by
    owned.memory_allocations[ctx_handle][handle] = mem if owned # CHANGED: per-context device sub-map
  end
}