require 'babeltrace2'
require 'ze_library'
require 'set'
require 'ze_validator_zemodel'
require 'ze_validator_function_entry_exit_callbacks'
require 'ze_validator_state_object'
require 'yaml'
require 'json'

class StateObject
  attr_reader :state
  attr_reader :ze_thread_safety
  attr_reader :lock_shared_object_on_entry
  attr_reader :unlock_shared_object_on_exit
  attr_accessor :print_tracker
  attr_accessor :device_agnostic
  attr_accessor :performance
  attr_accessor :memory_in_transit
  attr_reader :device_properties

  def initialize(**opts)
    @deprecated = JSON.parse(File.read(File.join(DATADIR, 'ze_deprecated.json')))
    @device_properties = load_device_properties
    #for supressing redundant error outputs
    @print_tracker = Hash.new { |h, k| h[k] = 0 }
    # Append a third slot to every entry: "has this warning been printed?".
    @deprecated.each do |api, (version, replacement)|
      @deprecated[api] = [version, replacement, false]
    end
    @performance = opts[:performance]
    @device_agnostic = opts[:device_agnostic]
    @state = Hash.new { |h, k| h[k] = ZEModel::Node.new(k) }
    @ze_thread_safety = YAML::load_file(File.join(DATADIR, 'ze_thread_safety.yaml'))
    @lock_shared_object_on_entry = Hash.new { |h, k| h[k] = [] }
    @unlock_shared_object_on_exit = Hash.new { |h, k| h[k] = [] }
    @init_called = Hash.new { |h, k| h[k] = false } #pid : init called status
    @memory_in_transit = Hash.new {|h,k| h[k] = []} #pid : [[mem, (src|dst)]] list of memories being transferred
    @printed_init_error = false
    @deferred_units = []
    @signal_epoch = 0
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

  # Returns the parsed ze_device_property.json, or nil if it is missing or
  # unparseable so the validator degrades gracefully.
  def load_device_properties
    path = File.join(DATADIR, 'ze_device_property.json')
    return nil unless File.file?(path)
    JSON.parse(File.read(path))
  rescue JSON::ParserError => e
    $stderr.puts "Warning: could not parse #{path}: #{e.message}"
    nil
  end

  # Returns a map e.g.,{"ordinal"=>1, "type"=>"copy", "numQueues"=>8} or nil. Without a
  # device index it uses the first device.
  def command_queue_group(ordinal, device_index: nil)
    return nil unless @device_properties
    devices = @device_properties['devices'] || []
    devices = devices.select { |d| d['device_index'] == device_index } if device_index
    devices.each do |dev|
      group = (dev['command_queue_groups'] || []).find { |g| g['ordinal'] == ordinal }
      return group if group
    end
    nil
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

  # Zero-padded so the same object reads identically everywhere, which also
  # makes these strings safe as print_tracker dedup keys.
  def get_handle_str(handle)
    '0x%016x' % handle
  end

  # "hostname - pid", for process-wide findings such as leaks and deadlocks.
  def get_proc_context_str(context)
    "#{context['hostname']} - #{context['vpid']}"
  end

  # "tid in zeSomeApi": identifies the specific call.
  def get_api_context(context)
    "#{context['vtid']} in #{context['api']}"
  end

  # "hostname - pid - tid in zeSomeApi", for findings attributable to one call.
  def get_context_str(context)
    "#{get_proc_context_str(context)} - #{get_api_context(context)}"
  end

  # Warns once per deprecated API actually used.
  def print_deprecation_warning(old_api)
    if @deprecated.include?(old_api) and @deprecated[old_api][2]
      deprecated_since = @deprecated[old_api][0]
      new_api = @deprecated[old_api][1]
      if deprecated_since == ""
        puts "#{old_api} is deprecated. Please use #{new_api} instead."
      else
        puts "#{old_api} is deprecated since #{deprecated_since}. Please use #{new_api} instead."
      end
    end
  end

  def print_portability_error(context,str)
    $stderr.puts "Level Zero Portability Error: on #{get_context_str(context)}: #{str}\n\n"
  end
  def print_performance_issue(context,str)
     $stderr.puts "Level Zero Performance Issue: on #{get_context_str(context)}: #{str}\n\n"
  end
  def print_usage_error(context, str)
    $stderr.puts "Level Zero Usage Error: on #{get_context_str(context)}: #{str}\n\n"
  end

  def print_crash_error(context, str)
    $stderr.puts "Level Zero Crash Error: on #{get_context_str(context)}: #{str}\n\n"
  end

  def print_memory_error(context, str)
    $stderr.puts "Level Zero Memory Error: on #{get_context_str(context)}: #{str}\n\n"
  end

  def print_deadlock_error(context, str)
    $stderr.puts "Level Zero Deadlock: on #{get_proc_context_str(context)}: #{str}\n\n"
  end


  def print_leak_error(context, type, handle, memtypestr="")
    if memtypestr.empty?
      $stderr.puts "Level Zero Leak: on #{get_proc_context_str(context)}: #{type} #{get_handle_str(handle)}\n\n"
    else
      $stderr.puts "Level Zero Leak #{memtypestr}-memory: on #{get_proc_context_str(context)}: #{type} #{get_handle_str(handle)}\n\n"
    end
  end

  # Not a finding about the traced program: the validator's own bookkeeping is
  # wrong, so further output would be untrustworthy.
  def raise_internal_error(context, str)
    raise "Invalid state #{get_context_str(context)}: #{str}"
  end

  # Deduped per (object, other holder) so a racing loop reports once.
  def print_race_condition(context, other_context, type, handle)
    if @print_tracker["#{type}-#{get_handle_str(handle)}-#{get_api_context(other_context)}"] == 0
      @print_tracker["#{type}-#{get_handle_str(handle)}-#{get_api_context(other_context)}"] = 1
      print_usage_error(context, "concurrent acces to #{type} #{get_handle_str(handle)}, already held by #{get_api_context(other_context)}")
    end
  end

  # Passed as the block to Hash#delete, so it fires when a destroy names a
  # handle the model never recorded.
  def object_not_found(context, type, handle, sub_context = nil)
    raise_internal_error(context, "event_pool #{get_handle_str(handle)} not found#{sub_context ? " in #{sub_context}" : ""}")
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

  # The live allocations of one Level Zero context (address -> Memory).
  def memory_allocations(context, ctx_handle)
    get_process(context).memory_allocations[ctx_handle]
  end

  # The freed allocations of one Level Zero context, for use-after-free checks.
  def freed_memory_allocations(context, ctx_handle)
    get_process(context).freed_memory_allocations[ctx_handle]
  end

  # Yields [unit, op] for every copy op still pending in this process, so a free
  # can tell whether the buffer is still referenced by submitted work.
  def each_inflight_copy_op(context)
    @deferred_units.each do |unit|
      next unless unit.context['hostname'] == context['hostname'] &&
                  unit.context['vpid'] == context['vpid']
      unit.ops[unit.cursor..].each do |op|
        next unless op && op.kind == :copy
        yield unit, op
      end
    end
  end

  # True if a prior submission of this command list has not drained yet.
  def command_list_in_flight?(context, handle)
    @deferred_units.any? do |unit|
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

  # Signals an event and notes progress so pump_deferred sweeps again.
  def signal_event(context, handle, by = nil)
    ev = event_by_handle(context, handle)
    if ev
      ev.signal(by)
      @signal_epoch += 1
    end
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
    return true if waits.nil? || waits.empty?
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
  def run_deferred_list(context, ops, label, in_order: false, cmd_list_handle: nil)
    @deferred_units << ZEModel::DeferredUnit.new(ops, context, label, in_order: in_order,
                                                 cmd_list_handle: cmd_list_handle)
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

  # Immediate lists execute each op as it is appended, but still go through the
  # same machinery so they get the same checks.
  def enqueue_immediate_op(context, op, handle = nil)
    label = handle ? "immediate command list (#{get_handle_str(handle)})" \
                   : 'immediate command list'
    run_deferred_list(context, [op], label)
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


  # Checks for the issues only visible at end of trace: deadlocks, calls that
  # never returned, and objects that were never destroyed.
  def check_issues()
    #drain deferred command-list executions before reporting leaks/crashes
    flush_deferred
    crash = false
    @state.each { |hostname, node|
      node.processes.each { |pid, process|
        process.threads.each { |tid, thread|
          #any frame still on the stack is a call that never returned
          thread.call_stack.each { |frame|
            ctx = {'hostname' => hostname, 'vpid'=> pid, 'vtid' => tid, 'api' => frame.name}
            print_crash_error(ctx, 'command did not finish execution')
            crash = true
          }
        }
      }
    }

    unless crash && false
      @state.each { |hostname, node|
        node.processes.each { |pid, process|
          ctx = {'hostname' => hostname, 'vpid'=> pid}
          [ 'context',
            'event_pool',
            'command_queue',
            'fence',
            'command_list',
            'module',
            'module_build_log',
            'kernel',
          ].each { |t|
            process.objects(t).each { |h, c|
              print_leak_error(ctx, t, h)
            }
          }
          process.objects('memory_allocation').each { |_ctx_handle, allocs|
            allocs.each { |h, c|
              print_leak_error(ctx, 'memory_allocation', h, c.instance_variable_get(:@memtypestr))
            }
          }
        }
      }
    end
  end


  # Checks that zeInit or zeInitDrivers came before any other API call. Keyed by
  # pid, and reported once per run.
  def check_initialization(context)
	if ZEModel::INIT_API_NAMES.include?(context['api'])
        @init_called[context['vpid']] = true
    end

	if !@init_called[context['vpid']] && !@printed_init_error
		self.print_usage_error(context, "zeInit or zeDriversInit wasn't called before #{context['api']}")
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
    #modifies the satate based on entry fields. Needed because some fields are easier to access it from the entry
    l = $upon_entry[m[1]]
    l.call(self,context,payload) if l
  end

  def on_exit(m,hostname,context,payload)
    #unlock the shared object if the api name matches the predefined in ze_thread_safety.yaml
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
          if m
            hostname = e.stream.trace.get_environment_entry_value_by_name('hostname').value
            context = e.get_common_context_field.value
            #the event's own fields: input args at _entry, results at _exit
            payload = e.payload_field.value
            context['hostname'] = hostname
            context['api'] = m[1]
			      #zeDriversInit or zeInit must be the first one to be called before any api calls
            check_initialization(context)
            print_deprecation_warning(m[1]) if @deprecated[m[1]]

            if m[2] == 'entry'
              on_entry(m, hostname, context, payload)
            elsif m[2] == 'exit'
              on_exit(m, hostname, context, payload)
            end
            # Runs the blocked commands, if the wait-event(s) are satisfied
            pump_deferred 
          end
        end
      }

end
