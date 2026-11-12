# frozen_string_literal: true

require 'ze/validator/allocation_map'
require 'set'

module ZEModel
  # One of these APIs must be called before any other calls
  INIT_API_NAMES = %w[zeInit zeInitDrivers].freeze

  # Which object owns which, following the containment of the specs
  OWNERSHIP = {
    'context'           => 'driver',
    'memory_allocation' => 'context',
    'event_pool'        => 'context',
    'command_queue'     => 'context',
    'command_list'      => 'context',
    'module'            => 'context',
    'module_build_log'  => 'context',
    'event'             => 'event_pool',
    'fence'             => 'command_queue',
    'kernel'            => 'module'
  }.freeze

  # OWNERSHIP inverted
  OWNED_TYPES = OWNERSHIP.each_with_object({}) { |(child, owner), h|
    (h[owner] ||= []) << child
  }.each_value(&:freeze).freeze

  # This defines the object in which most ze objects (command list, command queue) extend form
  class Object
    attr_reader :handle
    attr_accessor :status
    attr_accessor :leak_reported

    # returns the typename of the object: must match exactly 'OWNERSHIP' strings.
    # e.g., 'Device' object has :typename = 'device'
    class << self
      attr_reader :typename
    end

    def initialize(handle)
      @handle = handle
      @lock = nil
      @leak_reported = false
    end

    # The object that owns this one, or nil when it was never recorded.
    def owner
      owner_type = OWNERSHIP[self.class.typename]
      owner_type && instance_variable_get(:"@#{owner_type}")
    end

    # Yields every live child of one type.
    def each_child(type, &block)
      children(type).each_value(&block)
    end

    def child_count(type)
      children(type).size
    end

    # Lock the object. If it already has been locked, report a race.
    # This is typically to be used for thread safety checks on API parameters.
    def lock(state, ctx)
      if @lock
        state.print_race_condition(ctx, @lock, self.class.typename, @handle)
      else
        @lock = ctx
      end
    end

    # Unlock the object
    def unlock(ctx)
      return unless @lock == ctx

      @lock = nil
    end

    private

    # The container for a type is the instance variable named after its plural.
    # e.g., a 'Driver' has multiple 'Context' in the 'contexts' container member.
    def children(type)
      instance_variable_get(:"@#{type}s")
    end
  end

  class Driver < Object
    @typename = 'driver'
    attr_reader :devices
    attr_reader :contexts

    def initialize(handle)
      super
      @devices = []
      @contexts = {}
    end
  end

  class Device < Object
    @typename = 'device'
    attr_reader :sub_devices
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

  # One allocation returned by zeMemAllocDevice/Shared/Host. `memtypestr` keeps
  # device, shared and host allocations apart.
  class MemoryAllocation < Object
    @typename = 'memory_allocation'
    attr_reader :context, :size, :owned_by # the Device for a device allocation; nil for host
    attr_accessor :memtypestr, :base # "device" | "host" | "shared"
    attr_accessor :freed_by # the zeMemFree that released this allocation, nil while live.

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
    attr_reader :memory_allocations
    attr_reader :freed_memory_allocations

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
      @memory_allocations = AllocationMap.new
      @freed_memory_allocations = AllocationMap.new
    end
  end

  class EventPool < Object
    @typename = 'event_pool'

    attr_reader :context, :desc, :devices, :events

    # slot indices not yet in use:
    #   - zeEventCreate removes one (double use = error)
    #   - zeEventDestroy puts it back (double free = error)
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
    # :ordinal and :index are checked against the engine topology recorded by
    # the lttng_ust_ze_properties:command_queue_group tracepoint
    attr_reader :desc

    def initialize(handle, context, device, desc)
      super(handle)
      @context = context
      @device = device
      @desc = desc
      @fences = {}
    end
  end

  class Fence < Object
    @typename = 'fence'
    attr_reader :command_queue, :desc, :not_signaled, :in_use, :signaled

    # not_signaled -> in_use -> signaled -> not_signaled (zeFenceReset).
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
    attr_reader :context, :device, :desc, :altdesc
    attr_accessor :associated_command_queue, :immediate
    attr_accessor :in_order
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
      @in_order = false
      @ops = []
    end

    def queue_group_ordinal
      return @desc[:commandQueueGroupOrdinal] if @desc
      return @altdesc[:ordinal] if @altdesc

      0
    end
  end

  class RecordedOp
    # :copy, :wait, :signal, :reset, :barrier, :ranges_barrier or :launch
    attr_reader :kind
    attr_reader :signal # event this op signals on completion (nil if none)
    attr_reader :waits # events that must be signaled before this op may run
    attr_reader :params, :api

    def initialize(kind, signal: 0, waits: [], params: {}, api: nil)
      @kind = kind
      @signal = signal == 0 ? nil : signal  # normalize 0 to nil
      @waits = waits
      @params = params
      @api = api || params[:api]
    end
  end

  class DeferredUnit
    attr_reader :ops        # snapshot of the list's ops for this execution
    attr_reader :context    # trace context captured at submit time
    attr_reader :label      # e.g. "command_list (0x00007f...)
    attr_reader :in_order

    attr_accessor :cursor     # index of the next op to run; == ops.size means done
    attr_accessor :blocked_on # events the current op is still waiting for

    # Events this unit has not signaled yet. If unit U is blocked on an event
    # only in V's pending_signals, U waits on V: an edge in the wait-for graph.
    attr_accessor :pending_signals

    # the command list this unit came from, so list-scoped checks can find their
    # units without matching on the label string
    attr_reader :cmd_list_handle

    # true when this unit is the running tail of an immediate list rather than a
    # queue submission; only the latter counts as in-flight for a reset
    attr_reader :immediate

    def initialize(ops, context, label, in_order: false, cmd_list_handle: nil, immediate: false)
      @ops = ops
      @context = context
      @label = label
      @cursor = 0
      @blocked_on = []
      @in_order = in_order
      @cmd_list_handle = cmd_list_handle
      @immediate = immediate
      # every event this unit will eventually signal, for the wait-for graph
      @pending_signals = ops.map(&:signal).compact
    end

    # Queues one more op behind the ones already here.
    def push_op(new_op)
      @ops << new_op
      @pending_signals << new_op.signal if new_op.signal
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
      attr_reader :context
      attr_reader :module # nil when the build failed and produced no module

      def initialize(handle, context, mod = nil)
        super(handle)
        @context = context
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
    attr_reader :module, :desc, :name

    def initialize(handle, mod, desc, name)
      super(handle)
      @module = mod
      @desc = desc
      @name = name
    end
  end

  class ApiCall
    attr_reader :name, :params

    def initialize(name, params)
      @name = name
      @params = params
    end
  end

  class Thread
    attr_reader :vtid

    # call stack: a traced API may call another traced API on the same thread
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

    # handle -> object, one table per Level Zero object type
    attr_reader :vpid
    attr_reader :threads
    attr_reader :drivers, :devices, :contexts, :kernels, :event_pools, :events, :command_queues, :fences, :command_lists, :modules, :module_build_logs

    def initialize(vpid)
      @vpid = vpid
      @threads = Hash.new { |h, k| h[k] = Thread.new(k) }
      @drivers = {}
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
    end

    # objects('command_list') returns @command_lists, so callers can iterate
    # object types by name (see StateObject#finalize).
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
