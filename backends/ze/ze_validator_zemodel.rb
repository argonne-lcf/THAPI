require 'set'

# =============================================================================
# ZEModel -- the data model the validator replays a trace into.
#
# Every class here is a plain Ruby mirror of a Level Zero concept. As the
# validator walks the trace, a `zeXxxCreate` event constructs one of these and
# files it in a lookup table; a `zeXxxDestroy` removes it. Anything still
# present when the trace ends is, by definition, leaked.
#
# LEVEL ZERO IN ONE PARAGRAPH (for readers new to the API)
# --------------------------------------------------------
# Level Zero is Intel's low-level GPU compute API (the layer SYCL/OpenMP sit on
# top of). The object hierarchy is roughly:
#
#   Driver            one per installed GPU runtime
#    └─ Device        a physical GPU (may expose SubDevices, e.g. tiles)
#   Context           an isolation domain: memory and objects belong to exactly
#                     one context and may not be mixed across contexts
#    ├─ Memory        allocations (device / host / shared)
#    ├─ Module        a compiled GPU binary
#    │   └─ Kernel    one entry point within a module
#    ├─ CommandList   a recorded sequence of GPU operations ("append" to build
#    │                it, "close" to finalize it)
#    ├─ CommandQueue  where a closed command list is submitted to actually run
#    │   └─ Fence     host-visible "this submission finished" signal
#    └─ EventPool     preallocated slots for Events
#        └─ Event     fine-grained GPU/host synchronization token
#
# Work is ASYNCHRONOUS: submitting a command list returns immediately, and the
# operations inside it run later, ordered by Events. That asynchrony is the
# reason for RecordedOp and DeferredUnit near the bottom of this file -- the
# validator cannot check a memory copy at the moment it is appended, because
# the copy has not happened yet.
#
# HANDLES
# -------
# Level Zero identifies every object by an opaque pointer-sized "handle"
# (ze_command_list_handle_t and friends). In the trace these arrive as plain
# integers, and the validator uses them as hash keys throughout. When you see a
# bare `handle` in this codebase, it is that integer.
#
# WHERE THESE OBJECTS LIVE
# ------------------------
# Node (one per hostname)
#   └─ Process (one per pid)        <- the main container; see Process below
#        ├─ threads    (per tid, each with a call stack)
#        └─ one hash per object type: @devices, @contexts, @command_lists, ...
# StateObject#find_objects(ctx, 'command_list') is how the rest of the code
# reaches those hashes.
#
# A NOTE ON THE "ADDED:" / "CHANGED:" COMMENTS
# --------------------------------------------
# Comments marked ADDED/CHANGED/REMOVED record deliberate deviations from an
# earlier version of the model and explain WHY the change was necessary. They
# are kept because the reasoning (e.g. why memory is keyed by context) is not
# recoverable from the code alone.
# =============================================================================
module ZEModel
  #One of these APIs must be called before any other calls
  # The Level Zero spec requires the runtime to be initialized before any other
  # entry point is used. Calling anything else first is undefined behavior, so
  # StateObject#check_initialization watches for one of these appearing first.
  INIT_API_NAMES = ['zeInit', 'zeInitDrivers']
  #This defines the object in which most ze objects (command list, command queue) extend form
  # Common base class for every tracked Level Zero object. It provides two
  # things every object needs:
  #   * @handle -- the integer identity from the trace
  #   * a lock  -- NOT a real mutex. It is a marker used to detect data races:
  #     see #lock below.
  class Object
    attr_reader :handle
    attr_accessor :status

    # returns what object the caller is
    # e.g., 'Device' will return device
    # Each subclass sets `@typename` at class level (a class instance variable);
    # this reader exposes it for error messages, e.g. "concurrent access to
    # command_list 0x...".
    def self.typename
      @typename
    end

    # lock is needed to check for concurrent properties.
    # e.g., calling the same APIs that can be called from simultaneous threads (zeCommandListAppendBarrier)
    def initialize(handle)
      @handle = handle
      @lock = nil
      #@status = -1
    end

    # THREAD-SAFETY CHECK.
    # The Level Zero spec documents, per API, which objects may NOT be touched
    # concurrently from two threads. ze_thread_safety.yaml encodes that table,
    # and StateObject turns each entry into a pair of callbacks that lock the
    # object on _entry and unlock it on _exit.
    #
    # Because the validator processes events one at a time (never truly in
    # parallel), @lock is not a real mutex -- it is simply "which API call
    # currently has this object open". Since the trace is timestamp-ordered,
    # finding the object ALREADY locked when a second call tries to lock it
    # means two calls overlapped in wall-clock time on the real machine: a
    # genuine data race. `ctx` identifies the locking call (thread id + API).
    def lock(state,ctx)
      if @lock
        state.print_race_condition(ctx, @lock, self.class.typename, @handle)
      else
        @lock = ctx
      end
    end

    # Release only if this same call is the holder. The guard matters for
    # nested/overlapping calls: a thread that failed to acquire the lock above
    # (and was reported as a race) must not then steal the real holder's lock
    # by unlocking on its own way out.
    def unlock(ctx)
      if @lock == ctx
        @lock = nil
      end
    end
  end

  # 'A < B' means A inherits from B
  # The driver is an interface that serves between the host and the devices
  # Created by zeDriverGet, which reports every Level Zero driver installed.
  # Holds the Devices discovered under it via zeDeviceGet.
  class Driver < Object
    @typename = 'driver'
    attr_reader :devices

    def initialize(handle)
      super
      @devices = []
    end
  end


  # Device is mostly GPU
  # A physical accelerator, created by zeDeviceGet. Besides identity, the model
  # tracks two "did the application ask about me before assuming things?" flags
  # used by the portability checks -- hardcoding device characteristics instead
  # of querying them is the classic way GPU code breaks on the next generation
  # of hardware.
  class Device < Object
    @typename = 'device'
    attr_reader :properties #delete
    attr_reader :sub_devices
    # CHANGED: nested by Level Zero context handle -- ctx_handle -> {addr -> Memory}
    # -- for the same reason as Process#memory_allocations: an address is only
    # guaranteed unique within a context, and one device can back allocations in
    # several contexts. Auto-vivifies an empty sub-map per context.
    attr_accessor :memory_allocations
    # set by zeDeviceGetProperties -- the app asked what this device is
    attr_accessor :property_fetched
    # set by zeDeviceGetCommandQueueGroupProperties -- the app asked which
    # engines (compute / copy) this device has and how many queues each
    # supports. If it never asks but still passes an ordinal, it is guessing:
    # see check_group_property_queued.
    attr_accessor :cmd_queue_group_properties_queried
    #attr_accessor :p2p_list

    def initialize(handle)
      super
      @sub_devices = []
      @memory_allocations = Hash.new { |h, k| h[k] = {} }
      @property_fetched = false
      #This
      @cmd_queue_group_properties_queried = false
    end
  end



  # A tile/slice of a Device exposed by zeDeviceGetSubDevices. It behaves like a
  # Device in every respect (hence the inheritance) but remembers its parent.
  # Note the unusual ordering below: @parent is assigned BEFORE calling super,
  # because Device#initialize is what actually sets up the shared state.
  class SubDevice < Device
    attr_reader :parent
    def initialize(handle, parent)
      @parent = parent
      super(handle)
    end
  end

  #create memory object so that device, shared, host mem allocs can be differentiated
  # One GPU-visible allocation. Level Zero has three flavors, distinguished by
  # @memtypestr because they have different accessibility and residency rules:
  #   "device" - lives in GPU memory; only that device can touch it
  #   "host"   - lives in host memory; every device in the context can touch it
  #   "shared" - migrates between host and device on demand
  #
  # Unlike the handle-based objects, memory is identified by its ADDRESS, and
  # the validator must reason about ranges rather than exact matches: a copy
  # into `base + 64` is a legitimate use of the allocation starting at `base`.
  # Hence @base and @size are the fields most checks actually consult.
  class Memory < Object
    @typename = 'memory_allocation'
    attr_reader :context     # the Context this allocation belongs to
    attr_reader :size        # length in bytes
    attr_reader :owned_by    # the Device for a device allocation; nil for host
    # Whether the allocation is currently resident in device memory. Toggled by
    # zeContextMakeMemoryResident / zeContextEvictMemory. Device allocations
    # start non-resident; host/shared memory is always reachable.
    attr_accessor :resident
    attr_accessor :memtypestr  # "device" | "host" | "shared"
    # Start address. Equal to the handle -- for memory the "handle" IS the
    # pointer -- but kept under a descriptive name because range arithmetic
    # (base <= ptr < base + size) reads far better than handle arithmetic.
    attr_accessor :base
    # ADDED: api-context string of the zeMemFree that released this allocation,
    # or nil while live. A freed allocation is moved to the process-level
    # @freed_memory_allocations registry (kept, not discarded) so a later copy/
    # fill/kernel that still references its address range can be reported as a
    # use-after-free instead of silently passing (unknown pointer).
    attr_accessor :freed_by
    def initialize(handle, context, size, owned_by, memtypestr="shared")
      super(handle)
      @context = context
      @size = size
      @owned_by = owned_by
      @memtypestr = memtypestr
      @base = handle
      @freed_by = nil # ADDED
      #puts "size = #{size}, handle = #{handle}, handle+size=#{handle + size}"
      #for device memory.
      # Device allocations must be explicitly made resident before the GPU can
      # use them; host and shared memory is always accessible, so it starts
      # resident and stays that way.
      if memtypestr == "device"
        @resident = false
      else
        @resident = true
      end
    end
  end


  # No need to create DeviceMemory class. Just create Memory with the specified type (device,shared,host)
  # class DeviceMemory < Object
  #   @typename = 'memory_allocation_device'
  #   attr_reader :context
  #   attr_reader :size
  #   attr_reader :device

  #   def initialize(handle, context, size, device)
  #     super(handle,context,size,device)
  #     @context = context
  #     @size = size
  #     @device = device
  #   end
  # end





  # THE CENTRAL ISOLATION BOUNDARY.
  # Nearly every "you cannot mix these" rule in Level Zero is really "these two
  # objects were created on different contexts". A command list, the events it
  # uses, the queue it is submitted to, and the memory it copies must all belong
  # to one context. A large share of the validator's checks are variations on
  # comparing two Context objects, and the per-context nesting of the memory
  # maps (see Process below) exists for the same reason.
  #
  # The child collections below mirror what the context owns, so destroying a
  # context can report anything still alive inside it.
  class Context < Object
    @typename = 'context'
    attr_reader :driver
    attr_reader :desc     # the ze_context_desc_t passed to zeContextCreate
    attr_reader :devices  # nil means "all devices of the driver"

    attr_reader :event_pools
    attr_reader :command_queues
    attr_reader :command_lists
    attr_reader :modules
    attr_reader :module_build_logs

    def initialize(handle, driver, desc, devices = nil)
      super(handle)
      @driver = driver
      @desc = desc
      @devices = devices

      @event_pools = {}
      @command_queues = {}
      @command_lists = {}
      @modules = {} #binaries for gpu
      @module_build_logs = {}
    end
  end

  # A fixed-size block of event slots, allocated up front by zeEventPoolCreate.
  # Events are not individually allocated: the pool reserves `desc[:count]`
  # slots, and each zeEventCreate claims one by index. Two live events may not
  # share an index, so the model tracks which indices are still free.
  #
  # An event pool is also how an Event acquires a context (an Event has no
  # context of its own), which several checks rely on.
  class EventPool < Object
    @typename = 'event_pool'
    attr_reader :context
    attr_reader :desc
    attr_reader :devices
    attr_reader :events   # handle -> Event, for leak reporting on destroy
    # The set of slot indices NOT yet in use. Starts as {0, 1, ..., count-1};
    # zeEventCreate removes one (double use = error) and zeEventDestroy puts it
    # back (double free = error).
    attr_reader :indices

    def initialize(handle, context, desc, devices = nil)
      super(handle)
      @context = context
      @desc = desc
      @devices = devices
      @events = {}
      @indices = Set.new(desc[:count].times.to_a)
    end
  end

  # A synchronization token. An Event is a one-bit flag the GPU or host can
  # SIGNAL, that other work can WAIT on, and that must be RESET before it is
  # reused. Getting that lifecycle wrong is a major source of GPU hangs, so the
  # model tracks more than just the bit -- see the accessors below.
  class Event < Object
    @typename = 'event'
    attr_reader :event_pool
    attr_reader :desc
    attr_accessor :signaled
    # ADDED: richer event state so we can model Level Zero event semantics.
    #   signaled_by - api-context string of whoever last signaled this event
    #                 (used only for diagnostic messages).
    #   observed    - whether the host has observed the signaled state since the
    #                 last signal (via zeEventHostSynchronize / a successful
    #                 zeEventQueryStatus / a device-wide synchronize). This lets
    #                 us tell a genuine concurrent double-signal (signaled but
    #                 never consumed) from a reuse-without-reset (signaled,
    #                 consumed by the host, then signaled again with no reset).
    attr_reader :signaled_by
    attr_reader :observed

    def initialize(handle, event_pool, desc)
      super(handle)
      @event_pool = event_pool
      @desc = desc
      #event can have 2 states, not signaled or signaled
      @signaled = false
      @signaled_by = nil
      @observed = false
    end

    # ADDED: move the event to the signaled state. `by` records who signaled it
    # (for messages). A fresh signal has not yet been observed by the host.
    def signal(by = nil)
      @signaled = true
      @signaled_by = by
      @observed = false
    end

    # ADDED: zeEventHostReset / zeCommandListAppendEventReset return the event to
    # the unsignaled state so it can be reused as a dependency again.
    def reset
      @signaled = false
      @signaled_by = nil
      @observed = false
    end

    # ADDED: record that the host observed the signaled state. Distinguishes a
    # later reuse-without-reset from a concurrent double-signal.
    def observe
      @observed = true
    end

    def signaled?
      @signaled
    end
  end

  # Where closed command lists are submitted for execution. A queue is bound at
  # creation to one engine of one device, identified by desc[:ordinal] (which
  # command queue GROUP -- compute, copy, or both) and desc[:index] (which
  # queue within that group). Submitting compute work to a copy-only ordinal, or
  # using an index beyond the group's numQueues, is checked against the real
  # device topology in ze_device_property.json.
  class CommandQueue < Object
    @typename = 'command_queue'
    attr_reader :context
    attr_reader :device
    attr_reader :desc     # ze_command_queue_desc_t: carries :ordinal and :index
    attr_reader :fences

    def initialize(handle, context, device, desc)
      super(handle)
      @context = context
      @device = device
      @desc = desc
      @fences = {}
      @valid_fences = Hash.new { |h, k| h[k] = true } #fences that have been reset or haven't been signaled
    end
  end

  # A coarse, host-visible completion signal for ONE submission to a queue.
  # Where an Event synchronizes individual operations, a Fence answers "has this
  # whole zeCommandQueueExecuteCommandLists finished?".
  #
  # A fence must be reset before it can be reused. The three-state lifecycle:
  #   not_signaled -> in_use (submitted with this fence)
  #                -> signaled (zeFenceHostSynchronize observed completion)
  #                -> not_signaled again (zeFenceReset)
  # Reusing a fence that is still in_use or already signaled is the misuse
  # check_fence_misuse reports.
  #
  # NOTE: not_signaled/in_use/signaled are per-instance constants exposed as
  # readers, so comparisons read `fence.status == fence.signaled`.
  class Fence < Object
    @typename = 'fence'
    attr_reader :command_queue  # the queue this fence was created for
    attr_reader :desc
    attr_accessor :status       # one of the three values below
    attr_reader :not_signaled
    attr_reader :in_use
    attr_reader :signaled


    def initialize(handle, command_queue, desc)
      super(handle)
      @command_queue = command_queue
      @desc = desc
      @not_signaled = 0
      @in_use = 1
      @signaled = 2
      @status = @not_signaled
    end

  end

  # A recorded sequence of GPU operations -- the core object of the API.
  #
  # NORMAL LIFECYCLE:
  #   zeCommandListCreate -> zeCommandListAppendXxx (many) ->
  #   zeCommandListClose  -> zeCommandQueueExecuteCommandLists
  # Appending only RECORDS an operation; nothing runs until the closed list is
  # submitted to a queue. Executing a list that was never closed is an error
  # (check_command_list_closed).
  #
  # IMMEDIATE LISTS are the exception: created by zeCommandListCreateImmediate,
  # they carry their own implicit queue and execute each operation the moment it
  # is appended. They are never closed and must never be passed to
  # ExecuteCommandLists. Because they take a QUEUE descriptor rather than a list
  # descriptor, the model stores that in @altdesc and leaves @desc nil -- which
  # is why so much code here branches on `desc ? ... : altdesc`.
  class CommandList < Object
    @typename = 'command_list'
    attr_reader :context
    attr_reader :device
    attr_reader :desc     # ze_command_list_desc_t   (nil for immediate lists)
    attr_reader :altdesc  # ze_command_queue_desc_t  (immediate lists only)
    attr_accessor :associated_command_queue
    attr_accessor :immediate  # true for zeCommandListCreateImmediate lists
    attr_accessor :associated_ordinal
    # ADDED: true when the list was created with ZE_COMMAND_LIST_FLAG_IN_ORDER
    # (or, for immediate lists, ZE_COMMAND_QUEUE_FLAG_IN_ORDER). In-order lists
    # execute their appended ops strictly in append order -- op N+1 will not
    # start until op N completes -- so an earlier op that waits on an event only
    # a later op in the SAME list signals can never complete (an intra-list
    # deadlock the cross-list detector cannot see). See check_in_order_self_deadlock.
    attr_accessor :in_order
    # ADDED: ordered list of RecordedOp appended to this command list. It is
    # replayed when the list is executed on a queue, so that checks depending on
    # event completion (out-of-bounds copy, event-signal reuse) run at the point
    # the op would actually execute -- not at append or execute time.
    attr_accessor :ops
    # Lifecycle states, compared via
    # ZEModel::CommandList.class_variable_get(:@@INITIALIZED) at the call sites.
    #   INITIALIZED - open for appending (fresh, or just reset)
    #   CLOSED      - finalized by zeCommandListClose, ready to submit
    #   DESTROYED   - zeCommandListDestroy was called; any further use is a bug
    @@INITIALIZED = 0 #created or being properly recycled
    @@CLOSED = 1
    @@DESTROYED = 2

    def initialize(handle, context, device, desc, altdesc)
      super(handle)
      @context = context
      @device = device
      @desc = desc
      @altdesc = altdesc
      @associated_command_queue = nil
      @status = @@INITIALIZED
      @immediate = false
      @associated_ordinal = 0
      @in_order = false # ADDED
      @api_calls = []
      @ops = [] # ADDED
    end

    # An immediate list has no list descriptor (it was given a queue descriptor
    # instead), so a nil desc identifies it. NOTE: the @immediate flag set by
    # the zeCommandListCreateImmediate callback is what most call sites actually
    # test; this predicate derives the same fact from the descriptor.
    def immediate?
      return !desc
    end
  end

  # ===========================================================================
  # DEFERRED EXECUTION -- why RecordedOp and DeferredUnit exist.
  #
  # THE PROBLEM. Consider this perfectly valid program:
  #
  #     append a copy into buffer X onto list A, gated on event E
  #     submit list A                 <- returns immediately, copy has NOT run
  #     ... later ...
  #     allocate buffer X
  #     signal event E                <- only NOW does the copy actually run
  #
  # If the validator checked the copy's bounds when it was APPENDED, or even
  # when the list was SUBMITTED, buffer X would not exist yet and it would
  # report a bogus error. The check is only meaningful at the moment the copy
  # really executes -- which the trace tells us only indirectly, via events.
  #
  # THE SOLUTION. Appending records a RecordedOp instead of checking. Submitting
  # turns the list's recorded ops into a DeferredUnit: a queued, half-executed
  # program with a cursor. As the trace goes on and events get signaled, the
  # scheduler in StateObject (pump_deferred) walks each unit's cursor forward
  # over every op whose wait-events are now satisfied, running the deferred
  # checks at that point -- the correct point on the program timeline.
  #
  # NO THREADS, NO FIBERS. Execution state is just an integer index. A unit that
  # cannot advance is simply left parked, and retried after the next signal.
  # Anything still parked when the trace ends could never have completed, which
  # is exactly the definition of a deadlock -- so the same data structure that
  # defers the memory checks also powers the deadlock detector.
  # ===========================================================================

  # ADDED: A single operation recorded when it is appended to a command list. It
  # snapshots everything the deferred checks need, because the trace's per-call
  # context (find_param) is gone by the time the op is replayed at execute time.
  #   kind   - :copy, :wait, :signal, :reset, :barrier, :ranges_barrier, :launch
  #   signal - handle of the completion event this op signals (nil/0 if none)
  #   waits  - event handles that must be signaled before this op can execute
  #   params - kind-specific data (copy: api/dst/src/size; reset: reset_handle;
  #            ranges_barrier: api/ctx_handle/ranges, where ranges is an array of
  #            {base:, size:} for each memory range the barrier covers)
  #   api    - the ZE API that appended this op (for diagnostics)
  class RecordedOp
    attr_reader :kind    # :copy | :wait | :signal | :reset | :barrier |
                         # :ranges_barrier | :launch
    attr_reader :signal  # event this op signals on completion (nil if none)
    attr_reader :waits   # events that must be signaled before this op may run
    attr_reader :params  # kind-specific snapshot (see class comment above)
    attr_reader :api     # originating ze API name, for diagnostics

    def initialize(kind, signal: 0, waits: [], params: {}, api: nil)
      @kind = kind
      #normalize a null (0) signal handle to nil so "does this op signal?" is a
      #simple truthiness test
      @signal = (signal && signal != 0) ? signal : nil
      @waits = waits || []
      @params = params
      @api = api || params[:api]
    end
  end

  # ADDED: One deferred-execution unit -- a single submitted command list whose
  # recorded ops are replayed cooperatively by the (non-concurrent) scheduler in
  # StateObject. Instead of a Ruby Fiber, execution state is an explicit integer
  # cursor into `ops`: the scheduler advances the cursor past every op whose
  # waits are satisfied, and leaves it parked on the first op that is still
  # blocked. `blocked_on` / `pending_signals` are the metadata the deadlock
  # detector uses to build a wait-for graph across units.
  #   ops             - snapshot (dup) of the list's ops for this execution
  #   context         - trace context captured at submit time
  #   label           - human label for messages (e.g. "command_list 0x..")
  #   cursor          - index of the next op to execute
  #   blocked_on      - event handles the current op is waiting for (or [])
  #   pending_signals - events this unit may still signal before it finishes
  class DeferredUnit
    attr_reader :ops        # the ops to replay, in order
    attr_reader :context    # trace context (host/pid/tid/api) captured at submit
    attr_reader :label      # e.g. "command_list (0x00007f...)", used in messages
    attr_accessor :cursor   # index of the next op to run; == ops.size means done
    attr_accessor :blocked_on      # events the current op is still waiting for
    # Events this unit has not signaled yet. Read by the deadlock detector: if
    # unit U is blocked on an event that appears only in unit V's
    # pending_signals, then U is waiting on V -- an edge in the wait-for graph.
    attr_accessor :pending_signals
    # ADDED: whether the originating command list is in-order (see CommandList#in_order).
    # The intra-list self-deadlock check only applies to in-order units.
    attr_reader :in_order
    # ADDED: handle of the command list this unit was submitted from (nil if
    # unknown/immediate without a handle). Lets checks scoped to a specific list --
    # e.g. resetting a list while a prior submission is still in-flight -- find the
    # deferred units that belong to it without matching on the label string.
    attr_reader :cmd_list_handle

    def initialize(ops, context, label, in_order: false, cmd_list_handle: nil)
      @ops = ops
      @context = context
      @label = label
      @cursor = 0
      @blocked_on = []
      @in_order = in_order # ADDED
      @cmd_list_handle = cmd_list_handle # ADDED
      #every event this unit will eventually signal, for the wait-for graph
      @pending_signals = ops.map { |op| op.signal }.compact
    end

    # true once every op has executed
    def done?
      @cursor >= @ops.size
    end

    # the op the cursor currently points at (nil when done)
    def current_op
      @ops[@cursor]
    end
  end

  # A compiled GPU binary loaded into a context (zeModuleCreate takes SPIR-V or
  # native code). Kernels are the individual entry points inside it.
  #
  # A Kernel has no context of its own, so "which context does this kernel
  # belong to?" is answered by kernel.module.context -- the indirection
  # check_kernel_list_context_match relies on.
  class Module < Object
    @typename = 'module'

    # The compiler diagnostics object optionally produced alongside a module.
    # It is a separately destroyable handle, so failing to destroy it is its own
    # leak; @module may be nil when the build FAILED (no module was produced,
    # but the log describing why still exists).
    class BuildLog < Object
      @typename = 'module_build_log'
      attr_reader :module

      def initialize(handle, mod = nil)
        super(handle)
        @module = mod
      end
    end

    attr_reader :context
    attr_reader :device
    attr_reader :desc
    attr_accessor :build_log
    attr_reader :kernels

    def initialize(handle, context, device, desc)
      super(handle)
      @context = context
      @device = device
      @desc = desc
      @kernels = {}
    end
  end

  # One entry point within a Module, created by zeKernelCreate. @name is the
  # source-level function name, kept so diagnostics can say which kernel
  # misbehaved instead of only printing a handle.
  class Kernel < Object
    @typename = 'kernel'
    attr_reader :module  # owning Module -- also how the kernel's context is found
    attr_reader :desc
    attr_reader :name    # human-readable kernel name from the descriptor

    def initialize(handle, mod, desc, name)
      super(handle)
      @module = mod
      @desc = desc
      @name = name
    end
  end

  # One in-flight API call: pushed on a thread's stack at _entry and popped at
  # _exit. @params holds the entry payload -- the call's INPUT arguments -- which
  # is why exit callbacks reach back through find_param to read inputs that the
  # exit event itself does not carry.
  class ApiCall
    attr_reader :name    # e.g. "zeCommandListAppendMemoryCopy"
    attr_reader :params  # the decoded _entry payload

    def initialize(name, params)
      @name = name
      @params = params
    end
  end

  # One OS thread of the traced process, identified by its LTTng vtid.
  class Thread
    attr_reader :vtid
    # CHANGED: was a single `last_entry` slot, which broke when a traced API
    # internally calls another traced API on the same thread (e.g.
    # zelLoaderDriverCheck calls zeInit): the inner entry clobbered the outer
    # frame and the outer _exit then failed check_last_entry. Modeling it as a
    # stack lets nested calls push/pop correctly; the top of stack is the
    # currently-executing call.
    attr_reader :call_stack

    def initialize(vtid)
      @vtid = vtid
      @call_stack = []
    end

    # the innermost in-flight ApiCall, or nil if the thread has none.
    # Kept as `last_entry` so existing callers (find_param, etc.) are unchanged.
    def last_entry
      @call_stack.last
    end
  end

  # One traced process. This is the main container: every Level Zero handle is
  # only meaningful within the process that created it, so all the object tables
  # live here rather than globally.
  #
  # The tables are reached generically -- #objects('command_list') and
  # StateObject#find_objects both resolve the name to the matching @...s ivar --
  # which is what lets the leak reporter loop over object types by name.
  class Process
    attr_reader :vpid     # LTTng virtual pid
    attr_reader :threads  # tid -> Thread (auto-created on first sight)
    # handle -> object, one table per Level Zero object type
    attr_reader :drivers
    attr_reader :devices
    attr_reader :contexts
    attr_reader :event_pools
    attr_reader :events
    attr_reader :command_queues
    attr_reader :fences
    attr_reader :command_lists
    attr_reader :modules
    attr_reader :module_build_logs
    # ADDED: address -> freed Memory objects (kept after zeMemFree) so a later
    # reference to a released address can be flagged as use-after-free.
    attr_reader :freed_memory_allocations
    # ADDED: both memory maps are nested by Level Zero context handle --
    # { ctx_handle => { address => Memory } } -- because the L0 unified virtual
    # address space only guarantees non-aliasing addresses WITHIN a context.
    # Two live allocations in different contexts may share a numeric address, so
    # a flat address-keyed map would let the second overwrite the first. Each
    # inner sub-map has the same shape as the old flat map, so code that already
    # holds a sub-map (allocations[ptr], .each_value, .delete) is unchanged.
    attr_reader :memory_allocations

    def initialize(vpid)
      @vpid = vpid
      @threads = Hash.new { |h, k| h[k] = Thread.new(k) }
      #can it model memory imports/exports??
      @drivers = {}
      @event_dependencies = {} #for detecting deadlocks
      @devices = {}
      @contexts = {}
      @event_pools = {}
      @events = {}
      @command_queues = {}
      @fences = {}
      @command_lists = {}
      @modules = {}
      @module_build_logs = {}
      @kernels = {}
      # CHANGED: nested by context handle -- ctx_handle -> { address -> Memory }.
      # Auto-vivify an empty sub-map on first use of a context so callers never
      # get nil for a context that has not allocated yet.
      @memory_allocations = Hash.new { |h, k| h[k] = {} }
      @freed_memory_allocations = Hash.new { |h, k| h[k] = {} } # ADDED: ctx -> {addr -> freed Memory}
      #@initCalled = false
    end

    # Generic accessor: objects('command_list') returns @command_lists. The
    # singular type names come from the model's typename strings, so a caller
    # can iterate ['context', 'fence', 'command_list', ...] and reach each table
    # without a case statement -- see StateObject#check_issues.
    def objects(type)
      instance_variable_get(:"@#{type}s")
    end
  end

  # One machine in the trace, keyed by hostname. A multi-node MPI run produces
  # one trace per node; they are merged into a single timestamp-ordered stream,
  # so the top of the model is keyed by hostname to keep them apart.
  class Node
    attr_reader :name       # hostname
    attr_reader :processes  # pid -> Process (auto-created on first sight)

    def initialize(name)
      @name = name
      @processes = Hash.new { |h, k| h[k] = Process.new(k) }
    end
  end

end