require 'babeltrace2'
require 'ze_library'
require 'set'
require 'ze_validator_zemodel'
require 'ze_validator_function_entry_exit_callbacks'
require 'ze_validator_state_object'
require 'yaml'
require 'json'

# =============================================================================
# StateObject -- the validator's engine.
#
# One instance exists for an entire run. It plays three roles:
#
#   1. THE BABELTRACE SINK. #consume returns the lambda the trace graph calls
#      with each batch of decoded events. That lambda is the entry point for
#      everything below.
#
#   2. THE MODEL OWNER. @state holds the whole ZEModel tree
#      (hostname -> Node -> pid -> Process -> objects). Every find_* / get_*
#      helper here is a path into that tree, and the callbacks in
#      ze_validator_function_entry_exit_callbacks.rb reach the model only
#      through these methods.
#
#   3. THE REPORTER AND SCHEDULER. All print_* methods (the only place
#      diagnostics are emitted) live here, as does the deferred-execution
#      scheduler that replays asynchronous GPU work at the right moment.
#
# THE LIFE OF ONE TRACE EVENT
# ---------------------------
#   consume
#     -> match name against /:(z.*)_(entry|exit)/ -> api name + entry|exit
#     -> build `context` = {hostname, vpid, vtid, api}, the "who/where" tuple
#        threaded through literally every method in the validator
#     -> check_initialization  (was zeInit called first?)
#     -> print_deprecation_warning
#     -> on_entry / on_exit
#          - lock/unlock shared objects for the thread-safety check
#          - run the API's callback
#          - push/pop the thread's call stack
#     -> pump_deferred (a signal may have unblocked pending GPU work)
#   ...and once the trace is exhausted, ze_validator.in calls #check_issues.
#
# TWO VOCABULARY NOTES THAT UNLOCK THE REST OF THE CODE
# -----------------------------------------------------
#   `context` (lowercase, often `ctx`) -- the TRACE context hash above. It says
#       which host/process/thread/API is currently executing. It is NOT a
#       Level Zero context.
#   `ctx_handle` / ZEModel::Context -- the LEVEL ZERO context, the isolation
#       domain objects belong to.
#   The two are unrelated and appear side by side constantly; keeping them
#       distinct is the single biggest hurdle when reading this file.
#
#   `defi` -- the decoded event payload ("definition"). At _entry it holds the
#       call's input arguments; at _exit it holds the return code and output
#       values. This asymmetry is why exit callbacks call find_param (which
#       reads the saved _entry payload) to get at inputs.
# =============================================================================
class StateObject
  # hostname -> ZEModel::Node. The root of the entire model.
  attr_reader :state
  # Parsed ze_thread_safety.yaml: api name -> objects that must not be touched
  # concurrently during that call.
  attr_reader :ze_thread_safety
  # Callback lists built from the YAML above, run around every traced call.
  attr_reader :lock_shared_object_on_entry
  attr_reader :unlock_shared_object_on_exit
  # DEDUPLICATION TABLE. A GPU program is a loop: the same mistake in the same
  # line will recur thousands of times and would bury the report. Checks build a
  # key describing the specific violation, print only when its counter is 0, and
  # then set it to 1. Seeing `state.print_tracker[key] == 0` guarding a
  # print_usage_error is this idiom, and it is used all over the check library.
  attr_accessor :print_tracker
  attr_accessor :device_agnostic  # --disable-dagn turns this off
  attr_accessor :performance      # --disable-performance turns this off
  attr_accessor :memory_in_transit
  # Real command-queue-group topology of this machine, or nil when unavailable.
  attr_reader :device_properties
  def initialize(**opts)
    # api name -> [deprecated_since_version, replacement_api]. Loaded from a
    # data file so new deprecations need no code change.
    @deprecated = JSON.parse(File.read(File.join(DATADIR, 'ze_deprecated.json')))
    # Per-device command queue group topology (ordinal -> engine type + numQueues),
    # generated on a real device by the ze_device_property helper binary and
    # installed alongside the other data files. Loaded if present; validation that
    # does not depend on it still runs when the file is absent.
    @device_properties = load_device_properties

    #for supressing redundant error outputs
    # Any key not yet seen reads as 0, so checks can test-and-set without
    # initializing keys first.
    @print_tracker = Hash.new { |h, k| h[k] = 0 }

    # Append a third slot to every entry: "has this warning been printed?".
    @deprecated.each do |api, (version, replacement)|
      @deprecated[api] = [version, replacement, false]
    end
    @performance = opts[:performance]
    @device_agnostic = opts[:device_agnostic]
    # The model root. Nodes, and in turn processes and threads, spring into
    # existence the first time an event mentions them -- so no discovery pass is
    # needed before consuming the trace.
    @state = Hash.new { |h, k| h[k] = ZEModel::Node.new(k) }
    @ze_thread_safety = YAML::load_file(File.join(DATADIR, 'ze_thread_safety.yaml'))
    @lock_shared_object_on_entry = Hash.new { |h, k| h[k] = [] }
    @unlock_shared_object_on_exit = Hash.new { |h, k| h[k] = [] }
    @init_called = Hash.new { |h, k| h[k] = false } #pid : init called status
    @memory_in_transit = Hash.new {|h,k| h[k] = []} #pid : [[mem, (src|dst)]] list of memories being transferred
    @printed_init_error = false
    # zeCommandQueueExecuteCommandLists is asynchronous, so we do not check a
    # list's memory copies at execute time (the destination may only be allocated
    # later by another unit that then signals a wait-event). Instead each
    # submitted list becomes a ZEModel::DeferredUnit here and is advanced by
    # pump_deferred as events get signaled -- NO Ruby threads/fibers; it is a
    # plain cursor-based worklist driven by the single trace-consumption loop.
    @deferred_units = []
    # ADDED: bumped whenever an event transitions to signaled, so pump_deferred
    # knows something may have become unblocked and is worth another sweep.
    @signal_epoch = 0
    # ------------------------------------------------------------------
    # Build the thread-safety callbacks from ze_thread_safety.yaml.
    #
    # The YAML maps each API to the objects it must hold exclusively, as
    # [param_name, object_type] pairs -- e.g. for zeCommandListAppendBarrier,
    # ['hCommandList', 'command_list'] means "the command list named by the
    # hCommandList argument". So `o.first` is the parameter name to read the
    # handle from, and `o.last` is which model table to look it up in.
    #
    # For every such pair we synthesize two lambdas: one that locks the object
    # when the call starts, one that unlocks it when the call returns. Because
    # trace events are timestamp-ordered, finding an object already locked means
    # two calls genuinely overlapped -- see ZEModel::Object#lock.
    #
    # A parameter may name an ARRAY of handles (e.g. a list of command lists),
    # hence the kind_of? Array branch in each lambda.
    # ------------------------------------------------------------------
    @ze_thread_safety.each { |api, objects|
      objects.each { |o|
        @lock_shared_object_on_entry[api].push( lambda { |state, ctx, defi|
                                    #at entry the input args are in defi directly
                                    handle = defi[o.first]
                                    if handle.kind_of? Array
                                      handle.each { |h|
                                        # CHANGED: nil-guard -- find_object may
                                        # return nil for an unknown handle (e.g.
                                        # tracing started mid-stream); do not crash
                                        obj = state.find_object(ctx, o.last, h)
                                        obj.lock(state, ctx) if obj
                                      }
                                    else
                                      obj = state.find_object(ctx, o.last, handle)
                                      obj.lock(state, ctx) if obj
                                    end
                                  })
        @unlock_shared_object_on_exit[api].push( lambda { |state, ctx, defi|
                                   #at exit defi holds only outputs, so the input
                                   #handle comes from the saved entry payload
                                   handle = state.find_param(ctx, o.first)
                                   if handle.kind_of? Array
                                     handle.each { |h|
                                       # CHANGED: nil-guard as above
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

  # Reads ze_device_property.json from DATADIR. Returns the parsed hash, or nil
  # if the file is missing or unparseable so the validator degrades gracefully.
  def load_device_properties
    path = File.join(DATADIR, 'ze_device_property.json')
    return nil unless File.file?(path)
    JSON.parse(File.read(path))
  rescue JSON::ParserError => e
    $stderr.puts "Warning: could not parse #{path}: #{e.message}"
    nil
  end

  # Look up a command queue group by ordinal. Without a device index it returns
  # the matching group from the first device (sufficient for homogeneous nodes).
  # Returns a hash like {"ordinal"=>1, "type"=>"copy", "numQueues"=>8} or nil.
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


  # --------------------------------------------------------------------------
  # Model navigation. Each of these walks the trace context down the model tree
  # (hostname -> Node -> pid -> Process -> tid -> Thread). Every level
  # auto-creates on first access, so these never return nil for a new
  # host/process/thread.
  # --------------------------------------------------------------------------

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

  # Sanity check run at every _exit: the call we are returning from must be the
  # one on top of this thread's stack. A mismatch means the model has lost sync
  # with the trace (missing or reordered events), so it aborts loudly rather
  # than producing nonsense diagnostics from a corrupt state.
  def check_last_entry(context)
    last_entry = get_last_entry(context)
    unless last_entry && last_entry.name == context['api']
      raise "Invalid State in #{context['api']}"
    end
  end


  # CHANGED: push a new call frame instead of overwriting a single slot, so a
  # traced API that calls another traced API on the same thread nests correctly.
  def set_last_entry(state, context, defi)
    get_thread(context).call_stack.push(ZEModel::ApiCall.new(context['api'], defi))
  end

  # CHANGED: pop the innermost frame on return, exposing the caller's frame (if
  # any) rather than clearing everything.
  def reset_last_entry(context)
    get_thread(context).call_stack.pop
  end

  # Did the call succeed? The exit payload carries the raw integer return code;
  # the generated FFI enum turns it back into a symbol to compare against
  # ZE_RESULT_SUCCESS. This decides whether on_exit runs the success or the
  # error callback for the API.
  def validate_result(defi)
    ZE::ZEResult.from_native(defi["zeResult"], nil) == :ZE_RESULT_SUCCESS
  end

  # --------------------------------------------------------------------------
  # Message formatting. Handles are printed zero-padded to 16 hex digits so
  # columns line up and the same object is textually identical everywhere --
  # which also makes these strings safe to use as print_tracker dedup keys.
  # --------------------------------------------------------------------------
  def get_handle_str(handle)
    '0x%016x' % handle
  end

  # "hostname - pid": the right granularity for process-wide findings such as
  # leaks and deadlocks, which no single API call is responsible for.
  def get_proc_context_str(context)
    "#{context['hostname']} - #{context['vpid']}"
  end

  # "tid in zeSomeApi": identifies the specific call.
  def get_api_context(context)
    "#{context['vtid']} in #{context['api']}"
  end

  # Fully qualified "hostname - pid - tid in zeSomeApi", used for findings
  # attributable to one call.
  def get_context_str(context)
    "#{get_proc_context_str(context)} - #{get_api_context(context)}"
  end

  # Warn once per deprecated API actually used. The third slot of the
  # @deprecated entry is the already-printed flag.
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

  # --------------------------------------------------------------------------
  # DIAGNOSTIC CHANNELS. Every finding the validator reports goes through one of
  # these; nothing else prints. Each writes to stderr with a distinct
  # "Level Zero <Kind>:" prefix so a report can be grepped or filtered by
  # severity class. Findings are printed as they are discovered, so their order
  # follows the trace timeline.
  #
  #   Portability Error - works here, may break on other/newer hardware
  #   Performance Issue - correct but slow API usage
  #   Usage Error       - violates the Level Zero specification
  #   Crash Error       - a call that never returned (no _exit event in trace)
  #   Memory Error      - use-after-free, freeing memory still in use
  #   Deadlock          - circular event dependency; work can never proceed
  #   Leak              - object created but never destroyed
  # --------------------------------------------------------------------------
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

  # ADDED: reporting channel for memory-safety violations (use-after-free,
  # freeing memory still in use by in-flight device work).
  def print_memory_error(context, str)
    $stderr.puts "Level Zero Memory Error: on #{get_context_str(context)}: #{str}\n\n"
  end

  # ADDED: reporting channel for circular event dependency (deadlock). Uses the
  # process-level context because a deadlock spans multiple command lists/threads
  # rather than a single api call.
  def print_deadlock_error(context, str)
    $stderr.puts "Level Zero Deadlock: on #{get_proc_context_str(context)}: #{str}\n\n"
  end



  # Reported from check_issues at end of trace for every object still alive.
  # memtypestr distinguishes device/host/shared for memory allocations; it is
  # empty for handle-based objects.
  def print_leak_error(context, type, handle, memtypestr="")
    if memtypestr.empty?
      $stderr.puts "Level Zero Leak: on #{get_proc_context_str(context)}: #{type} #{get_handle_str(handle)}\n\n"
    else
      $stderr.puts "Level Zero Leak #{memtypestr}-memory: on #{get_proc_context_str(context)}: #{type} #{get_handle_str(handle)}\n\n"
    end
  end

  # NOT a finding about the traced program -- this aborts the validator itself.
  # Used when the model reaches a state that should be impossible (e.g. deleting
  # an object that was never created), which means the validator's own
  # bookkeeping is wrong and any further output would be untrustworthy.
  def raise_internal_error(context, str)
    raise "Invalid state #{get_context_str(context)}: #{str}"
  end

  # Reported from ZEModel::Object#lock when two calls hold the same object at
  # once. Deduped per (object, other holder) so a racing loop reports once.
  def print_race_condition(context, other_context, type, handle)
    if @print_tracker["#{type}-#{get_handle_str(handle)}-#{get_api_context(other_context)}"] == 0
      @print_tracker["#{type}-#{get_handle_str(handle)}-#{get_api_context(other_context)}"] = 1
      print_usage_error(context, "concurrent acces to #{type} #{get_handle_str(handle)}, already held by #{get_api_context(other_context)}")
    end
  end

  # Passed as the block to Hash#delete by the destroy callbacks, so it fires
  # when a destroy names a handle the model never recorded. `sub_context` says
  # which secondary table was being cleaned up (e.g. the owning context's list).
  def object_not_found(context, type, handle, sub_context = nil)
    raise_internal_error(context, "event_pool #{get_handle_str(handle)} not found#{sub_context ? " in #{sub_context}" : ""}")
  end

  # --------------------------------------------------------------------------
  # Lookup helpers used constantly by the callbacks and checks.
  # --------------------------------------------------------------------------

  # Read one INPUT argument of the call currently executing on this thread. This
  # works at _exit too -- the exit payload has only outputs, but the entry
  # payload is still on the thread's call stack, which is exactly what this
  # reaches into.
  def find_param(context, name)
    get_last_entry(context).params[name]
  end

  # The whole handle->object table for a type, e.g. find_objects(ctx,
  # 'command_list') returns the process's @command_lists.
  def find_objects(context, type)
    get_process(context).instance_variable_get("@#{type}s")
  end

  # Resolve a single object. `handle` may be either the integer handle itself
  # or, as a convenience, the NAME of the parameter carrying it -- so
  # find_object(ctx, 'context', 'hContext') reads hContext from the current call
  # and looks the resulting handle up in one step.
  def find_object(context, type, handle)
    handle = find_param(context, handle) if handle.kind_of? String
    find_objects(context, type)[handle]
  end

  # ADDED: the live-allocation sub-map for one Level Zero context (address ->
  # Memory). Allocations are keyed by context handle because the L0 unified
  # virtual address space only guarantees non-aliasing addresses within a
  # context (see Process#memory_allocations). ctx_handle nil falls back to a
  # single shared bucket so a trace that started mid-stream -- where the owning
  # context is unknown -- still tracks something rather than crashing.
  def memory_allocations(context, ctx_handle)
    get_process(context).memory_allocations[ctx_handle]
  end

  # ADDED: per-context registry of allocations that have been zeMemFree'd but
  # kept for use-after-free detection (address -> freed ZEModel::Memory).
  # CHANGED: now scoped by context handle, mirroring the live map above.
  def freed_memory_allocations(context, ctx_handle)
    get_process(context).freed_memory_allocations[ctx_handle]
  end

  # ADDED: yield [unit, op] for every copy/fill op still pending (at or after the
  # cursor) in an in-flight deferred unit belonging to this process. Used by the
  # free-in-flight check to see whether a buffer being freed is still referenced
  # by device work that has been submitted but not yet completed. Deferred units
  # do not carry a process id, so we match on the unit's context host+pid.
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

  # ADDED: true if command list `handle` still has an in-flight deferred
  # execution in this process -- a prior zeCommandQueueExecuteCommandLists whose
  # ops have not all drained yet. Used by zeCommandListReset, which must not run
  # while the list is still executing (undefined behavior in Level Zero). Matches
  # on the unit's originating list handle and the process it belongs to (deferred
  # units carry no pid, so we compare context host+pid, mirroring
  # each_inflight_copy_op).
  def command_list_in_flight?(context, handle)
    @deferred_units.any? do |unit|
      unit.cmd_list_handle == handle &&
        unit.context['hostname'] == context['hostname'] &&
        unit.context['vpid'] == context['vpid'] &&
        !unit.done?
    end
  end

  # Decode a raw descriptor blob from the trace into a typed FFI struct.
  # The tracer captures structs like ze_command_queue_desc_t as opaque bytes;
  # this copies them into native memory and reinterprets them as `klass` (from
  # the generated ze_library bindings) so callbacks can read desc[:ordinal],
  # desc[:stype], and so on. Returns nil for an empty blob (a null descriptor
  # pointer), which every caller must be prepared for.
  def to_struct(memory, klass)
    memory.size > 0 ? klass.new(FFI::MemoryPointer.from_string(memory)) : nil
  end

  # ============================================================================
  # ADDED: Event semantics + non-concurrent deferred-execution scheduler.
  #
  # zeCommandQueueExecuteCommandLists is asynchronous. Checking a list's memory
  # copies against the memory model at execute time gives false positives,
  # because a copy's destination may only be allocated by another unit that
  # signals a wait-event later. So each submitted command list is turned into a
  # ZEModel::DeferredUnit and its ops are replayed only as their wait-events
  # actually become signaled.
  #
  # There are NO Ruby threads or fibers. Each unit keeps an integer cursor into
  # its op list. pump_deferred repeatedly sweeps all units, advancing any unit
  # whose current op has all wait-events satisfied, and loops until a full sweep
  # makes no progress. A unit left parked on an unsatisfied op is simply waiting
  # for a future event (which arrives as later trace events are consumed).
  # ============================================================================

  # ADDED: look up an Event model object by raw handle. nil for a null/unknown
  # handle (nothing to track).
  def event_by_handle(context, handle)
    return nil if handle.nil? || handle == 0
    find_objects(context, 'event')[handle]
  end

  # ADDED: signal an event and note progress so pump_deferred re-sweeps. `by`
  # records who signaled it, for diagnostics.
  def signal_event(context, handle, by = nil)
    ev = event_by_handle(context, handle)
    if ev
      ev.signal(by)
      @signal_epoch += 1
    end
    ev
  end

  # ADDED: return an event to the unsignaled state.
  def reset_event(context, handle)
    event_by_handle(context, handle)&.reset
  end

  # ADDED: record that the host observed an event's signaled state.
  def observe_event(context, handle)
    event_by_handle(context, handle)&.observe
  end

  # ADDED: a device-wide host synchronization (zeCommandQueueSynchronize, or for
  # immediate lists zeCommandListHostSynchronize) means the host waited for all
  # submitted work -- so every currently-signaled event has been consumed. Mark
  # them observed so a later signal without a reset reads as reuse-without-reset
  # rather than a concurrent double-signal.
  def observe_all_signaled_events(context)
    find_objects(context, 'event').each_value { |ev| ev.observe if ev.signaled? }
  end

  # ADDED: true once every wait handle is signaled (or is null/unknown, which we
  # treat as satisfied: we cannot track it, and any unit may signal an event, so
  # we must not invent a deadlock).
  def waits_satisfied?(context, waits)
    return true if waits.nil? || waits.empty?
    waits.all? { |h| ev = event_by_handle(context, h); ev.nil? || ev.signaled? }
  end

  # ADDED: execute one op of a unit (the op is known to be runnable). Runs the
  # deferred checks, then applies the op's reset/signal side effects, and
  # advances the cursor. Returns true if it signaled an event (progress that may
  # unblock other units).
  def run_deferred_op(unit)
    context = unit.context
    op = unit.current_op
    if op.kind == :copy
      check_oob_copy(self, context, op.params)
      #a pointer freed before this copy's turn to execute is a use-after-free
      check_use_after_free(self, context, op.params)
    end
    #a memory-ranges barrier references memory freed before its turn is a UAF
    check_ranges_barrier(self, context, op.params) if op.kind == :ranges_barrier
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

  # ADDED: advance every deferred unit as far as its wait-events allow. Sweeps
  # repeatedly until a whole pass makes no progress (completed an op or signaled
  # an event), then drops finished units. Units still parked on an unmet wait
  # stay queued for a future event or the end-of-trace flush.
  #
  # Why a repeat-until-quiet loop rather than one pass: advancing unit A can
  # signal an event that unblocks unit B, which may already have been visited
  # earlier in the same sweep. Iterating until a full pass changes nothing
  # reaches the fixed point regardless of the order units happen to sit in.
  # Termination is guaranteed because every unit of progress advances some
  # cursor, and cursors only move forward over finite op lists.
  #
  # Called after every trace event (see #consume), so deferred work advances in
  # lockstep with the real program's event signals.
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
              ev = event_by_handle(unit.context, h); ev.nil? || ev.signaled?
            }
            break
          end
        end
      end
      @deferred_units.reject!(&:done?)
    end
  end

  # ADDED: register a command list's ops as a deferred unit and pump. `ops` is a
  # snapshot (dup) taken by the caller so a later reset+re-append on the same
  # list cannot mutate an in-flight execution.
  def run_deferred_list(context, ops, label, in_order: false, cmd_list_handle: nil)
    @deferred_units << ZEModel::DeferredUnit.new(ops, context, label, in_order: in_order,
                                                 cmd_list_handle: cmd_list_handle)
    pump_deferred
  end

  # ADDED: deferred execution of the lists submitted to
  # zeCommandQueueExecuteCommandLists. Each list becomes its OWN unit: lists in
  # one submit are ordered only by events, not by list order, so a circular
  # event dependency across two lists in one submit is a real deadlock we must be
  # able to see.
  def enqueue_deferred_execution(context, command_lists)
    command_lists.each do |cl|
      next unless cl
      run_deferred_list(context, cl.ops.dup, "command_list (#{get_handle_str(cl.handle)})",
                        in_order: cl.in_order, cmd_list_handle: cl.handle)
    end
  end

  # ADDED: immediate command lists execute each op as it is appended, so we
  # schedule the single op immediately. It still honors wait-events and goes
  # through the same machinery, giving immediate lists the same OOB-copy and
  # event-reuse checks as regular lists.
  def enqueue_immediate_op(context, op, handle = nil)
    label = handle ? "immediate command list (#{get_handle_str(handle)})" \
                   : 'immediate command list'
    run_deferred_list(context, [op], label)
  end

  # ADDED: end-of-trace drain. First pump normally in case ordering left work
  # runnable. Whatever is still parked cannot progress on its own -- report any
  # circular event dependency (deadlock) among the stuck units, then force each
  # remaining unit's blocked op (reporting the never-signaled wait) so the
  # deferred checks still run against the final memory state.
  def flush_deferred
    pump_deferred
    return if @deferred_units.empty?
    check_circular_deadlock(self, @deferred_units)
    #ADDED: an in-order list where an earlier op waits on an event only a later op
    #in the SAME list signals is a self-deadlock the cross-list check cannot see
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

  # END-OF-TRACE REPORTING PASS, called once by ze_validator.in after the whole
  # trace has been consumed. Findings here are the ones that are only knowable
  # when you know nothing further is coming:
  #   1. deferred work that never completed  -> deadlocks (via flush_deferred)
  #   2. API calls with an _entry but no _exit -> the process crashed inside them
  #   3. objects still present in the model    -> leaks
  # Everything else was already reported inline as the trace was replayed.
  def check_issues()
    #ADDED: drain deferred command-list executions (and detect deadlocks) before
    #reporting leaks/crashes
    flush_deferred
    crash = false
    @state.each { |hostname, node|
      node.processes.each { |pid, process|
        process.threads.each { |tid, thread|
          # CHANGED: iterate the whole call stack instead of a single slot. Any
          # frame still on the stack is a traced call that never returned (a
          # crash); a clean run pops every frame back to empty.
          thread.call_stack.each { |frame|
            ctx = {'hostname' => hostname, 'vpid'=> pid, 'vtid' => tid, 'api' => frame.name}
            print_crash_error(ctx, 'command did not finish execution')
            crash = true
          }
        }
      }
    }

    #if !crash || true
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
            #objects that were created will be deleted upon successful exits.
            #So, only the ones that didn't get deleted will be reported
            process.objects(t).each { |h, c|
              print_leak_error(ctx, t, h) #it prints the type as well
            }
          }
          # CHANGED: memory_allocation is now nested by context handle
          # (ctx_handle -> {address -> Memory}), so iterate one level deeper.
          # Any allocation still live at end of trace, in any context, is a leak.
          process.objects('memory_allocation').each { |_ctx_handle, allocs|
            allocs.each { |h, c|
              #puts "mem alloc type = #{c.instance_variable_get(:@memtypestr)}"
              print_leak_error(ctx, 'memory_allocation', h, c.instance_variable_get(:@memtypestr))
            }
          }
        }
      }
    end
  end


  # The Level Zero spec requires zeInit (or zeInitDrivers) before any other API
  # call. Initialization is per-process, so the flag is keyed by pid. `m` is the
  # regex match from #consume, so m[1] is the API name.
  # Reported at most once per run to avoid one missing init producing an error
  # for every subsequent call in the trace.
  def check_initialization(context,m)
	if ZEModel::INIT_API_NAMES.include?(m[1])
        @init_called[context['vpid']] = true
    end

	if !@init_called[context['vpid']] && !@printed_init_error
		self.print_usage_error(context, "zeInit or zeDriversInit wasn't called before #{m[1]}")
		@printed_init_error = true
	end
  end


  # Handle a call ENTRY, in order:
  #   1. push the call (with its input args) onto the thread's stack, so
  #      find_param can reach those args later, including from the exit callback
  #   2. take the thread-safety locks this API requires
  #   3. run the API's $upon_entry callback, if any
  #
  # Checks live at entry either because they need the input arguments in their
  # pre-call state, or -- more often -- because the call being validated might
  # crash the process, in which case no _exit event is ever written and an
  # exit-time check would silently never run.
  def on_entry(m,hostname, context,defi)
    set_last_entry(self, context, defi) #sets the per-thread callstack of the APIs
      @lock_shared_object_on_entry[m[1]].each { |l|
                  l.call(self, context, defi)
      }
    #modifies the satate based on entry fields. Needed because some fields are easier to access it from the entry
    l = $upon_entry[m[1]]
    l.call(self,context,defi) if l
  end

  # Handle a call EXIT, the mirror image of on_entry:
  #   1. release the thread-safety locks (in reverse order -- lock ordering
  #      discipline, so nested acquisitions unwind as a stack)
  #   2. run the success OR the error callback, depending on the return code
  #   3. verify and pop the thread's call stack
  #
  # Most model MUTATION happens here rather than at entry, because a call that
  # failed must not be allowed to change the model (a failed Create produced no
  # object), and because output handles only exist once the call has returned.
  def on_exit(m,hostname,context,defi)
    #unlock the shared object if the api name matches the predefined in ze_thread_safety.yaml
    @unlock_shared_object_on_exit[m[1]].reverse_each { |l|
      l.call(self, context, defi)
    }

    #check if the return code indicates successful return from the API call
    if validate_result(defi)
      l = $on_successful_exit[m[1]] #This might be a problem for tracking erroneous exits.
      l.call(self, context, defi) if l
    else
      #puts "failed: #{m[1]}"
      l = $on_erroneous_exit[m[1]]
      l.call(self, context, defi) if l
    end

    check_last_entry(context)  #When we return from _exit, we need to see what we saw in _entry for the current thread_id
    reset_last_entry(context)  #Reset the callstack for current thread_id
  end

  # THE MAIN LOOP. Returns the lambda babeltrace invokes with each batch of
  # decoded messages; it is installed as the graph's sink in ze_validator.in.
  # Everything the validator does happens somewhere inside this callback.
  #
  # Note the Ruby endless-method syntax (`def consume = lambda {...}`): calling
  # `consume` returns the lambda rather than running it.
  def consume = lambda { |iterator, _|
        iterator.next_messages.each do |m|
          #the stream also carries stream-begin/end and packet messages; only
          #actual trace events are of interest here
          next unless m.type == :BT_MESSAGE_TYPE_EVENT
          e = m.event
          #Event names look like "lttng_ust_ze:zeMemAllocDevice_entry". This
          #splits out the API name (m[1]) and the phase (m[2]). Non-matching
          #events -- other backends sharing the trace, tracer-internal events --
          #fall through the `if` and are ignored.
          m = e.name.match(/:(z.*)_(entry|exit)/)
          if m
            #hostname is trace-level metadata (recorded once per trace), while
            #vpid/vtid come from LTTng's per-event common context. Merging them
            #with the API name produces the `context` tuple that identifies
            #"who is calling what", threaded through the entire validator.
            hostname = e.stream.trace.get_environment_entry_value_by_name('hostname').value
            context = e.get_common_context_field.value
            #the event's own fields: input args at _entry, results at _exit
            defi = e.payload_field.value
            context['hostname'] = hostname
            context['api'] = m[1]
			      #zeDriversInit or zeInit must be the first one to be called before any api calls
            check_initialization(context,m)
            #print the known deprecated APIs
            print_deprecation_warning(m[1]) if @deprecated[m[1]]

            if m[2] == 'entry'
              on_entry(m, hostname, context, defi)
            elsif m[2] == 'exit'
              #puts "#{m[1]}"
              on_exit(m, hostname, context, defi)
            end
            #ADDED: this event may have signaled something a deferred command list
            #was waiting on, so advance the deferred worklist now
            pump_deferred
          end
        end
      }

end #end of StateObject
