require 'babeltrace2'
require 'ze_library'
require 'set'
require 'ze/validator/errors'
require 'ze/validator/model'
require 'ze/validator/callbacks'
require 'yaml'

class StateObject
  attr_reader :state
  attr_reader :lock_shared_object_on_entry
  attr_reader :unlock_shared_object_on_exit
  attr_reader :device_properties
  attr_reader :reporter

  def initialize(**opts)
    metadata = YAML.load_file(File.join(DATADIR, 'ze', 'validator', 'api_metadata.yaml'))
    @deprecated = metadata.fetch('deprecated')
    @device_properties = Hash.new { |h, k| h[k] = {} }
    @reporter = ZEValidator::Reporter.new

    # fullpath to a .csv file for exporting detected errors on 'finalize()'
    @csv_export = opts[:csv_export]

    # end-of-trace leak sweep, on unless --no-report-leaks turned it off
    @report_leaks = opts.fetch(:report_leaks, true)

    # APIs whose deprecation warning has already been printed.
    @deprecation_warned = Set.new

    @state = Hash.new { |h, k| h[k] = ZEModel::Node.new(k) }
    @ze_thread_safety = metadata.fetch('thread_unsafe')
    @lock_shared_object_on_entry = Hash.new { |h, k| h[k] = [] }
    @unlock_shared_object_on_exit = Hash.new { |h, k| h[k] = [] }
    @init_called = Hash.new { |h, k| h[k] = false } #pid : init called status
    @printed_init_error = false
    @deferred_units = []
    @ze_thread_safety.each { |api, objects|
      objects.each { |o|
        @lock_shared_object_on_entry[api].push( lambda { |state, ctx, payload|
                                    #at entry the input args are in payload directly
                                    handle = payload[o.first]
                                    if handle.kind_of? Array
                                      handle.each { |h|
                                        obj = state.find_object(ctx, o.last, h)
                                        obj.lock(state, ctx) if obj
                                      }
                                    else
                                      obj = state.find_object(ctx, o.last, handle)
                                      obj.lock(state, ctx) if obj
                                    end
                                  })
        @unlock_shared_object_on_exit[api].push( lambda { |state, ctx, payload|
                                   #at exit payload holds only outputs, so the input
                                   #handle comes from the saved entry payload
                                   handle = state.find_param(ctx, o.first)
                                   if handle.kind_of? Array
                                     handle.each { |h|
                                       obj = state.find_object(ctx, o.last, h)
                                       obj.unlock(ctx) if obj
                                     }
                                   else
                                     obj = state.find_object(ctx, o.last, handle)
                                     obj.unlock(ctx) if obj
                                   end
                                 })
      }
    }

  end



  # The innermost API call currently executing on this thread, or nil.
  def get_last_entry(context)
    @state[context['hostname']].processes[context['vpid']].threads[context['vtid']].last_entry
  end

  def get_thread(context)
    @state[context['hostname']].processes[context['vpid']].threads[context['vtid']]
  end

  def get_process(context)
    @state[context['hostname']].processes[context['vpid']]
  end

  # Checks that the call we return from is on top of this thread's stack. A
  # mismatch means the model lost sync with the trace, so it aborts.
  def check_last_entry(context)
    last_entry = get_last_entry(context)
    unless last_entry && last_entry.name == context['api']
      raise "Invalid State in #{context['api']}"
    end
  end


  # Pushes a call frame, so a traced API calling another traced API on the same
  # thread nests correctly.
  def set_last_entry(state, context, payload)
    get_thread(context).call_stack.push(ZEModel::ApiCall.new(context['api'], payload))
  end

  # Pops the innermost frame on return, exposing the caller's frame.
  def reset_last_entry(context)
    get_thread(context).call_stack.pop
  end

  # Decides whether on_exit runs the success or the error callback.
  def validate_result(payload)
    ZE::ZEResult.from_native(payload["zeResult"], nil) == :ZE_RESULT_SUCCESS
  end

  def get_handle_str(handle)
    '0x%016x' % handle
  end

  def get_proc_context_str(context)
    "#{context['hostname']}:#{context['vpid']}"
  end

  def get_api_context(context)
    "#{context['vtid']} in #{context['api']}"
  end

  def get_context_str(context)
    "#{get_proc_context_str(context)}:#{context['vtid']}"
  end

  # Reports a finding attributable to one API call. `id` names a leaf of the
  # diagnostic tree (see errors.rb); `key`, when given,
  # suppresses verbatim repeats about the same object.
  def report(id, context, str, key: nil)
    @reporter.report(id, [context['hostname'], context['vpid'], context['vtid']],
                     "in #{context['api']}: #{str}", key: key)
  end

  # Reports a process-wide finding, which has no thread or API to attribute to.
  def report_proc(id, context, str, key: nil)
    @reporter.report(id, [context['hostname'], context['vpid']], str, key: key)
  end

  # Warns once per deprecated API actually used.
  def print_deprecation_warning(context, old_api)
    return unless @deprecation_warned.add?(old_api)

    deprecated_since, new_api = @deprecated[old_api]
    since = deprecated_since.to_s.empty? ? '' : " since #{deprecated_since}"
    report(:deprecated_api, context, "#{old_api} is deprecated#{since}. Please use #{new_api} instead.")
  end

  # Reports a leaked object, naming what it still held. The objects inside it
  # are not reported on their own, so the count is where the detail went.
  def print_leak_error(context, type, obj)
    report_proc(:object_leak, context,
                "#{type} #{get_handle_str(obj.handle)} was never destroyed#{held_summary(obj)}")
  end

  # " (still held 2 command_lists, 5 memory_allocations)", or "" for an object
  # that owns nothing or had nothing left in it.
  def held_summary(obj)
    held = ZEModel::OWNED_TYPES.fetch(obj.class.typename, []).filter_map { |type|
      count = obj.child_count(type)
      "#{count} #{type}#{'s' if count > 1}" if count.positive?
    }
    held.empty? ? '' : " (still held #{held.join(', ')})"
  end

  # How an object reads on a report line. Memory is a range rather than a
  # handle, and carries the kind the allocator gave it.
  def object_label(type, obj)
    return "#{obj.memtypestr}-memory #{get_handle_str(obj.base)}" if type == 'memory_allocation'

    "#{type} #{get_handle_str(obj.handle)}"
  end

  # Not a finding about the traced program: the validator's own bookkeeping is
  # wrong, so further output would be untrustworthy.
  def raise_internal_error(context, str)
    raise "Invalid state #{get_context_str(context)} in #{context['api']}: #{str}"
  end

  # Deduped per (object, other holder) so a racing loop reports once.
  def print_race_condition(context, other_context, type, handle)
    report(:concurrent_object_access, context,
           "concurrent access to #{type} #{get_handle_str(handle)}, already held by #{get_api_context(other_context)}",
           key: "#{type}-#{get_handle_str(handle)}-#{get_api_context(other_context)}")
  end

  # Passed as the block to Hash#delete, so it fires when a destroy names a
  # handle the model never recorded.
  def object_not_found(context, type, handle, sub_context = nil)
    raise_internal_error(context, "#{type} #{get_handle_str(handle)} not found#{sub_context ? " in #{sub_context}" : ""}")
  end

  # Reads one input argument of the call executing on this thread. Works at
  # _exit too, since the entry payload is still on the call stack.
  def find_param(context, name)
    get_last_entry(context).params[name]
  end

  # The whole handle -> object table for a type.
  def find_objects(context, type)
    get_process(context).instance_variable_get("@#{type}s")
  end

  # `handle` may be the handle itself or the name of the param carrying it.
  def find_object(context, type, handle)
    handle = find_param(context, handle) if handle.kind_of? String
    find_objects(context, type)[handle]
  end

  # AllocationMap for living allocations on a Context
  def memory_allocations(context, ctx_handle)
    get_process(context).contexts[ctx_handle]&.memory_allocations
  end

  # AllocationMap for freed allocations on a Context (for use_after_free detections)
  def freed_memory_allocations(context, ctx_handle)
    get_process(context).contexts[ctx_handle]&.freed_memory_allocations
  end

  # Get a Context of a Process from handles
  def context_from_handle(context, ctx_handle)
    ctx_obj = get_process(context).contexts[ctx_handle]
    return ctx_obj if ctx_obj

    report(:unknown_context, context,
           "context #{get_handle_str(ctx_handle)} was never created, or was already destroyed; " \
           'its allocations cannot be tracked',
           key: "unknown-context-#{get_handle_str(ctx_handle)}")
    nil
  end

  # Yields [unit, op] for every copy op still pending in this process
  def each_inflight_copy_op(context)
    @deferred_units.each do |unit|
      next unless unit.context['hostname'] == context['hostname'] &&
                  unit.context['vpid'] == context['vpid']
      unit.ops[unit.cursor..].each do |op|
        next unless op.kind == :copy
        yield unit, op
      end
    end
  end

  # True if a prior submission of this command list has not drained yet.
  def command_list_in_flight?(context, handle)
    @deferred_units.any? do |unit|
      !unit.immediate &&
        unit.cmd_list_handle == handle &&
        unit.context['hostname'] == context['hostname'] &&
        unit.context['vpid'] == context['vpid'] &&
        !unit.done?
    end
  end

  # Decodes a raw descriptor blob from the trace into a typed FFI struct, or nil
  # for a null descriptor.
  def to_struct(memory, klass)
    memory.size > 0 ? klass.new(FFI::MemoryPointer.from_string(memory)) : nil
  end

  # Returns the Event for a handle, nil for a null or unknown one.
  def event_by_handle(context, handle)
    return nil if handle.nil? || handle == 0
    find_objects(context, 'event')[handle]
  end

  # Signals an event, if the handle names one we track.
  def signal_event(context, handle, by = nil)
    ev = event_by_handle(context, handle)
    ev&.signal(by)
    ev
  end

  # Reset's the given handle's event
  def reset_event(context, handle)
    event_by_handle(context, handle)&.reset
  end

  # Records that the host observed an event's signaled state.
  def observe_event(context, handle)
    event_by_handle(context, handle)&.observe
  end

  # A device-wide host synchronization means every signaled event was consumed.
  def observe_all_signaled_events(context)
    find_objects(context, 'event').each_value { |ev| ev.observe if ev.signaled }
  end

  # True once every wait handle is signaled. Untracked handles count as
  # satisfied, so we never invent a deadlock for one.
  def waits_satisfied?(context, waits)
    waits.all? { |h| ev = event_by_handle(context, h); ev.nil? || ev.signaled }
  end

  # Runs the op the cursor points at, applying its deferred checks and signal.
  def run_deferred_op(unit)
    context = unit.context
    op = unit.current_op
    if op.kind == :copy
      check_oob_copy(self, context, op.params)
      #a pointer freed before this copy's turn to execute is a use-after-free
      check_use_after_free(self, context, op.params)
    end
    #a memory-ranges barrier references memory freed before its turn is a UAF
    check_uaf_ranges_barrier(self, context, op.params) if op.kind == :ranges_barrier
    #a reset takes effect before this op signals its own completion event
    reset_event(context, op.params[:reset_handle]) if op.kind == :reset
    signaled = false
    if op.signal
      #the completion event must be unsignaled here: reuse without an intervening
      #reset (or a concurrent double-signal) is a misuse
      check_event_signal_reuse(self, context, op.signal, op.api || 'a command list append')
      signal_event(context, op.signal, op.api)
      unit.pending_signals.delete(op.signal)
      signaled = true
    end
    unit.cursor += 1
    unit.blocked_on = []
    signaled
  end

  # Advances every unit as far as its wait-events allow, sweeping until a whole
  # pass makes no progress since one unit's signal can unblock another.
  def pump_deferred
    progress = true
    while progress
      progress = false
      @deferred_units.each do |unit|
        until unit.done?
          op = unit.current_op
          if waits_satisfied?(unit.context, op.waits)
            run_deferred_op(unit)
            progress = true
          else
            #park the unit on this op and record what it is blocked on so the
            #deadlock detector can see the wait-for edges
            unit.blocked_on = op.waits.reject { |h|
              ev = event_by_handle(unit.context, h); ev.nil? || ev.signaled
            }
            break
          end
        end
      end
      @deferred_units.reject!(&:done?)
    end
  end

  # Registers a command list's ops as a deferred unit and pumps.
  def run_deferred_list(context, ops, label, in_order: false, cmd_list_handle: nil, immediate: false)
    @deferred_units << ZEModel::DeferredUnit.new(ops, context, label, in_order: in_order,
                                                 cmd_list_handle: cmd_list_handle,
                                                 immediate: immediate)
    pump_deferred
  end

  # Each submitted list becomes its own unit: lists in one submit are ordered by
  # events, not by list order, so a cycle between two of them is a real deadlock.
  def enqueue_deferred_execution(context, command_lists)
    command_lists.each do |cl|
      next unless cl
      run_deferred_list(context, cl.ops.dup, "command_list (#{get_handle_str(cl.handle)})",
                        in_order: cl.in_order, cmd_list_handle: cl.handle)
    end
  end

  # The unit of an in-order immediate list that still has ops to run, if any.
  def open_immediate_unit(context, handle)
    @deferred_units.find do |unit|
      unit.immediate && unit.cmd_list_handle == handle &&
        unit.context['hostname'] == context['hostname'] &&
        unit.context['vpid'] == context['vpid'] &&
        !unit.done?
    end
  end

  # Immediate lists execute each op as it is appended, but still go through the
  # same machinery so they get the same checks.
  def enqueue_immediate_op(context, op, handle = nil, in_order: false)
    unit = in_order && handle ? open_immediate_unit(context, handle) : nil
    if unit
      unit.push_op(op)
      pump_deferred
      return
    end

    label = handle ? "immediate command list (#{get_handle_str(handle)})" \
                   : 'immediate command list'
    run_deferred_list(context, [op], label, in_order: in_order,
                                            cmd_list_handle: handle, immediate: true)
  end

  # End-of-trace drain: reports deadlocks among whatever is still stuck, then
  # forces each remaining op so its deferred checks run against the final state.
  def flush_deferred
    pump_deferred
    return if @deferred_units.empty?
    check_circular_deadlock(self, @deferred_units)
    check_in_order_self_deadlock(self, @deferred_units)
    until @deferred_units.empty?
      unit = @deferred_units.first
      #force the op the unit is stuck on: report its unsignaled waits, then run it
      report_unsignaled_waits(self, unit.context, unit.current_op.waits) if unit.current_op
      run_deferred_op(unit) unless unit.done?
      @deferred_units.reject!(&:done?)
      #a forced completion may unblock others cleanly
      pump_deferred
    end
  end


  # Checks for the issues only visible at end of trace
  # Notify that all traces had been processed to:
  #   - perform final checks, that can only be performed after the entire
  #   traced got read (e.g., deadlocks, leaks, etc.)
  #   - export to csv if asked by users
  def finalize()
    flush_deferred
    report_unfinished_calls
    report_leaks if @report_leaks
    @reporter.summary
    @reporter.export_csv(@csv_export) if @csv_export
  end

  # Any frame still on a thread's call stack is a call that never returned.
  def report_unfinished_calls
    @state.each { |hostname, node|
      node.processes.each { |pid, process|
        process.threads.each { |tid, thread|
          thread.call_stack.each { |frame|
            ctx = {'hostname' => hostname, 'vpid'=> pid, 'vtid' => tid, 'api' => frame.name}
            report(:api_never_returned, ctx, 'call did not finish execution')
          }
        }
      }
    }
  end

  # Objects still registered once the trace is exhausted were never destroyed.
  LEAKABLE_OBJECT_TYPES = %w[context event_pool command_queue fence command_list
                             module module_build_log kernel].freeze

  # Reports what a container still held when it was destroyed, and forgets it.
  def report_orphans(context, parent)
    parent_label = "#{parent.class.typename} #{get_handle_str(parent.handle)}"
    ZEModel::OWNED_TYPES.fetch(parent.class.typename, []).each { |type|
      verb = type == 'memory_allocation' ? 'freed' : 'destroyed'
      parent.each_child(type) { |child|
        report(:object_outlives_owner, context,
               "#{object_label(type, child)} was not #{verb} prior to #{parent_label} destruction" \
               "#{held_summary(child)}",
               key: "outlives-#{type}-#{get_handle_str(child.handle)}")
        mark_leak_reported(type, child)
      }
    }
  end

  # Marks an object and everything below it as already reported, so the end of
  # the trace does not name them a second time.
  def mark_leak_reported(type, obj)
    obj.leak_reported = true
    ZEModel::OWNED_TYPES.fetch(type, []).each { |child_type|
      obj.each_child(child_type) { |child| mark_leak_reported(child_type, child) }
    }
  end

  # True when this object is the outermost leaked object of its subtree, and so
  # the one worth a report.
  def leak_root?(process, type, obj)
    return false if obj.leak_reported

    owner_type = ZEModel::OWNERSHIP[type]
    return true unless owner_type && LEAKABLE_OBJECT_TYPES.include?(owner_type)

    owner = obj.owner
    owner.nil? || !process.objects(owner_type).key?(owner.handle)
  end

  def report_leaks
    @state.each { |hostname, node|
      node.processes.each { |pid, process|
        ctx = {'hostname' => hostname, 'vpid'=> pid}
        LEAKABLE_OBJECT_TYPES.each { |t|
          process.objects(t).each_value { |obj|
            print_leak_error(ctx, t, obj) if leak_root?(process, t, obj)
          }
        }
      }
    }
  end


  # Checks that zeInit or zeInitDrivers came before any other API call. Keyed by
  # pid, and reported once per run.
  def check_initialization(context)
	if ZEModel::INIT_API_NAMES.include?(context['api'])
        @init_called[context['vpid']] = true
    end

	if !@init_called[context['vpid']] && !@printed_init_error
		report(:init_not_called, context, "zeInit or zeInitDrivers wasn't called before #{context['api']}")
		@printed_init_error = true
	end
  end


  # Pushes the call frame, takes the thread-safety locks and runs the API's
  # entry callback. Checks run here when the call itself might crash.
  def on_entry(m,hostname, context,payload)
    set_last_entry(self, context, payload) #sets the per-thread callstack of the APIs
      @lock_shared_object_on_entry[m[1]].each { |l|
                  l.call(self, context, payload)
      }
    #modifies the state based on entry fields. Needed because some fields are easier to access it from the entry
    l = $upon_entry[m[1]]
    l.call(self,context,payload) if l
  end

  def on_exit(m,hostname,context,payload)
    #unlock the shared object if the api name matches the predefined in api_metadata.yaml
    @unlock_shared_object_on_exit[m[1]].reverse_each { |l|
      l.call(self, context, payload)
    }

    #check if the return code indicates successful return from the API call
    if validate_result(payload)
      l = $on_successful_exit[m[1]] #This might be a problem for tracking erroneous exits.
      l.call(self, context, payload) if l
    else
      l = $on_erroneous_exit[m[1]]
      l.call(self, context, payload) if l
    end

    check_last_entry(context)  #When we return from _exit, we need to see what we saw in _entry for the current thread_id
    reset_last_entry(context)  #Reset the callstack for current thread_id
  end

  # The main loop: returns the lambda called with each batch of decoded
  # messages.
  def consume = lambda { |iterator, _|
        iterator.next_messages.each do |m|
          next unless m.type == :BT_MESSAGE_TYPE_EVENT
          e = m.event
          #splits "lttng_ust_ze:zeMemAllocDevice_entry" into the API name (m[1])
          #and the phase (m[2]); anything else is ignored
          m = e.name.match(/:(z.*)_(entry|exit)/)
          device_property = e.name.match(/lttng_ust_ze_properties:command_queue_group/) 
          if m
            hostname = e.stream.trace.get_environment_entry_value_by_name('hostname').value
            context = e.get_common_context_field.value
            #the event's own fields: input args at _entry, results at _exit
            payload = e.payload_field.value
            context['hostname'] = hostname
            context['api'] = m[1]
			      #zeDriversInit or zeInit must be the first one to be called before any api calls
            check_initialization(context)
            print_deprecation_warning(context, m[1]) if @deprecated[m[1]]

            if m[2] == 'entry'
              on_entry(m, hostname, context, payload)
            elsif m[2] == 'exit'
              on_exit(m, hostname, context, payload)
            end
            # Runs the blocked commands, if the wait-event(s) are satisfied
            pump_deferred 
          elsif device_property
            payload = e.payload_field.value
            count = payload["pCount"]
            device = payload["hDevice"]
            blob  = payload["pGroupProperties_vals"]
            ptr   = FFI::MemoryPointer.from_string(blob)
            size  = ZE::ZECommandQueueGroupProperties.size 
            groups = (0...count).map do |i| ZE::ZECommandQueueGroupProperties.new(ptr + i * size) end

            groups.each_with_index do |g, ordinal|
              @device_properties[device][ordinal] = {}
              @device_properties[device][ordinal]['flags'] = g[:flags]
              @device_properties[device][ordinal]['numQueues'] = g[:numQueues]
            end

          end
        end
      }

end
