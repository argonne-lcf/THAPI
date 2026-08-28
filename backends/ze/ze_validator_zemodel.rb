# frozen_string_literal: true

require 'set'

module ZEModel
  # One of these APIs must be called before any other calls
  INIT_API_NAMES = %w[zeInit zeInitDrivers].freeze
  # This defines the object in which most ze objects (command list, command queue) extend form
  class Object
    attr_reader :handle
    attr_accessor :status

    # returns what object the caller is
    # e.g., 'Device' will return device
    class << self
      attr_reader :typename
    end

    # lock is needed to check for concurrent properties.
    # e.g., calling the same APIs that can be called from simultaneous threads (zeCommandListAppendBarrier)
    def initialize(handle)
      @handle = handle
      @lock = nil
    end

    # Reports a race: the trace is timestamp-ordered, so finding the object
    # already locked means two calls really overlapped.
    def lock(state, ctx)
      if @lock
        state.print_race_condition(ctx, @lock, self.class.typename, @handle)
      else
        @lock = ctx
      end
    end

    # Releases only if this same call is the holder, so a call that lost the
    # race above does not steal the real holder's lock on its way out.
    def unlock(ctx)
      return unless @lock == ctx

      @lock = nil
    end
  end

  class Driver < Object
    @typename = 'driver'
    attr_reader :devices

    def initialize(handle)
      super
      @devices = []
    end
  end

  class Device < Object
    @typename = 'device'
    attr_reader :sub_devices
    # checks look at (see check_group_property_queued)
    attr_accessor :property_fetched
    attr_accessor :cmd_queue_group_properties_queried

    def initialize(handle)
      super
      @sub_devices = []
      @property_fetched = false
      @cmd_queue_group_properties_queried = false
    end
  end

  class SubDevice < Device
    attr_reader :parent

    def initialize(handle, parent)
      @parent = parent
      super(handle)
    end
  end

  # create memory object so that device, shared, host mem allocs can be differentiated
  class Memory < Object
    @typename = 'memory_allocation'
    attr_reader :context, :size, :owned_by # the Device for a device allocation; nil for host
    attr_accessor :memtypestr, :base # "device" | "host" | "shared"
    # the zeMemFree that released this allocation, nil while live. Freed
    # allocations are kept so a later reference is caught as use-after-free.
    attr_accessor :freed_by

    def initialize(handle, context, size, owned_by, memtypestr)
      super(handle)
      @context = context
      @size = size
      @owned_by = owned_by
      @memtypestr = memtypestr
      @base = handle
      @freed_by = nil
    end
  end

  class Context < Object
    @typename = 'context'
    attr_reader :driver, :desc, :devices, :event_pools, :command_queues, :command_lists, :modules, :module_build_logs

    def initialize(handle, driver, desc, devices = nil)
      super(handle)
      @driver = driver
      @desc = desc
      @devices = devices

      @event_pools = {}
      @command_queues = {}
      @command_lists = {}
      @modules = {} # binaries for gpu
      @module_build_logs = {}
    end
  end

  class EventPool < Object
    @typename = 'event_pool'
    attr_reader :context, :desc, :devices, :events
    # slot indices not yet in use: zeEventCreate removes one (double use =
    # error), zeEventDestroy puts it back (double free = error)
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

  class Event < Object
    @typename = 'event'
    attr_reader :event_pool, :desc, :signaled_by
    attr_accessor :signaled # who last signaled it, for diagnostics
    # whether the host observed the signaled state since the last signal. Tells
    # a concurrent double-signal (never consumed) from a reuse-without-reset.
    attr_reader :observed

    def initialize(handle, event_pool, desc)
      super(handle)
      @event_pool = event_pool
      @desc = desc
      # event can have 2 states, not signaled or signaled
      @signaled = false
      @signaled_by = nil
      @observed = false
    end

    # `by` records who signaled it, for messages
    def signal(by = nil)
      @signaled = true
      @signaled_by = by
      @observed = false
    end

    def reset
      @signaled = false
      @signaled_by = nil
      @observed = false
    end

    def observe
      @observed = true
    end
  end

  class CommandQueue < Object
    @typename = 'command_queue'
    attr_reader :context, :device, :fences
    # :ordinal and :index are checked against the topology in
    # ze_device_property.json
    attr_reader :desc

    def initialize(handle, context, device, desc)
      super(handle)
      @context = context
      @device = device
      @desc = desc
      @fences = {}
      @valid_fences = Hash.new { |h, k| h[k] = true } # fences that have been reset or haven't been signaled
    end
  end

  class Fence < Object
    @typename = 'fence'
    attr_reader :command_queue, :desc, :not_signaled, :in_use, :signaled
    # not_signaled -> in_use -> signaled -> not_signaled (zeFenceReset). Compared
    # as `fence.status == fence.signaled`; see check_fence_misuse.
    attr_accessor :status

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

  class CommandList < Object
    @typename = 'command_list'
    attr_reader :context, :device, :desc, :altdesc # nil for immediate lists  # queue descriptor, immediate lists only
    attr_accessor :associated_command_queue, :immediate, :associated_ordinal
    # enables check_in_order_self_deadlock: in an in-order list an op waiting on
    # an event only a later op in the same list signals can never complete
    attr_accessor :in_order
    # RecordedOps in append order, replayed when the list is executed so the
    # deferred checks run at the point the op would actually execute
    attr_accessor :ops

    @@INITIALIZED = 0 # created or being properly recycled
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
      @in_order = false
      @api_calls = []
      @ops = []
    end

    # An immediate list is given a queue descriptor instead of a list one, so a
    # nil desc identifies it.
    def immediate?
      !desc
    end
  end

  class RecordedOp
    # :copy, :wait, :signal, :reset, :barrier, :ranges_barrier or :launch
    attr_reader :kind
    attr_reader :signal, :waits, :params, :api # event this op signals on completion (nil if none)   # events that must be signaled before this op may run

    def initialize(kind, signal: 0, waits: [], params: {}, api: nil)
      @kind = kind
      # normalize a null (0) signal handle to nil so "does this op signal?" is a
      # simple truthiness test
      @signal = signal && signal != 0 ? signal : nil
      @waits = waits || []
      @params = params
      @api = api || params[:api]
    end
  end

  class DeferredUnit
    attr_reader :ops, :context, :label, :in_order # snapshot of the list's ops for this execution    # trace context captured at submit time      # e.g. "command_list (0x00007f...)", for messages
    attr_accessor :cursor, :blocked_on # index of the next op to run; == ops.size means done # events the current op is still waiting for
    # Events this unit has not signaled yet. If unit U is blocked on an event
    # only in V's pending_signals, U waits on V: an edge in the wait-for graph.
    attr_accessor :pending_signals
    # the command list this unit came from, so list-scoped checks can find their
    # units without matching on the label string
    attr_reader :cmd_list_handle

    def initialize(ops, context, label, in_order: false, cmd_list_handle: nil)
      @ops = ops
      @context = context
      @label = label
      @cursor = 0
      @blocked_on = []
      @in_order = in_order
      @cmd_list_handle = cmd_list_handle
      # every event this unit will eventually signal, for the wait-for graph
      @pending_signals = ops.map(&:signal).compact
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

  class Module < Object
    @typename = 'module'

    class BuildLog < Object
      @typename = 'module_build_log'
      attr_reader :module # nil when the build failed and produced no module

      def initialize(handle, mod = nil)
        super(handle)
        @module = mod
      end
    end

    attr_reader :context, :device, :desc, :kernels
    attr_accessor :build_log

    def initialize(handle, context, device, desc)
      super(handle)
      @context = context
      @device = device
      @desc = desc
      @kernels = {}
    end
  end

  class Kernel < Object
    @typename = 'kernel'
    attr_reader :module, :desc, :name # also how the kernel's context is found    # kernel name from the descriptor, for diagnostics

    def initialize(handle, mod, desc, name)
      super(handle)
      @module = mod
      @desc = desc
      @name = name
    end
  end

  class ApiCall
    attr_reader :name, :params # the _entry payload, i.e. the call's input arguments

    def initialize(name, params)
      @name = name
      @params = params
    end
  end

  class Thread
    attr_reader :vtid
    # a stack, not a single slot: a traced API may call another traced API on
    # the same thread (e.g. zelLoaderDriverCheck calls zeInit)
    attr_reader :call_stack

    def initialize(vtid)
      @vtid = vtid
      @call_stack = []
    end

    # the innermost in-flight ApiCall, or nil if the thread has none
    def last_entry
      @call_stack.last
    end
  end

  class Process
    attr_reader :vpid, :threads, :devices, :contexts, :event_pools, :events, :command_queues, :fences, :command_lists, :modules, :module_build_logs # LTTng virtual pid  # tid -> Thread (auto-created on first sight)
    # handle -> object, one table per Level Zero object type
    attr_reader :drivers
    # allocations kept after zeMemFree, for use-after-free detection
    attr_reader :freed_memory_allocations
    # { ctx_handle => { address => Memory } }: addresses are only guaranteed
    # non-aliasing within a context, so a flat map would lose one of two.
    attr_reader :memory_allocations

    def initialize(vpid)
      @vpid = vpid
      @threads = Hash.new { |h, k| h[k] = Thread.new(k) }
      # can it model memory imports/exports??
      @drivers = {}
      @event_dependencies = {} # for detecting deadlocks
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
      @memory_allocations = Hash.new { |h, k| h[k] = [] }
      @freed_memory_allocations = Hash.new { |h, k| h[k] = [] }
    end

    # objects('command_list') returns @command_lists, so callers can iterate
    # object types by name (see StateObject#check_issues).
    def objects(type)
      instance_variable_get(:"@#{type}s")
    end
  end

  class Node
    attr_reader :name, :processes # hostname  # pid -> Process (auto-created on first sight)

    def initialize(name)
      @name = name
      @processes = Hash.new { |h, k| h[k] = Process.new(k) }
    end
  end
end
