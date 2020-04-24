using ZE::ZERefinements
module ZE
  class Pointer < FFI::Pointer
    def initialize(*args)
      if args.length == 2 then
        super(ZE::find_type(args[0]), args[1])
      else
        super(*args)
      end
    end
  end

  class Function < FFI::Function
    def initialize(ret, args, *opts, &block)
      super(ZE::find_type(ret), args.collect { |a| ZE::find_type(a) }, *opts, &block)
    end
  end

  class MemoryPointer < FFI::MemoryPointer

    def initialize(size, count = 1, clear = true)
      if size.is_a?(Symbol)
        size = ZE::find_type(size)
      end
      super(size, count, clear)
    end

  end

  UINT32_MAX = 0xffffffff
  ZE_OBJECTS = {}
  ZE_OBJECTS_MUTEX = Mutex.new

  def self.register(handle, destructor)
    if handle
      ZE_OBJECTS_MUTEX.synchronize {
        ZE_OBJECTS[handle] = destructor
      }
    else
      raise "invalid handle"
    end
  end

  def self.unregister(handle)
    if handle
      ZE_OBJECTS_MUTEX.synchronize {
        d = ZE_OBJECTS.delete(handle) { raise "non-existing handle" }
        result = method(d).call(handle)
        ZE.error_check(result)
      }
    else
      raise "invalid handle"
    end
  end

  at_exit {
    ZE_OBJECTS_MUTEX.synchronize {
      ZE_OBJECTS.to_a.reverse.each do |h, d|
        result = method(d).call(h)
        ZE.error_check(result)
      end
      ZE_OBJECTS.clear
    }
  }

  ENV["ZE_ENABLE_VALIDATION_LAYER"] = "1"
  ENV["ZE_ENABLE_LOADER_INTERCEPT"] = "1"
  ENV["ZE_ENABLE_PARAMETER_VALIDATION"] = "1"
  if ENV["LIBZE_LOADER_SO"]
    ffi_lib ENV["LIBZE_LOADER_SO"]
  else
    begin
      ffi_lib "ze_loader"
    rescue FFI::LoadError
      ffi_lib "libze_loader.so.0.91"
    end
  end

  class Error < StandardError
  end

  def self.error_check(result)
    if result != :ZE_RESULT_SUCCESS &&
       result != :ZE_ZE_RESULT_NOT_READY
      raise Error, result
    end
  end

  class Object
    attr_reader :handle
    def initialize(handle)
      @handle = handle
    end

    def to_ptr
      @handle
    end

    def self.process_property_name(pname)
      res = pname.to_s.split("_").collect(&:capitalize).join
      res.gsub("Uuid","UUID").gsub("Dditable", "DDITable").gsub(/\AFp/, "FP").gsub("Ipc", "IPC").gsub("P2p", "P2P")
    end

    def self.add_property(pname, sname: nil, fname: nil)
      n = name.split("::").last
      ppn = process_property_name(pname) unless sname && fname
      sname = "ZE" << n << ppn unless sname
      fname = "ze#{n}Get" << ppn unless fname
      src = <<EOF
    def #{pname}
      return @#{pname} if @#{pname}
      #{pname} = #{sname}::new
      result = ZE.#{fname}(@handle, #{pname})
      ZE.error_check(result)
      @#{pname} = #{pname}
    end
EOF
      eval src
    end

    def self.add_object_array(aname, oname, fname)
      src = <<EOF
    def #{aname}
      return @#{aname} if @#{aname}
      pCount = MemoryPointer::new(:uint32_t)
      result = ZE.#{fname}(@handle, pCount, nil)
      ZE.error_check(result)
      count = pCount.read(:uint32)
      return [] if count == 0
      pArr = MemoryPointer::new(:pointer, count)
      result = ZE.#{fname}(@handle, pCount, pArr)
      ZE.error_check(result)
      @#{aname} = pArr.read_array_of_pointer(count).collect { |h| #{oname}::new(h) }
    end
EOF
      eval src
    end

    def self.add_array_property(aname, sname, fname)
      src = <<EOF
    def #{aname}
      return @#{aname} if @#{aname}
      pCount = MemoryPointer::new(:uint32_t)
      result = ZE.#{fname}(@handle, pCount, nil)
      ZE.error_check(result)
      count = pCount.read(:uint32)
      return [] if count == 0
      pArr = MemoryPointer::new(#{sname}, count)
      sz = #{sname}.size
      #{aname} = count.times.collect { |i| #{sname}::new(pArr.slice(sz * i, sz)) }
      result = ZE.#{fname}(@handle, pCount, #{aname}.first)
      ZE.error_check(result)
      @#{aname} = #{aname}
    end
EOF
      eval src
    end
  end

  class ManagedObject < Object
    class << self
      attr_reader :destructor
    end

    def initialize(handle)
      super
      ZE.register(handle, self.class.destructor)
    end

    def destroy
      handle = @handle
      @handle = nil
      ZE.unregister(handle)
      nil
    end
  end

  class Driver < Object
    add_property :api_version, sname: :ZEApiVersion
    add_property :properties
    add_property :ipc_properties

    def devices
      return @devices if @devices
      pCount = MemoryPointer::new(:uint32_t)
      result = ZE.zeDeviceGet(@handle, pCount, nil)
      ZE.error_check(result)
      count = pCount.read(:uint32)
      return [] if count == 0
      pArr = MemoryPointer::new(:pointer, count)
      result = ZE.zeDeviceGet(@handle, pCount, pArr)
      ZE.error_check(result)
      @devices = pArr.read_array_of_pointer(count).collect { |h| Device::new(h, self) }
    end

    def alloc_host_mem(size, alignment: 0, flags: 0)
      pptr = MemoryPointer::new(:pointer)
      desc = ZEHostMemAllocDesc::new
      desc[:flags] = flags
      result = ZE.zeDriverAllocHostMem(@handle, desc, size, alignment, pptr)
      ZE.error_check(result)
      pptr.read_pointer.slice(0, size)
    end

    def alloc_device_mem(size, device, alignment: 0, ordinal: 0, flags: 0)
      pptr = MemoryPointer::new(:pointer)
      desc = ZEDeviceMemAllocDesc::new
      desc[:flags] = flags
      desc[:ordinal] = ordinal
      result = ZE.zeDriverAllocDeviceMem(@handle, desc, size, alignment, device, pptr)
      ZE.error_check(result)
      pptr.read_pointer.slice(0, size)
    end

    def alloc_shared_mem(size, device, alignment: 0, ordinal: 0, host_flags: 0, device_flags: 0)
      pptr = MemoryPointer::new(:pointer)
      host_desc = ZEHostMemAllocDesc::new
      device_desc = ZEDeviceMemAllocDesc::new
      host_desc[:flags] = host_flags
      device_desc[:flags] = device_flags
      device_desc[:ordinal] = ordinal
      result = ZE.zeDriverAllocSharedMem(@handle, device_desc, host_desc, size, alignment, device, pptr)
      pptr.read_pointer.slice(0, size)
    end

    def free_mem(ptr)
      result = ZE.zeDriverFreeMem(@handle, ptr)
      ZE.error_check(result)
      return self
    end
  end

  class Device < Object
    attr_reader :driver

    def initialize(handle, driver)
      super(handle)
      @driver = driver
    end

    add_property :properties
    add_object_array :sub_devices, :Device, :zeDeviceGetSubDevices
    add_property :compute_properties
    add_property :kernel_properties
    add_array_property :memory_properties, :ZEDeviceMemoryProperties, :zeDeviceGetMemoryProperties
    add_property :memory_access_properties
    add_property :cache_properties
    add_property :image_properties

    def sub_devices
      return @sub_devices if @sub_devices
      pCount = MemoryPointer::new(:uint32_t)
      result = ZE.zeDeviceGetSubDevices(@handle, pCount, nil)
      ZE.error_check(result)
      count = pCount.read(:uint32)
      return [] if count == 0
      pArr = MemoryPointer::new(:pointer, count)
      result = ZE.zeDeviceGetSubDevices(@handle, pCount, pArr)
      ZE.error_check(result)
      @sub_devices = pArr.read_array_of_pointer(count).collect { |h| Device::new(h, @driver) }
    end

    def p2p_properties(device)
      p2p_properties = ZEDeviceP2PProperties::new
      result = ZE.zeDeviceGetP2PProperties(@handle, device, p2p_properties)
      ZE.error_check(result)
      return p2p_properties
    end

    def can_access_peer?(device)
      can_access_peer = MemoryPointer::new(:ze_bool_t)
      result = ZE.zeDeviceCanAccessPeer(@handle, device, can_access_peer)
      ZE.error_check(result)
      return can_access_peer.read_ze_bool_t == 1
    end

    def set_last_level_cache_config(cache_config = :ZE_CACHE_CONFIG_DEFAULT)
      result = ZE.zeDeviceSetLastLevelCacheConfig(@handle, cache_config)
      ZE.error_check(result)
      cache_config
    end

    def last_level_cache_config=(cache_config)
      set_last_level_cache_config(cache_config)
    end

    def command_queue_create(flags: 0, mode: 0, priority: 0)
      desc = ZECommandQueueDesc::new
      desc[:flags] = flags
      desc[:mode] = mode
      desc[:priority] = priority
      phCommandQueue = MemoryPointer::new(:ze_command_queue_handle_t)
      result = ZE.zeCommandQueueCreate(@handle, desc, phCommandQueue)
      ZE.error_check(result)
      CommandQueue::new(phCommandQueue.read_ze_command_queue_handle_t)
    end

    def command_list_create(flags: 0)
      desc = ZECommandListDesc::new
      desc[:flags] = flags
      phCommandList = MemoryPointer::new(:ze_command_list_handle_t)
      result = ZE.zeCommandListCreate(@handle, desc, phCommandList)
      ZE.error_check(result)
      CommandList::new(phCommandList.read_ze_command_list_handle_t)
    end

    def command_list_create_immediate(flags: 0, mode: 0, priority: 0)
      desc = ZECommandQueueDesc::new
      desc[:flags] = flags
      desc[:mode] = mode
      desc[:priority] = priority
      phCommandList = MemoryPointer::new(:ze_command_list_handle_t)
      result = ZE.zeCommandListCreateImmediate(@handle, desc, phCommandList)
      ZE.error_check(result)
      CommandList::new(phCommandList.read_ze_command_list_handle_t)
    end

    def image_create(width, height = 0, depth = 0,
                     arraylevels: 0,
                     miplevels: 0,
                     flags: :ZE_IMAGE_FLAG_PROGRAM_READ,
                     type: nil,
                     format_layout: :ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                     format_type: :ZE_IMAGE_FORMAT_TYPE_UINT,
                     format_x: :ZE_IMAGE_FORMAT_SWIZZLE_R,
                     format_y: :ZE_IMAGE_FORMAT_SWIZZLE_G,
                     format_z: :ZE_IMAGE_FORMAT_SWIZZLE_B,
                     format_w: :ZE_IMAGE_FORMAT_SWIZZLE_A)
      if !type
        type = _get_image_type(width, height, depth, arraylevels)
      end
      desc = ZEImageDesc::new
      desc[:flags] = flags
      desc[:type] = type
      desc[:format][:layout] = format_layout
      desc[:format][:type] = format_type
      desc[:format][:x] = format_x
      desc[:format][:y] = format_y
      desc[:format][:z] = format_z
      desc[:format][:w] = format_w
      desc[:width] = width
      desc[:height] = height
      desc[:depth] = depth
      desc[:arraylevels] = arraylevels
      desc[:miplevels] = miplevels
      phImage = MemoryPointer::new(:ze_image_handle_t)
      result = ZE.zeImageCreate(@handle, desc, phImage)
      ZE.error_check(result)
      Image::new(phImage.read_ze_image_handle_t)
    end

    def image_properties(width, height = 0, depth = 0,
                         arraylevels: 0,
                         miplevels: 0,
                         flags: :ZE_IMAGE_FLAG_PROGRAM_READ,
                         type: nil,
                         format_layout: :ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,
                         format_type: :ZE_IMAGE_FORMAT_TYPE_UINT,
                         format_x: :ZE_IMAGE_FORMAT_SWIZZLE_R,
                         format_y: :ZE_IMAGE_FORMAT_SWIZZLE_G,
                         format_z: :ZE_IMAGE_FORMAT_SWIZZLE_B,
                         format_w: :ZE_IMAGE_FORMAT_SWIZZLE_A)
      if !type
        type = _get_image_type(width, height, depth, arraylevels)
      end
      desc = ZEImageDesc::new
      desc[:flags] = flags
      desc[:type] = type
      desc[:format][:layout] = format_layout
      desc[:format][:type] = format_type
      desc[:format][:x] = format_x
      desc[:format][:y] = format_y
      desc[:format][:z] = format_z
      desc[:format][:w] = format_w
      desc[:width] = width
      desc[:height] = height
      desc[:depth] = depth
      desc[:arraylevels] = arraylevels
      desc[:miplevels] = miplevels
      props = ZEImageProperties::new
      result = ZE.zeImageGetProperties(handle, desc, props)
      ZE.error_check(result)
      props
    end

    def system_barrier
      result = ZE.zeDeviceSystemBarrier(@handle)
      ZE.error_check(result)
      self
    end

    def module_create(input_module,
                      format: :ZE_MODULE_FORMAT_IL_SPIRV,
                      build_flags: "",
                      constants: nil)
      consts = ZEModuleConstants::new
      consts[:numConstants] = 0
      desc = ZEModuleDesc::new
      input_size = input_module.bytesize
      p_input_module = MemoryPointer::new(input_size)
      p_input_module.write_bytes(input_module)
      p_build_flags = MemoryPointer.from_string(build_flags)
      desc[:format] = format
      desc[:inputSize] = input_size
      desc[:pInputModule] = p_input_module
      desc[:pBuildFlags] = p_build_flags
      desc[:pConstants] = consts
      ph_module = MemoryPointer::new(:ze_module_handle_t)
      ph_build_log = MemoryPointer::new(:ze_module_build_log_handle_t)
      result = ZE.zeModuleCreate(@handle, desc, ph_module, ph_build_log)
      ZE.error_check(result)
      [Module::new(ph_module.read_ze_module_handle_t, self),
       Module::BuildLog::new(ph_build_log.read_ze_module_build_log_handle_t)]
    end

    def free_mem(ptr)
      @driver.free_mem(ptr)
      self
    end

    def alloc_mem(size, alignment: 0, ordinal: 0, flags: 0)
      @driver.alloc_device_mem(size, @handle, alignment: alignment, ordinal: ordinal, flags: flags)
    end

    def alloc_shared_mem(size, alignment: 0, ordinal: 0, host_flags: 0, device_flags: 0)
      @driver.alloc_shared_mem(size, @handle, alignment: alignment, ordinal: ordinal, host_flags: host_flags, device_flags: device_flags)
    end

    private
    def _get_image_type(width, height, depth, arraylevels)
      if arraylevels > 0
        if height > 0
          type = :ZE_IMAGE_TYPE_2DARRAY
        else
          type = :ZE_IMAGE_TYPE_1DARRAY
        end
      else
        if height > 0
          if depth > 0
            type = :ZE_IMAGE_TYPE_3D
          else
            type = :ZE_IMAGE_TYPE_2D
          end
        else
          type = :ZE_IMAGE_TYPE_1D
        end
      end
      type
    end

  end

  class CommandQueue < ManagedObject
    @destructor = :zeCommandQueueDestroy

    def execute_command_lists(command_lists, fence: nil)
      result = ZE.zeCommandQueueExecuteCommandLists(@handle, [command_lists].flatten, fence)
      ZE.error_check(result)
      self
    end

    def synchronize(timeout: ZE.UINT32_MAX)
      result = ZE.zeCommandQueueSynchronize(@handle, timeout)
      ZE.error_check(result)
      result
    end
  end

  class CommandList < ManagedObject
    @destructor = :zeCommandListDestroy

    def close
      result = ZE.zeCommandListClose(@handle)
      ZE.error_check(result)
      self
    end

    def reset
      result = ZE.zeCommandListReset(@handle)
      ZE.error_check(result)
      self
    end

    def append_barrier(signal_event: nil, wait_events: nil)
      count, phWaitEvents = _create_event_list(wait_events)
      result = ZE.zeCommandListAppendBarrier(@handle, signal_event, count, phWaitEvents)
      ZE.error_check(result)
      self
    end

    def append_memory_ranges_barrier(*ranges, signal_event: nil, wait_events: nil)
      count, phWaitEvents = _create_event_list(wait_events)
      num_ranges = ranges.length
      if num_ranges > 0
        pRangesSizes = MemoryPointer::new(:size_t, num_ranges)
        pRanges = MemoryPointer::new(:pointer, num_ranges)
        rranges = ranges.collect { |r|
          case r
          when Array
            [r.first, r.last]
          when Range
            [r.size, r.first]
          else
            ZE.error_check(:ZE_RESULT_ERROR_INVALID_ARGUMENT)
          end
        }
        sizes, starts = rranges.transpose
        p sizes
        p starts
        pRangesSizes.write_array_of_size_t(sizes)
        pRanges.write_array_of_pointer(starts)
      else
        pRanges = nil
        pRangesSizes = nil
      end
      result = ZE.zeCommandListAppendMemoryRangesBarrier(@handle, num_ranges, pRangesSizes, pRanges, signal_event, count, phWaitEvents)
      ZE.error_check(result)
      self
    end

    def append_launch_kernel(kernel, x, y = 1, z = 1, signal_event: nil, wait_events: nil)
      count, phWaitEvents = _create_event_list(wait_events)
      group_count = ZEGroupCount::new
      group_count[:groupCountX] = x
      group_count[:groupCountY] = y
      group_count[:groupCountZ] = z
      result = ZE.zeCommandListAppendLaunchKernel(@handle, kernel, group_count, signal_event, count, phWaitEvents)
      ZE.error_check(result)
      self
    end

    private
    def _create_event_list(wait_events)
      count = 0
      phWaitEvents = nil
      if wait_events
        events = [wait_events].flatten
        count = events.length
        if count > 0
          phWaitEvents = MemoryPointer::new(:ze_event_handle_t, count)
          phWaitEvents.write_array_of_event_handle_t(events.collect(&:handle).collect(&:address))
        end
      end
      [count, phWaitEvents]
    end
  end

  class Image < ManagedObject
    @destructor = :zeImageDestroy
  end

  class Module < ManagedObject
    @destructor = :zeModuleDestroy
    attr_reader :device
    class BuildLog < ManagedObject
      @destructor = :zeModuleBuildLogDestroy

      def string
        p_size = MemoryPointer::new(:size_t)
        result = ZE.zeModuleBuildLogGetString(@handle, p_size, nil)
        ZE.error_check(result)
        sz = p_size.read_size_t
        p_build_log = MemoryPointer::new(sz)
        result = ZE.zeModuleBuildLogGetString(@handle, p_size, p_build_log)
        ZE.error_check(result)
        return p_build_log.read_string(sz)
      end
      alias to_s string
    end

    def initialize(handle, device)
      super(handle)
      @device = device
    end

    def native_binary
      p_size = MemoryPointer::new(:size_t)
      result = ZE.zeModuleGetNativeBinary(@handle, p_size, nil)
      ZE.error_check(result)
      sz = p_size.read_size_t
      p_binary = MemoryPointer::new(sz)
      result = ZE.zeModuleGetNativeBinary(@handle, p_size, p_binary)
      ZE.error_check(result)
      return p_binary.read_bytes(sz)
    end

    def global_pointer(name)
      pptr = @device.alloc_shared_mem(8)
      p_name = MemoryPointer::from_string(name)
      result = ZE.zeModuleGetGlobalPointer(@handle, p_name, pptr)
      ZE.error_check(result)
      return pptr.read_pointer
    end

    def kernel_names
      p_count = MemoryPointer::new(:uint32)
      result = ZE.zeModuleGetKernelNames(@handle, p_count, nil)
      ZE.error_check(result)
      count  = p_count.read_uint32
      p_names = MemoryPointer::new(:pointer, count)
      result = ZE.zeModuleGetKernelNames(@handle, p_count, p_names)
      ZE.error_check(result)
      p_names.read_array_of_pointer(count).collect { |ptr| ptr.read_string }
    end

    def kernel_create(name, flags: 0)
      desc = ZEKernelDesc::new
      desc[:flags] = flags
      desc[:pKernelName] = MemoryPointer::from_string(name)
      ph_kernel = MemoryPointer::new(:ze_kernel_handle_t)
      result = ZE.zeKernelCreate(@handle, desc, ph_kernel)
      ZE.error_check(result)
      Kernel::new(ph_kernel.read_ze_kernel_handle_t)
    end

    def function_pointer(name)
      pptr = MemoryPointer::new(:pointer)
      p_name = MemoryPointer::from_string(name)
      result = ZE.zeModuleGetFunctionPointer(@handle, p_name, pptr)
      ZE.error_check(result)
      return pptr.read_pointer
    end

  end

  class Kernel < ManagedObject
    @destructor = :zeKernelDestroy
    add_property :properties

    def set_group_size(x, y = 1, z = 1)
      result = ZE.zeKernelSetGroupSize(@handle, x, y, z)
      ZE.error_check(result)
      return self
    end

    def suggest_group_size(x, y = 1, z = 1)
      ptr = MemoryPointer::new(:uint32, 3)
      result = ZE.zeKernelSuggestGroupSize(@handle, x, y, z, ptr, ptr + 4, ptr + 8)
      ZE.error_check(result)
      ptr.read_array_of_uint32(3)
    end

    def suggest_max_cooperative_group_count
      ptr = MemoryPointer::new(:uint32)
      result = ZE.zeKernelSuggestMaxCooperativeGroupCount(@handle, ptr)
      ZE.error_check(result)
      ptr.read_uint32
    end

    def set_argument_value(index, ptr, size: nil)
      sz = size
      sz = ptr.to_ptr.size unless sz
      result = ZE.zeKernelSetArgumentValue(@handle, index, sz, ptr)
      ZE.error_check(result)
      self
    end

    def set_attribute(attr, ptr, size: nil)
      sz = size
      sz = ptr.to_ptr.size unless sz
      result = ZE.zeKernelSetAttribute(@handle, attr, sz, ptr)
      ZE.error_check(result)
      self
    end

    # WRNING untested, zeKernelGetAttribute buggy on latest intel driver
    def attribute(attr)
      p_size = MemoryPointer::new(:uint32)
      result = ZE.zeKernelGetAttribute(@handle, attr, p_size, nil)
      ZE.error_check(result)
      sz = p_size.read_uint32
      return nil if sz == 0
      p_value = MemoryPointer::new(sz)
      result = ZE.zeKernelGetAttribute(@handle, attr, p_size, p_value)
      case attr
      when :ZE_KERNEL_ATTR_INDIRECT_HOST_ACCESS, :ZE_KERNEL_ATTR_INDIRECT_DEVICE_ACCESS, :ZE_KERNEL_ATTR_INDIRECT_SHARED_ACCESS
        p_value.read_ze_bool_t
      when :ZE_KERNEL_ATTR_SOURCE_ATTRIBUTE
        p_value.read_string(sz)
      else
        p_value
      end
    end

    def set_intermediate_cache_config(config)
      result = ZE.zeKernelSetIntermediateCacheConfig(@handle, config)
      ZE.error_check(result)
      self
    end
  end

  def self.ze_init(flags: 0)
    result = zeInit(flags)
    error_check(result)
    nil
  end

  def self.zet_init(flags: 0)
    result = zetInit(flags)
    error_check(result)
    nil
  end

  def self.init(flags: 0)
    ze_init(flags: flags)
    zet_init(flags: flags)
    nil
  end

  def self.drivers
    return @drivers if @drivers
    pCount = MemoryPointer::new(:uint32_t)
    result = zeDriverGet(pCount, nil);
    error_check(result)
    count = pCount.read(:uint32)
    return [] if count == 0
    phDrivers = MemoryPointer::new(:ze_driver_handle_t, count)
    result = zeDriverGet(pCount, phDrivers);
    error_check(result)
    @drivers = phDrivers.read_array_of_ze_driver_handle_t(count).collect { |h| Driver::new(h) }
  end

end
