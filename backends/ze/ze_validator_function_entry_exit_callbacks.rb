require 'ze_validator_entry_exit_helpers'
require 'ze_validator_zemodel'
require 'ze_library'



$upon_entry = {} #called to modify program state on entry
$on_successful_exit = {} #called upon seeing exit functions with a successful return code
$on_erroneous_exit = {} #called upon seeing exit functions with a non-successful return code

#these two record only that the app asked, for the portability checks
$on_successful_exit["zeDeviceGetProperties"] = lambda{|state, ctx, payload|
  device_ptr = state.find_param(ctx,'hDevice')
  devices = state.find_objects(ctx, 'device')
  devices[device_ptr].property_fetched = true
}
# Mark that the queue group property was queried.
$on_successful_exit["zeDeviceGetCommandQueueGroupProperties"] = lambda{|state, ctx, payload|
  device_ptr = state.find_param(ctx,'hDevice')
  devices = state.find_objects(ctx, 'device')
  devices[device_ptr].cmd_queue_group_properties_queried = true
}

# For every append API below: validation at entry (the call may crash),
# recording at exit (only a successful append will ever execute).
$upon_entry["zeCommandListAppendLaunchKernel"] = lambda { |state, ctx, payload|
  #Retrieve the compute ordinal from the command list
  command_lists = state.find_objects(ctx, 'command_list')
  cmd_list = command_lists[payload['hCommandList']]
  cqg_ordinal = 0
  #a normal list carries the ordinal in desc; an immediate list in altdesc
  if cmd_list && cmd_list.desc
    cqg_ordinal = cmd_list.desc[:commandQueueGroupOrdinal]
  elsif cmd_list && cmd_list.altdesc
    cqg_ordinal =  cmd_list.altdesc[:ordinal]
  end
  #both checks must run even if the launch later aborts
  check_valid_ordinal(state, ctx, payload, cqg_ordinal)
  check_kernel_created(state, ctx, payload)
  #the kernel's module must be on the same context as the command list
  check_kernel_list_context_match(state, ctx, payload)
}

$on_successful_exit["zeCommandListAppendLaunchKernel"] = lambda { |state, ctx, payload|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:launch,
              signal: state.find_param(ctx, 'hSignalEvent'),
              waits: wait_event_handles(state, ctx),
              api: 'zeCommandListAppendLaunchKernel'))
}

$upon_entry["zeCommandListReset"] = lambda { |state, ctx, payload|
  check_command_list_reset(state, ctx, payload)
}

# on success the list is empty and open again, so clear the recorded ops.
# In-flight executions are unaffected: they snapshotted the ops at submit time.
$on_successful_exit["zeCommandListReset"] = lambda { |state, ctx, payload|
  command_lists = state.find_objects(ctx, 'command_list')
  cmd_list = command_lists[state.find_param(ctx, 'hCommandList')]
  return unless cmd_list
  cmd_list.ops.clear
  cmd_list.status = ZEModel::CommandList.class_variable_get(:@@INITIALIZED)
}

#check_command_list_closed later verifies this happened before any submission
$on_successful_exit["zeCommandListClose"] = lambda { |state, ctx, payload|
  command_lists = state.find_objects(ctx, 'command_list')
  command_list_handle = state.find_param(ctx,"hCommandList")
  cmd_list = command_lists[command_list_handle]
  cmd_list.status = ZEModel::CommandList.class_variable_get(:@@CLOSED)
}

$upon_entry["zeCommandListAppendLaunchCooperativeKernel"] = lambda { |state, ctx, payload|
  command_lists = state.find_objects(ctx, 'command_list')
  cmd_list = command_lists[payload['hCommandList']]
  check_group_property_queued(state,ctx,payload,cmd_list.device) if cmd_list
  #the kernel's module must be on the same context as the command list
  check_kernel_list_context_match(state, ctx, payload)
}

$on_successful_exit["zeCommandListAppendLaunchCooperativeKernel"] = lambda { |state, ctx, payload|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:launch,
              signal: state.find_param(ctx, 'hSignalEvent'),
              waits: wait_event_handles(state, ctx),
              api: 'zeCommandListAppendLaunchCooperativeKernel'))
}

# Copy/event ops are recorded in list order for deferred replay. The
# out-of-bounds check waits until the op's wait-events are satisfied.
$upon_entry['zeCommandListAppendMemoryCopy'] = lambda { |state, ctx, payload|
  params = { api: 'zeCommandListAppendMemoryCopy',
             ctx_handle: cmd_list_ctx_handle(state, ctx, payload['hCommandList']),
             dst: payload['dstptr'], src: payload['srcptr'], size: payload['size'] }
  waits = wait_event_handles(state, ctx)
  check_null_copy_ptr(state, ctx, 'zeCommandListAppendMemoryCopy',
    { 'destination' => payload['dstptr'], 'source' => payload['srcptr'] })
  check_use_after_free_on_append(state, ctx, params, waits)
  #an out-of-bounds copy can crash the driver, which emits no _exit
  check_oob_copy_on_append(state, ctx, params, waits)
  # known memory endpoints must be allocated on the command list's context
  check_copy_ptr_list_context(state, ctx, 'zeCommandListAppendMemoryCopy', payload['hCommandList'],
    { 'destination' => payload['dstptr'], 'source' => payload['srcptr'] })
}

$upon_entry['zeCommandListAppendMemoryFill'] = lambda { |state, ctx, payload|
  params = { api: 'zeCommandListAppendMemoryFill',
             ctx_handle: cmd_list_ctx_handle(state, ctx, payload['hCommandList']),
             dst: payload['ptr'], src: nil, size: payload['size'] }
  waits = wait_event_handles(state, ctx)
  check_null_copy_ptr(state, ctx, 'zeCommandListAppendMemoryFill',
    { 'destination' => payload['ptr'] })
  check_use_after_free_on_append(state, ctx, params, waits)
  #an out-of-bounds fill can crash the driver, which emits no _exit
  check_oob_copy_on_append(state, ctx, params, waits)
  # known memory endpoint must be allocated on the command list's context
  check_copy_ptr_list_context(state, ctx, 'zeCommandListAppendMemoryFill', payload['hCommandList'],
    { 'destination' => payload['ptr'] })
}

$on_successful_exit['zeCommandListAppendMemoryCopy'] = lambda { |state, ctx, payload|
  record_copy_op(state, ctx, 'zeCommandListAppendMemoryCopy', 'dstptr', 'srcptr')
}

$on_successful_exit['zeCommandListAppendMemoryFill'] = lambda { |state, ctx, payload|
  #a fill only touches the destination; model it as a copy with no source
  record_copy_op(state, ctx, 'zeCommandListAppendMemoryFill', 'ptr', nil)
}

# A failed append is never recorded, so the deferred check would never see it,
# but the copy is out-of-bounds regardless of the error code. Check it here.
$on_erroneous_exit['zeCommandListAppendMemoryCopy'] = lambda { |state, ctx, payload|
  params = { api: 'zeCommandListAppendMemoryCopy',
             ctx_handle: cmd_list_ctx_handle(state, ctx, state.find_param(ctx, 'hCommandList')),
             dst: state.find_param(ctx, 'dstptr'),
             src: state.find_param(ctx, 'srcptr'),
             size: state.find_param(ctx, 'size') }
  check_oob_copy(state, ctx, params)
  check_use_after_free(state, ctx, params)
}

$on_erroneous_exit['zeCommandListAppendMemoryFill'] = lambda { |state, ctx, payload|
  params = { api: 'zeCommandListAppendMemoryFill',
             ctx_handle: cmd_list_ctx_handle(state, ctx, state.find_param(ctx, 'hCommandList')),
             dst: state.find_param(ctx, 'ptr'),
             src: nil,
             size: state.find_param(ctx, 'size') }
  check_oob_copy(state, ctx, params)
  check_use_after_free(state, ctx, params)
}

# region copies carry 2D/3D extents, so `size` is not a flat byte count; we only
# record ordering + event effects and skip the flat OOB comparison
$on_successful_exit['zeCommandListAppendMemoryCopyRegion'] = lambda { |state, ctx, payload|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:launch,
              signal: state.find_param(ctx, 'hSignalEvent'),
              waits: wait_event_handles(state, ctx),
              api: 'zeCommandListAppendMemoryCopyRegion'))
}

#A device-side signal: the event is signaled when this op executes (after waits).
$on_successful_exit['zeCommandListAppendSignalEvent'] = lambda { |state, ctx, payload|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:signal, signal: state.find_param(ctx, 'hEvent'),
              api: 'zeCommandListAppendSignalEvent'))
}

#A device-side wait: this op blocks the list until phEvents are signaled.
$on_successful_exit['zeCommandListAppendWaitOnEvents'] = lambda { |state, ctx, payload|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:wait, waits: wait_event_handles(state, ctx),
              api: 'zeCommandListAppendWaitOnEvents'))
}

#A device-side reset: returns the event to unsignaled when this op executes.
$on_successful_exit['zeCommandListAppendEventReset'] = lambda { |state, ctx, payload|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:reset, params: { reset_handle: state.find_param(ctx, 'hEvent') }))
}

#A barrier waits on its events and signals its completion event.
$on_successful_exit['zeCommandListAppendBarrier'] = lambda { |state, ctx, payload|
  record_op(state, ctx, state.find_param(ctx, 'hCommandList'),
            ZEModel::RecordedOp.new(:barrier,
              signal: state.find_param(ctx, 'hSignalEvent'),
              waits: wait_event_handles(state, ctx),
              api: 'zeCommandListAppendBarrier'))
}

# Same event semantics as a plain barrier, plus the memory ranges it names,
# which are validated when the barrier executes (check_uaf_ranges_barrier).
$on_successful_exit['zeCommandListAppendMemoryRangesBarrier'] = lambda { |state, ctx, payload|
  record_ranges_barrier_op(state, ctx)
}

#host-side event operations, effective immediately in trace order
$on_successful_exit['zeEventHostSignal'] = lambda { |state, ctx, payload|
  handle = state.find_param(ctx, 'hEvent')
  check_event_signal_reuse(state, ctx, handle, 'zeEventHostSignal')
  state.signal_event(ctx, handle, 'zeEventHostSignal')
}

$on_successful_exit['zeEventHostReset'] = lambda { |state, ctx, payload|
  state.reset_event(ctx, state.find_param(ctx, 'hEvent'))
}

#does not signal the event, only records that the signaled state was consumed
$on_successful_exit['zeEventHostSynchronize'] = lambda { |state, ctx, payload|
  state.observe_event(ctx, state.find_param(ctx, 'hEvent'))
}

#A successful status query also observes the signaled state.
$on_successful_exit['zeEventQueryStatus'] = lambda { |state, ctx, payload|
  state.observe_event(ctx, state.find_param(ctx, 'hEvent'))
}

#the host waited for all submitted work, so every signaled event was consumed
$on_successful_exit['zeCommandQueueSynchronize'] = lambda { |state, ctx, payload|
  state.observe_all_signaled_events(ctx)
}

$on_successful_exit['zeCommandListHostSynchronize'] = lambda { |state, ctx, payload|
  state.observe_all_signaled_events(ctx)
}

# Submission is where the queue, the lists, their events and the fence are
# first brought together, so most "must belong together" rules are checked here.
$upon_entry["zeCommandQueueExecuteCommandLists"] = lambda { |state, ctx, payload|
  command_queues = state.find_objects(ctx, 'command_queue')
  command_queue_handle = payload['hCommandQueue']
  command_queue = command_queues[command_queue_handle]

  #check if any command list is null
  check_valid_command_lists(state,ctx,payload)
  check_valid_command_queue(state,ctx,payload,command_queues,command_queue_handle)
  #Check if command list was closed before executing it on the queue
  #ignore if it is the first execute call
  check_command_list_closed(state, ctx, payload)
  check_fence_misuse(state,ctx,payload)

  known_command_lists = state.find_objects(ctx, 'command_list')
  command_list_handles = payload['phCommandLists_vals'] || []

  fences = state.find_objects(ctx, 'fence')
  fence_handle = payload['hFence']
  fence = fences[fence_handle]

  if fence
    fence.status = fence.in_use #set this at the entry so that other command lists can view it
  end

  if command_queue
    check_group_property_queued(state,ctx,payload,command_queue.device)
    check_fence_and_queue_compatibility(state,ctx,payload,command_queue,fence)
    command_list_handles.each do |command_list_handle|
      check_list_and_queue_have_matching_context(state,ctx,payload,known_command_lists[command_list_handle],command_queue)
      check_list_and_fence_have_matching_context(state,ctx,payload,known_command_lists[command_list_handle],fence)
      #a list with a compute kernel launch must not go to a copy-only queue
      check_copy_only_queue_submission(state,ctx,command_queue,known_command_lists[command_list_handle])
      #events used by the list must come from a pool on the queue's context
      check_event_pool_list_context_match(state,ctx,known_command_lists[command_list_handle])
    end
  else
    #report and continue so the deferred execution below still runs
    state.print_usage_error(ctx, "command queue #{state.get_handle_str(command_queue_handle)} was not found ")
  end
}

# Execute is asynchronous, so each submitted list becomes a deferred unit and
# its copies are checked when their wait-events are signaled, not here.
$on_successful_exit["zeCommandQueueExecuteCommandLists"] = lambda { |state, ctx, payload|
  known_command_lists = state.find_objects(ctx, 'command_list')
  command_list_handles = state.find_param(ctx, 'phCommandLists_vals') || []
  command_lists = command_list_handles.map { |h| known_command_lists[h] }
  state.enqueue_deferred_execution(ctx, command_lists)
}

#When a fence signals the host, set the fence's status to signaled
$on_successful_exit["zeFenceHostSynchronize"] =  lambda { |state, ctx, payload|
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
$upon_entry["zeFenceReset"] =  lambda { |state, ctx, payload|
  curr_fence = get_fence(state, ctx, payload['hFence'])
  return unless curr_fence
  curr_fence.status = curr_fence.not_signaled
}

# Object lifecycle callbacks. Create files the object in the process table and
# its parent's; destroy removes both and reports children still alive.

#Set the driver for the current context
$on_successful_exit['zeDriverGet'] = lambda { |state, ctx, payload|
  drivers = state.get_process(ctx).drivers
  payload['phDrivers_vals'].each { |h|
    drivers[h] = ZEModel::Driver.new(h) unless drivers[h]
  }
}

#Create device objects
$on_successful_exit['zeDeviceGet'] = lambda { |state, ctx, payload|
  devices = state.find_objects(ctx, 'device')
  driver = state.find_object(ctx, 'driver', 'hDriver')
  if driver
    payload['phDevices_vals'].each { |h|
      unless devices[h]
        devices[h] = ZEModel::Device.new(h)
        driver.devices.push devices[h]
      end
    }
  end
}


#Create subdevice objects, with device as a parent
$on_successful_exit['zeDeviceGetSubDevices'] = lambda { |state, ctx, payload|
  devices = state.find_objects(ctx, 'device')
  device = state.find_object(ctx, 'device', 'hDevice')
  payload['phSubdevices_vals'].each { |h|
    unless devices[h]
      devices[h] = ZEModel::SubDevice.new(h, device)
      device.sub_devices.push devices[h]
    end
  }
}

#Create ze context objects
$on_successful_exit['zeContextCreate'] = lambda { |state, ctx, payload|
  contexts = state.find_objects(ctx, 'context')
  driver = state.find_object(ctx, 'driver', 'hDriver')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEContextDesc)
  handle = payload['phContext_val']
  contexts[handle] = ZEModel::Context.new(handle, driver, desc)
  check_struct_stype_misuse(state,ctx,payload,:ZE_STRUCTURE_TYPE_CONTEXT_DESC,desc[:stype])
}

#Experimental API, it practically serves the same purpose as zeContextCreate
$on_successful_exit['zeContextCreateEx'] = lambda { |state, ctx, payload|
  contexts = state.find_objects(ctx, 'context')
  devices = state.find_objects(ctx, 'device')
  driver = state.find_object(ctx, 'driver', 'hDriver')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEContextDesc)
  devs = state.find_param(ctx, 'phDevices_vals').collect { |h| devices[h] }
  devs = nil unless state.find_param(ctx, 'phDevices') != 0
  handle = payload['phContext_val']
  contexts[handle] = ZEModel::Context.new(handle, driver, desc, devs)
}

# Releases the corresponding ze context object
$on_successful_exit['zeContextDestroy'] = lambda { |state, ctx, payload|
  contexts = state.find_objects(ctx, 'context')
  contexts.delete(state.find_param(ctx, 'hContext')) { |h|
    raise_internal_error(ctx, "context #{state.get_handle_str(h)} does not exist")
  }
}

# Creates an event pool
$on_successful_exit['zeEventPoolCreate'] = lambda { |state, ctx, payload|
  context = state.find_object(ctx, 'context', 'hContext')
  devices = state.find_objects(ctx, 'device')
  event_pools = state.find_objects(ctx, 'event_pool')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEEventPoolDesc)
  devs = state.find_param(ctx, 'phDevices_vals').collect { |h| devices[h] }
  devs = nil unless state.find_param(ctx, 'phDevices') != 0
  handle = payload['phEventPool_val']
  event_pools[handle] = ZEModel::EventPool.new(handle, context, desc, devs)
  context.event_pools[handle] = event_pools[handle]
  check_struct_stype_misuse(state,ctx,payload,:ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,desc[:stype])
}

# Destroys the given event pool
# Destroying a pool while events carved out of it are still alive leads to leaks
$on_successful_exit['zeEventPoolDestroy'] = lambda { |state, ctx, payload|
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


#Create an event
$on_successful_exit['zeEventCreate'] = lambda { |state, ctx, payload|
  events = state.find_objects(ctx, 'event')
  event_pool = state.find_object(ctx, 'event_pool', 'hEventPool')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEEventDesc)
  handle = payload['phEvent_val']
  events[handle] = ZEModel::Event.new(handle, event_pool, desc)
  # "delete?" returns nil when desc[:index] was not free.
  if !event_pool.indices.delete?(desc[:index])
    state.print_usage_error(ctx, "event_pool #{state.get_handle_str(event_pool.handle)} index #{desc[:index]} is already used")
  end
  event_pool.events[handle] = events[handle]
  check_struct_stype_misuse(state,ctx,payload,:ZE_STRUCTURE_TYPE_EVENT_DESC,desc[:stype])
}

# Destroys an event
$on_successful_exit['zeEventDestroy'] = lambda { |state, ctx, payload|
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

# Creates a command queue
$on_successful_exit['zeCommandQueueCreate'] = lambda { |state, ctx, payload|
  command_queues = state.find_objects(ctx, 'command_queue')
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device', 'hDevice')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZECommandQueueDesc)
  handle = payload['phCommandQueue_val']
  command_queues[handle] = ZEModel::CommandQueue.new(handle, context, device, desc)
  context.command_queues[handle] = command_queues[handle]
  check_struct_stype_misuse(state,ctx,payload,:ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC,desc[:stype])
}

# A failed creation is likely an (ordinal, index) the device does not have, so
# check the index against the real topology to explain the failure.
$on_erroneous_exit['zeCommandQueueCreate'] = lambda { |state, ctx, payload|
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZECommandQueueDesc)
  handle = state.find_param(ctx, 'phCommandQueue')
  check_valid_index_for_ordinal(state,ctx,handle,desc[:ordinal],desc[:index])
}

# Destroys the given command queue
$on_successful_exit['zeCommandQueueDestroy'] = lambda { |state, ctx, payload|
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

# Creates a fence
$on_successful_exit['zeFenceCreate'] = lambda { |state, ctx, payload|
  fences = state.find_objects(ctx, 'fence')
  command_queue = state.find_object(ctx, 'command_queue', 'hCommandQueue')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEFenceDesc)
  handle = payload['phFence_val']
  fence = ZEModel::Fence.new(handle, command_queue, desc)
  fences[handle] = fence
  command_queue.fences[handle] = fence
  check_struct_stype_misuse(state,ctx,payload,:ZE_STRUCTURE_TYPE_FENCE_DESC,desc[:stype])
}

# Destroys the fence
$on_successful_exit['zeFenceDestroy'] = lambda { |state, ctx, payload|
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

# Creates the command list
$on_successful_exit['zeCommandListCreate'] = lambda { |state, ctx, payload|
  command_lists = state.find_objects(ctx, 'command_list')
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device', 'hDevice')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZECommandListDesc)
  handle = payload['phCommandList_val']
  command_lists[handle] = ZEModel::CommandList.new(handle, context, device, desc, nil)
  #in-order enables the intra-list self-deadlock check
  command_lists[handle].in_order = !!(desc && desc[:flags].respond_to?(:include?) &&
                                      desc[:flags].include?(:ZE_COMMAND_LIST_FLAG_IN_ORDER))
  context.command_lists[handle] = command_lists[handle]
  check_struct_stype_misuse(state,ctx,payload,:ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,desc[:stype])
}

# Creates an immediate command list
$on_successful_exit['zeCommandListCreateImmediate'] = lambda { |state, ctx, payload|
  command_lists = state.find_objects(ctx, 'command_list')
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device', 'hDevice')
  altdesc_val = state.find_param(ctx, 'altdesc_val')
  altdesc = state.to_struct(altdesc_val, ZE::ZECommandQueueDesc)
  handle = payload['phCommandList_val']
  check_group_property_queued(state,ctx,payload,device)
  command_lists[handle] = ZEModel::CommandList.new(handle, context, device, nil, altdesc)
  command_lists[handle].immediate = true #immdediate command lists cannot be passed to the execute command lists
  command_lists[handle].associated_ordinal = altdesc[:ordinal]
  #each immediate append is its own single-op unit, so a cycle between them is
  #caught by check_circular_deadlock; recorded here for consistency
  command_lists[handle].in_order = !!(altdesc && altdesc[:flags].respond_to?(:include?) &&
                                      altdesc[:flags].include?(:ZE_COMMAND_QUEUE_FLAG_IN_ORDER))
  context.command_lists[handle] = command_lists[handle]

  #immediate command list does not take in the list descriptor as an input
  check_struct_stype_misuse(state,ctx,payload,:ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC,altdesc[:stype])
}

# Destroys the command list
$on_successful_exit['zeCommandListDestroy'] = lambda { |state, ctx, payload|
  command_lists = state.find_objects(ctx, 'command_list')
  handle = state.find_param(ctx, 'hCommandList')
  command_list = command_lists.delete(handle) {
    state.object_not_found(ctx, 'command_list', handle)
  }
  command_list.context.command_lists.delete(handle) {
    state.object_not_found(ctx, 'command_list', handle, 'context')
  }
}

# Destroys the module
$on_successful_exit['zeModuleCreate'] = lambda { |state, ctx, payload|
  modules = state.find_objects(ctx, 'module')
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device', 'hDevice')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEModuleDesc)
  handle = payload['phModule_val']
  mod = ZEModel::Module.new(handle, context, device, desc)
  modules[handle] = mod
  context.modules[handle] = mod
  build_log_handle = payload['phBuildLog_val']
  if build_log_handle != 0
    module_build_logs = state.find_objects(ctx, 'module_build_log')
    build_log = ZEModel::Module::BuildLog.new(build_log_handle, mod)
    module_build_logs[build_log_handle] = build_log
    context.module_build_logs[build_log_handle] = build_log
    modules[handle].build_log = build_log
  end
  check_struct_stype_misuse(state,ctx,payload,:ZE_STRUCTURE_TYPE_MODULE_DESC,desc[:stype])


}

# Runs diagnoistics on why the module crete failed
$on_erroneous_exit['zeModuleCreate'] = lambda { |state, ctx, payload|
  build_log_handle = payload['phBuildLog_val']
  if build_log_handle != 0
    module_build_logs = state.find_objects(ctx, 'module_build_log')
    build_log = ZEModel::Module::BuildLog.new(build_log_handle)
    module_build_logs[build_log_handle] = build_log
    context.module_build_logs[build_log_handle] = build_log
  end
}

# Destroys the module. Kernel must be destroyed first.
$on_successful_exit['zeModuleDestroy'] = lambda { |state, ctx, payload|
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

# Appends build log at dynamic link
$on_erroneous_exit['zeModuleDynamicLink'] = $on_successful_exit['zeModuleDynamicLink'] = lambda { |state, ctx, payload|
  build_log_handle = payload['phLinkLog_val']
  if build_log_handle != 0
    module_build_logs = state.find_objects(ctx, 'module_build_log')
    build_log = ZEModel::Module::BuildLog.new(build_log_handle)
    module_build_logs[build_log_handle] = build_log
    context.module_build_logs[build_log_handle] = build_log
  end
}

# Destroys the build log
$on_successful_exit['zeModuleBuildLogDestroy'] = lambda { |state, ctx, payload|
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

# upon entering, check if a valid module was passed
$upon_entry['zeKernelCreate'] = lambda {|state, ctx, payload|
  check_valid_module(state,ctx, payload)
}

# Creates the kernel object
$on_successful_exit['zeKernelCreate'] = lambda { |state, ctx, payload|
  kernels = state.find_objects(ctx, 'kernel')
  mod = state.find_object(ctx, 'module', 'hModule')
  desc_val = state.find_param(ctx, 'desc_val')
  desc = state.to_struct(desc_val, ZE::ZEKernelDesc)
  handle = payload['phKernel_val']
  kernelName = state.find_param(ctx, 'desc__pKernelName_val')
  kernel = ZEModel::Kernel.new(handle, mod, desc, kernelName)
  kernels[handle] = kernel
  mod.kernels[handle] = kernel
  check_struct_stype_misuse(state,ctx,payload,:ZE_STRUCTURE_TYPE_KERNEL_DESC, desc[:stype])
}

# Destroys the kernel
$on_successful_exit['zeKernelDestroy'] = lambda { |state, ctx, payload|
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

# Each allocator keys the allocation by its Level Zero context, then calls
# mark_reallocated since the driver may hand back an address that was freed.

# Allocs device memory
$on_successful_exit['zeMemAllocDevice'] = lambda { |state, ctx, payload|
  # memory is associated with devices
  ctx_handle = state.find_param(ctx, 'hContext')
  memory_allocations = state.memory_allocations(ctx, ctx_handle)
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device','hDevice')
  size = state.find_param(ctx,"size")
  device_desc_val = state.find_param(ctx,"device_desc_val")
  handle = payload['pptr_val']
  mark_reallocated(state, ctx, ctx_handle, handle, size)
  memory_allocation =  ZEModel::Memory.new(handle, context, size, device, "device")
  memory_allocations[handle] = memory_allocation
  device.memory_allocations[ctx_handle][handle] = memory_allocation
  device_desc = state.to_struct(device_desc_val, ZE::ZEDeviceMemAllocDesc)
  check_struct_stype_misuse(state,ctx,payload,:ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC, device_desc[:stype])
}

# Allocs shared memory
$on_successful_exit['zeMemAllocShared'] = lambda { |state, ctx, payload|
  ctx_handle = state.find_param(ctx, 'hContext')
  memory_allocations = state.memory_allocations(ctx, ctx_handle)
  # finds the device and context objects associated with the params
  context = state.find_object(ctx, 'context', 'hContext')
  device = state.find_object(ctx, 'device','hDevice')
  # A nullptr device handle shares ownership between the host and all devices
  # supporting cross-device shared access.
  size = state.find_param(ctx,"size")
  handle = payload['pptr_val']
  mark_reallocated(state, ctx, ctx_handle, handle, size)
  memory_allocation =  ZEModel::Memory.new(handle, context, size, device)
  memory_allocations[handle] = memory_allocation
  device.memory_allocations[ctx_handle][handle] = memory_allocation if device
}

# Allocs Host memory
$on_successful_exit['zeMemAllocHost'] = lambda { |state, ctx, payload|
  # Host allocations are accessible by the host and all devices within the driver’s context.
  ctx_handle = state.find_param(ctx, 'hContext')
  memory_allocations = state.memory_allocations(ctx, ctx_handle)
  context = state.find_object(ctx, 'context', 'hContext')
  size = state.find_param(ctx,"size")
  handle = payload['pptr_val']
  mark_reallocated(state, ctx, ctx_handle, handle, size)
  memory_allocation =  ZEModel::Memory.new(handle, context, size, nil, "host")
  memory_allocations[handle] = memory_allocation
}

# The free is applied at entry: zeMemFree may block until the buffer is idle, so
# by _exit a gated copy could have drained and the in-flight check would miss it.
$upon_entry['zeMemFree'] = lambda { |state, ctx, payload|
  ctx_handle = payload['hContext']
  memory_allocations = state.memory_allocations(ctx, ctx_handle)
  handle = payload['ptr']
  memory_allocation = memory_allocations[handle]
  next unless memory_allocation
  # flag if this buffer is still referenced by a copy/fill that has been
  # submitted but not yet completed (in-flight device work would touch freed mem)
  check_free_in_flight(state, ctx, memory_allocation)
  memory_allocations.delete(handle)
  owned = memory_allocation.owned_by
  owned.memory_allocations[ctx_handle].delete(handle) if owned
  # keep the freed allocation in this context's freed registry so a later
  # copy/fill/kernel referencing this address is caught as use-after-free
  memory_allocation.freed_by = state.get_api_context(ctx)
  state.freed_memory_allocations(ctx, ctx_handle)[handle] = memory_allocation
}

# the free was applied at entry, so restore the allocation if it actually failed
$on_erroneous_exit['zeMemFree'] = lambda { |state, ctx, payload|
  ctx_handle = state.find_param(ctx, 'hContext')
  handle = state.find_param(ctx, "ptr")
  mem = state.freed_memory_allocations(ctx, ctx_handle).delete(handle)
  if mem
    mem.freed_by = nil
    state.memory_allocations(ctx, ctx_handle)[handle] = mem
    owned = mem.owned_by
    owned.memory_allocations[ctx_handle][handle] = mem if owned
  end
}