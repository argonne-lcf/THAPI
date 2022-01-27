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
  UINT64_MAX = 0xffffffffffffffff
  ZES_EVENT_WAIT_NONE = 0x0
  ZES_EVENT_WAIT_INFINITE = UINT32_MAX

  ZES_DIAG_FIRST_TEST_INDEX =  0x0
  ZES_DIAG_LAST_TEST_INDEX = 0xffffffff
  ZES_FAN_TEMP_SPEED_PAIR_COUNT = 32
  ZES_MAX_RAS_ERROR_CATEGORY_COUNT = 7

  ZET_DEBUG_TIMEOUT_INFINITE = 0xffffffffffffffff
  ZET_DEBUG_THREAD_NONE = 0xffffffffffffffff
  ZET_DEBUG_THREAD_ALL = 0xfffffffffffffffe

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

  ENV["ZE_ENABLE_VALIDATION_LAYER"] = "1"
  ENV["ZE_ENABLE_LOADER_INTERCEPT"] = "1"
  ENV["ZE_ENABLE_PARAMETER_VALIDATION"] = "1"
  ENV["ZET_ENABLE_API_TRACING_EXP"] = "1"
  ENV["ZET_ENABLE_METRICS"] = "1"
#  ENV["ZET_ENABLE_PROGRAM_INSTRUMENTATION"] = "1"
#  ENV["ZET_ENABLE_PROGRAM_DEBUGGING"] = "1"
  ENV["ZES_ENABLE_SYSMAN"] = "1"
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
       result != :ZE_RESULT_NOT_READY
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
      res.gsub("Uuid","UUID").gsub("Dditable", "DDITable").gsub(/\AFp/, "FP").gsub("P2p", "P2P")
    end

    def self.process_ffi_name(name)
      res = name.to_s.gsub(/_t\z/, "").split("_").collect(&:capitalize).join
      res.gsub(/\AZet/,"ZET").gsub(/\AZes/,"ZES").gsub(/\AZe/, "ZE").gsub("Uuid","UUID").gsub("Dditable", "DDITable").gsub(/\AFp/, "FP").gsub("P2p", "P2P")
    end

    def self.add_object_array(aname, oname, fname, memoize: false)
      src = <<EOF
    def #{aname}
EOF
      if memoize
        src << <<EOF
      return @#{aname} if @#{aname}
EOF
      end
      src << <<EOF
      pCount = MemoryPointer::new(:uint32_t)
      result = ZE.#{fname}(@handle, pCount, nil)
      ZE.error_check(result)
      count = pCount.read(:uint32)
      return [] if count == 0
      pArr = MemoryPointer::new(:pointer, count)
      result = ZE.#{fname}(@handle, pCount, pArr)
      ZE.error_check(result)
EOF
      if memoize
        src << <<EOF
      @#{aname} = pArr.read_array_of_pointer(count).collect { |h| #{oname}::new(h) }
    end
EOF
      else
        src << <<EOF
      pArr.read_array_of_pointer(count).collect { |h| #{oname}::new(h) }
    end
EOF
      end
      class_eval src
    end

    def self.add_array_property(aname, sname, fname, memoize: false)
      src = <<EOF
    def #{aname}
EOF
      if memoize
        src << <<EOF
      return @#{aname} if @#{aname}
EOF
      end
      src << <<EOF
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
EOF
      if memoize
        src << <<EOF
      @#{aname} = #{aname}
    end
EOF
      else
        src << <<EOF
      #{aname}
    end
EOF
      end
      class_eval src
    end

    def self.add_enum_array_property(aname, ename, fname, memoize: false)
      pname = process_ffi_name(ename)
      src = <<EOF
    def #{aname}
EOF
      if memoize
        src << <<EOF
      return @#{aname} if @#{aname}
EOF
      end
      src << <<EOF
      p_count = MemoryPointer::new(:uint32_t)
      result = ZE.#{fname}(@handle, p_count, nil)
      ZE.error_check(result)
      count = p_count.read(:uint32)
      return [] if count == 0
      p_arr = MemoryPointer::new(:#{ename}, count)
      result = ZE.#{fname}(@handle, p_count, p_arr)
      ZE.error_check(result)
EOF
      if memoize
        src << <<EOF
      @#{aname} = p_arr.read_array_of_int32.collect { |i| #{pname}.from_native(i, nil) }
    end
EOF
      else
        src << <<EOF
      p_arr.read_array_of_int32.collect { |i| #{pname}.from_native(i, nil) }
    end
EOF
      end
      class_eval src
    end

    def self.add_property(pname, sname, fname, memoize: false)
      src = <<EOF
    def #{pname}
EOF
      if memoize
        src << <<EOF
      return @#{pname} if @#{pname}
EOF
      end
      src << <<EOF
      #{pname} = #{sname}::new
      result = ZE.#{fname}(@handle, #{pname})
      ZE.error_check(result)
EOF
      if memoize
        src << <<EOF
      @#{pname} = #{pname}
    end
EOF
      else
        src << <<EOF
      #{pname}
    end
EOF
      end
      class_eval src
    end
    def self.add_enum_property(pname, ename, fname, memoize: false)
      pename = process_ffi_name(ename)
      src = <<EOF
    def #{pname}
EOF
      if memoize
        src << <<EOF
      return @#{pname} if @#{pname}
EOF
      end
      src << <<EOF
      p_prop = MemoryPointer::new(:#{ename})
      result = ZE.#{fname}(@handle, p_prop)
      ZE.error_check(result)
EOF
      if memoize
        src << <<EOF
      @#{pname} = #{pename}.from_native(p_prop.read_int32, nil)
    end
EOF
      else
        src << <<EOF
      #{pename}.from_native(p_prop.read_int32, nil)
    end
EOF
      end
      class_eval src
    end
  end

  class ZEObject < Object
    def self.add_property(pname, sname: nil, fname: nil, memoize: false)
      n = name.split("::").last
      ppn = process_property_name(pname) unless sname && fname
      sname = "ZE" << n << ppn unless sname
      fname = "ze#{n}Get" << ppn unless fname
      super(pname, sname, fname, memoize: memoize)
    end
  end

  class ZETObject < Object
    def self.add_property(pname, sname: nil, fname: nil, memoize: false)
      n = name.split("::").last
      ppn = process_property_name(pname) unless sname && fname
      sname = "ZET" << n << ppn unless sname
      fname = "zet#{n}Get" << ppn unless fname
      super(pname, sname, fname, memoize: memoize)
    end
  end

  class ZESObject < Object
    def self.add_property(pname, sname: nil, fname: nil, memoize: false)
      n = name.split("::").last
      ppn = process_property_name(pname) unless sname && fname
      sname = "ZES" << n << ppn unless sname
      fname = "zes#{n}Get" << ppn unless fname
      super(pname, sname, fname, memoize: memoize)
    end
  end

  module Lifetime
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

  module Destructor
    attr_reader :destructor
  end

  class ZEManagedObject < ZEObject
    extend Destructor
    include Lifetime
  end

  class ZETManagedObject < ZETObject
    extend Destructor
    include Lifetime
  end

  class ZESManagedObject < ZESObject
    extend Destructor
    include Lifetime
  end

  class Driver < ZEObject
    add_property :api_version, sname: :ZEApiVersion, memoize: true
    add_property :properties, memoize: true
    add_property :ipc_properties, memoize: true

    def extension_properties
      @extension_properties ||= begin
        pCount = MemoryPointer::new(:uint32_t)
        result = ZE.zeDriverGetExtensionProperties(@handle, pCount, nil);
        ZE.error_check(result)
        count = pCount.read(:uint32)
        if count == 0
          []
        else
          pArr = MemoryPointer::new(:ze_driver_extension_properties_t, count)
          result = ZE.zeDriverGetExtensionProperties(@handle, pCount, pArr)
          ZE.error_check(result)
          sz = ZEDriverExtensionProperties.size
          count.times.collect { |i| ZEDriverExtensionProperties::new(pArr.slice(sz * i, sz)) }
        end
      end
    end

    def devices
      @devices ||= begin
        pCount = MemoryPointer::new(:uint32_t)
        result = ZE.zeDeviceGet(@handle, pCount, nil)
        ZE.error_check(result)
        count = pCount.read(:uint32)
        if count == 0
          []
        else
          pArr = MemoryPointer::new(:pointer, count)
          result = ZE.zeDeviceGet(@handle, pCount, pArr)
          ZE.error_check(result)
          pArr.read_array_of_pointer(count).collect { |h| Device::new(h, self) }
        end
      end
    end

    def context_create(flags: 0, devices: nil)
      desc = ZEContextDesc::new()
      desc[:flags] = flags
      ph_context = MemoryPointer::new(:ze_context_handle_t)
      unless devices
        result = ZE.zeContextCreate(@handle, desc, ph_context)
      else
        count = devices.length
        ph_devices = MemoryPointer::new(:ze_device_handle_t, count)
        ph_devices.write_array_of_ze_device_handle_t(devices.collect(&:handle).collect(&:address))
        result = ZE.zeContextCreateEx(@handle, desc, count, ph_devices, ph_context)
      end
      ZE.error_check(result)
      Context::new(ph_context.read_ze_context_handle_t, self)
    end

    def event_listen(*devices, timeout: ZET_EVENT_WAIT_NONE)
      count = devices.length
      ph_devices = MemoryPointer::new(:zes_device_handle_t, count)
      ph_devices.write_array_of_zes_device_handle_t(devices.collect(&:handle).collect(&:address))
      p_num_device_events = MemoryPointer::new(:uint32_t)
      p_events = MemoryPointer::new(:zes_event_type_flags_t, count)
      result = ZE.zesDriverEventListen(@handle, timeout, count, ph_devices, p_num_device_events, p_events)
      ZE.error_check(result)
      p_events.read_array_of_uint32(count).collect { |e| ZESEventTypeFlag.from_native(e, nil) }
    end

  end

  class Context < ZEManagedObject
    @destructor = :zeContextDestroy

    attr_reader :driver
    def initialize(handle, driver)
      super(handle)
      @driver = driver
    end

    def status
      result = ZE.zeContextGetStatus(@handle)
      ZE.error_check(result)
      result
    end

    def command_queue_create(device, ordinal: 0, index: 0, flags: 0, mode: 0, priority: 0)
      desc = ZECommandQueueDesc::new
      desc[:ordinal] = ordinal
      desc[:index] = index
      desc[:flags] = flags
      desc[:mode] = mode
      desc[:priority] = priority
      phCommandQueue = MemoryPointer::new(:ze_command_queue_handle_t)
      result = ZE.zeCommandQueueCreate(@handle, device, desc, phCommandQueue)
      ZE.error_check(result)
      CommandQueue::new(phCommandQueue.read_ze_command_queue_handle_t)
    end

    def command_list_create(device, command_queue_group_ordinal: 0, flags: 0)
      desc = ZECommandListDesc::new
      desc[:commandQueueGroupOrdinal] = command_queue_group_ordinal
      desc[:flags] = flags
      phCommandList = MemoryPointer::new(:ze_command_list_handle_t)
      result = ZE.zeCommandListCreate(@handle, device, desc, phCommandList)
      ZE.error_check(result)
      CommandList::new(phCommandList.read_ze_command_list_handle_t)
    end

    def command_list_create_immediate(device, ordinal: 0, index: 0, flags: 0, mode: 0, priority: 0)
      desc = ZECommandQueueDesc::new
      desc[:ordinal] = ordinal
      desc[:index] = index
      desc[:flags] = flags
      desc[:mode] = mode
      desc[:priority] = priority
      phCommandList = MemoryPointer::new(:ze_command_list_handle_t)
      result = ZE.zeCommandListCreateImmediate(@handle, device, desc, phCommandList)
      ZE.error_check(result)
      CommandList::new(phCommandList.read_ze_command_list_handle_t)
    end

    def system_barrier(device)
      result = ZE.zeContextSystemBarrier(@handle, device)
      ZE.error_check(result)
      self
    end

    def event_pool_create(count, flags: 0, devices: nil)
      desc = ZEEventPoolDesc::new()
      desc[:flags] = flags
      desc[:count] = count
      ph_devices = nil
      num_devices = 0
      if devices
        devices = [devices].flatten
        num_devices = devices.length
        ph_devices = MemoryPointer::new(:ze_device_handle_t, num_devices)
        ph_devices.write_array_of_ze_device_handle_t(devices.collect(&:to_ptr))
      end
      ph_event_pool = MemoryPointer::new(:ze_event_pool_handle_t)
      result = ZE.zeEventPoolCreate(@handle, desc, num_devices, ph_devices, ph_event_pool)
      ZE.error_check(result)
      EventPool::new(ph_event_pool.read_ze_event_pool_handle_t)
    end

    def event_pool_open_ipc_handle(h_ipc)
      ph_event_pool = MemoryPointer::new(:ze_event_pool_handle_t)
      result = ZE.zeEventPoolOpenIpcHandle(@handle, h_ipc, ph_event_pool)
      ZE.error_check(result)
      IPCEventPool::new(ph_event_pool.read_ze_event_pool_handle_t)
    end

    def image_create(device, width, height = 0, depth = 0,
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
      result = ZE.zeImageCreate(@handle, device, desc, phImage)
      ZE.error_check(result)
      Image::new(phImage.read_ze_image_handle_t)
    end

    # missing support for raytracing
    def mem_alloc_shared(size, device: nil, alignment: 0, ordinal: 0, host_flags: 0, device_flags: 0)
      pptr = MemoryPointer::new(:pointer)
      host_desc = ZEHostMemAllocDesc::new
      device_desc = ZEDeviceMemAllocDesc::new
      host_desc[:flags] = host_flags
      device_desc[:flags] = device_flags
      device_desc[:ordinal] = ordinal
      result = ZE.zeMemAllocShared(@handle, device_desc, host_desc, size, alignment, device, pptr)
      pptr.read_pointer.slice(0, size)
    end

    # missing support for file descriptor
    def mem_alloc_device(size, device, alignment: 0, ordinal: 0, flags: 0)
      pptr = MemoryPointer::new(:pointer)
      desc = ZEDeviceMemAllocDesc::new
      desc[:flags] = flags
      desc[:ordinal] = ordinal
      result = ZE.zeMemAllocDevice(@handle, desc, size, alignment, device, pptr)
      ZE.error_check(result)
      pptr.read_pointer.slice(0, size)
    end

    def mem_alloc_host(size, alignment: 0, flags: 0)
      pptr = MemoryPointer::new(:pointer)
      desc = ZEHostMemAllocDesc::new
      desc[:flags] = flags
      result = ZE.zeMemAllocHost(@handle, desc, size, alignment, pptr)
      ZE.error_check(result)
      pptr.read_pointer.slice(0, size)
    end

    def mem_free(ptr)
      result = ZE.zeMemFree(@handle, ptr)
      ZE.error_check(result)
      return self
    end

    # missing support for file descriptor
    def mem_get_alloc_properties(ptr)
      props = ZEMemoryAllocationProperties::new
      ph_device = MemoryPointer::new(:ze_device_handle_t)
      result = ZE.zeMemGetAllocProperties(@handle, ptr, props, ph_device)
      ZE.error_check(result)
      h_device = ph_device.read_ze_device_handle_t
      device = h_device.null? ? nil : Device::new(h_device, self)
      return [props, device]
    end
    alias mem_alloc_properties mem_get_alloc_properties

    def mem_get_address_range(ptr)
      p_base = MemoryPointer::new(:pointer)
      p_size = MemoryPointer::new(:size_t)
      result = ZE.zeMemGetAddressRange(@handle, ptr, p_base, p_size)
      ZE.error_check(result)
      return p_base.read_pointer.slice(0, p_size.read_size_t)
    end
    alias mem_address_range mem_get_address_range

    def mem_get_ipc_handle(ptr)
      ipc_handle = ZEIpcMemHandle.new
      result = ZE.zeMemGetIpcHandle(@handle, ptr, ipc_handle)
      ZE.error_check(result)
      return ipc_handle
    end
    alias mem_ipc_handle mem_get_ipc_handle

    def mem_open_ipc_handle(device, ipc_handle, flags: 0)
      pptr = MemoryPointer::new(:pointer)
      result = ZE.zeMemOpenIpcHandle(@handle, device, ipc_handle, flags, pptr)
      ZE.error_check(result)
      return pptr.read_pointer
    end

    def mem_close_ipc_handle(ptr)
      result = ZE.zeMemCloseIpcHandle(@handle, ptr)
      ZE.error_check(result)
      return self
    end

    def module_create(device, input_module,
                      format: :ZE_MODULE_FORMAT_IL_SPIRV,
                      build_flags: "",
                      profile_flags: nil,
                      constants: nil)
      desc = ZEModuleDesc::new
      input_size = input_module.bytesize
      p_input_module = MemoryPointer::new(input_size)
      p_input_module.write_bytes(input_module)
      if profile_flags
       build_flags += " -zet-profile-flags 0x#{ZETProfileFlag.to_native(profile_flags, nil).to_s(16)}"
      end
      p_build_flags = MemoryPointer.from_string(build_flags)
      desc[:format] = format
      desc[:inputSize] = input_size
      desc[:pInputModule] = p_input_module
      desc[:pBuildFlags] = p_build_flags
      desc[:pConstants] = constants
      ph_module = MemoryPointer::new(:ze_module_handle_t)
      ph_build_log = MemoryPointer::new(:ze_module_build_log_handle_t)
      result = ZE.zeModuleCreate(@handle, device, desc, ph_module, ph_build_log)
      begin
        ZE.error_check(result)
      rescue
        l = Module::BuildLog::new(ph_build_log.read_ze_module_build_log_handle_t)
        p l.string
        raise
      end
      [Module::new(ph_module.read_ze_module_handle_t, device),
       Module::BuildLog::new(ph_build_log.read_ze_module_build_log_handle_t)]
    end

    def make_memory_resident(device, ptr, size: nil)
      size = ptr.to_ptr.size unless size
      result = ZE.zeContextMakeMemoryResident(@handle, device, ptr, size)
      ZE.error_check(result)
      ptr.to_ptr.slice(0, size)
    end

    def evict_memory(device, ptr, size: nil)
      size = ptr.to_ptr.size unless size
      result = ZE.zeContextEvictMemory(@handle, device, ptr, size)
      ZE.error_check(result)
      ptr.to_ptr.slice(0, size)
    end

    def make_image_resident(device, image)
      result = ZE.zeContextMakeImageResident(@handle, device, image)
      ZE.error_check(result)
      self
    end

    def evict_image(device, image)
      result = ZE.zeContextEvictImage(@handle, device, image)
      ZE.error_check(result)
      self
    end

    def sampler_create(device, address_mode, filter_mode, normalized)
      desc = ZESamplerDesc::new
      desc[:addressMode] = address_mode
      desc[:filterMode] = filter_mode
      desc[:isNormalized] = normalized
      ptr = MemoryPointer::new(:ze_sampler_handle_t)
      result = ZE.zeSamplerCreate(@handle, device, desc, ptr)
      ZE.error_check(result)
      Sampler::new(ptr.read_ze_sampler_handle_t)
    end

    def virtual_mem_reserve(size, start: nil)
      pptr = MemoryPointer::new(:pointer)
      result = ZE.zeVirtualMemReserve(@handle, start, size, pptr)
      ZE.error_check(result)
      pptr.read_pointer.slice(0, size)
    end

    def virtual_mem_free(ptr, size: nil)
      size = ptr.to_ptr.size unless size
      result = ZE.zeVirtualMemFree(@handle, ptr, size)
      ZE.error_check(result)
      self
    end

    def virtual_mem_query_page_size(device, size)
      pagesize = MemoryPointer::new(:size_t)
      result = ZE.zeVirtualMemQueryPageSize(@handle, device, size, pagesize)
      ZE.error_check(result)
      pagesize.read_size_t
    end

    def physical_mem_create(device, size, flags: 0)
      desc = ZEPhysicalMemDesc::new
      desc[:flags] = flags
      desc[:size] = size
      ph = MemoryPointer::new(:ze_physical_mem_handle_t)
      result = ZE.zePhysicalMemCreate(@handle, device, desc, ph)
      ZE.error_check(result)
      PhysicalMem::new(ph.read_ze_physical_mem_handle_t)
    end

    def virtual_mem_map(ptr, physical_memory, access, size: nil, offset: 0)
      size = ptr.to_ptr.size unless size
      result = ZE.zeVirtualMemMap(@handle, ptr, size, physical_memory, offset, access)
      ZE.error_check(result)
      ptr.to_ptr.slice(0, size)
    end

    def virtual_mem_unmap(ptr, size: nil)
      size = ptr.to_ptr.size unless size
      result = ZE.zeVirtualMemUnmap(@handle, ptr, size)
      ZE.error_check(result)
      ptr.to_ptr.slice(0, size)
    end

    def virtual_mem_set_access_attribute(ptr, access, size: nil)
      size = ptr.to_ptr.size unless size
      result = ZE.zeVirtualMemSetAccessAttribute(@handle, ptr, size, access)
      ZE.error_check(result)
      ptr.to_ptr.slice(0, size)
    end

    def virtual_mem_get_access_attribute(ptr, size: nil)
      size = ptr.to_ptr.size unless size
      p_access = MemoryPointer::new(:ze_memory_access_attribute_t)
      out_size = MemoryPointer::new(:size_t)
      result = ZE.zeVirtualMemGetAccessAttribute(@handle, ptr, size, p_access, out_size)
      ZE.error_check(result)
      [ptr.to_ptr.slice(0, out_size.read_size_t), p_access.read_ze_memory_access_attribute_t]
    end

    def activate_metric_groups(device, *metric_groups)
      count = metric_groups.length
      ph_metric_groups = MemoryPointer::new(:zet_metric_group_handle_t, count)
      ph_metric_groups.write_array_of_zet_metric_group_handle_t(metric_group.collect(&:to_ptr))
      result = ZE.zetContextActivateMetricGroups(@handle, device, count, ph_metric_groups)
      ZE.error_check(result)
      self
    end

    def metric_streamer_open(device, metric_group, sampling_period: 1000,  notify_every_n_reports: 0, signal_event: nil)
      desc = ZETMetricStreamerDesc::new
      desc[:notifyEveryNReports] = notify_every_n_reports
      desc[:samplingPeriod] = sampling_period
      ph_metric_tracer = MemoryPointer::new(:zet_metric_tracer_handle_t)
      result = ZE.zetMetricStreamerOpen(@handle, device, metric_group, desc, signal_event, ph_metric_tracer)
      ZE.error_check(result)
      MetricStreamer::new(ph_metric_tracer.read_zet_metric_tracer_handle_t)
    end

    def metric_query_pool_create(device, metric_group, count, type: 0)
      desc = ZETMetricQueryPoolDesc::new
      desc[:type] = type
      desc[:count] = count
      ph_metric_query_pool = MemoryPointer::new(:zet_metric_query_pool_handle_t)
      result = ZE.zetMetricQueryPoolCreate(@handle, device, metric_group, desc, ph_metric_query_pool)
      ZE.error_check(result)
      MetricQueryPool::new(ph_metric_query_pool.read_zet_metric_query_pool_handle_t)
    end

    def tracer_exp_create(user_data: nil)
      user_data = @handle unless user_data
      ph_tracer = MemoryPointer::new(:zet_tracer_exp_handle_t)
      desc = ZETTracerExpDesc::new
      desc[:pUserData] = user_data
      result = ZE.zetTracerExpCreate(@handle, desc, ph_tracer)
      ZE.error_check(result)
      TracerExp::new(ph_tracer.read_zet_tracer_exp_handle_t)
    end

  end

  def self.module_dynamic_link(*modules)
    count = modules.size
    ph_modules = MemoryPointer::new(:ze_module_handle_t, count)
    ph_modules.write_array_of_ze_module_handle_t(events.collect(&:handle).collect(&:address))
    ph_build_log = MemoryPointer::new(:ze_module_build_log_handle_t)
    result = zeModuleDynamicLink(count, ph_modules, ph_build_log)
    ZE.error_check(result)
    Module::BuildLog::new(ph_build_log.read_ze_module_build_log_handle_t)
  end

  class Device < ZEObject
    attr_reader :driver

    def initialize(handle, driver)
      super(handle)
      @driver = driver
    end

    add_property :properties, memoize: true
    add_property :compute_properties, memoize: true
    add_property :module_properties, memoize: true
    add_array_property :command_queue_group_properties, :ZECommandQueueGroupProperties, :zeDeviceGetCommandQueueGroupProperties, memoize: true
    add_array_property :memory_properties, :ZEDeviceMemoryProperties, :zeDeviceGetMemoryProperties, memoize: true
    add_property :memory_access_properties, memoize: true
    add_array_property :cache_properties, :ZEDeviceCacheProperties, :zeDeviceGetCacheProperties, memoize: true
    add_property :image_properties, memoize: true
    add_property :external_memory_properties, memoize: true
    add_property :debug_properties, sname: :ZETDeviceDebugProperties, fname: :zetDeviceGetDebugProperties, memoize: true
    add_object_array :metric_groups, :MetricGroup, :zetMetricGroupGet, memoize: true
    add_array_property :processes_state, :ZESProcessState, :zesDeviceProcessesGetState
    add_property :pci_properties, sname: :ZESPciProperties, fname: :zesDevicePciGetProperties
    add_property :pci_state, sname: :ZESPciState, fname: :zesDevicePciGetState
    add_array_property :pci_bars, :ZESPciBarProperties, :zesDevicePciGetBars
    add_property :pci_stats, sname: :ZESPciStats, fname: :zesDevicePciGetStats
    add_object_array :diagnostics_test_suites, :Diagnostics, :zesDeviceEnumDiagnosticTestSuites, memoize: true
    add_object_array :engine_groups, :Engine, :zesDeviceEnumEngineGroups, memoize: true
    add_object_array :fabric_ports, :FabricPort, :zesDeviceEnumFabricPorts, memoize: true
    add_object_array :fans, :Fan, :zesDeviceEnumFans, memoize: true
    add_object_array :firmwares, :Firmware, :zesDeviceEnumFirmwares, memoize: true
    add_object_array :frequency_domains, :Frequency, :zesDeviceEnumFrequencyDomains, memoize: true
    add_object_array :leds, :Led, :zesDeviceEnumLeds, memoize: true
    add_object_array :memory_modules, :Memory, :zesDeviceEnumMemoryModules, memoize: true
    add_object_array :performance_factor_domains, :PerformanceFactor, :zesDeviceEnumPerformanceFactorDomains, memoize: true
    add_object_array :power_domains, :Power, :zesDeviceEnumPowerDomains, memoize: true
    add_object_array :psus, :Psu, :zesDeviceEnumPsus, memoize: true
    add_object_array :ras_error_sets, :Ras, :zesDeviceEnumRasErrorSets, memoize: true
    add_object_array :schedulers, :Scheduler, :zesDeviceEnumSchedulers, memoize: true
    add_object_array :standby_domains, :Standby, :zesDeviceEnumStandbyDomains, memoize: true
    add_object_array :temperature_sensors, :Temperature, :zesDeviceEnumTemperatureSensors, memoize: true

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

    def status
      result = ZE.zeDeviceGetStatus(@handle)
      ZE.error_check(result)
      result
    end

    def image_get_properties(width, height = 0, depth = 0,
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

    def debug_attach(pid)
      config = ZETDebugConfig::new
      config[:pid] = pid
      ph_debug = MemoryPointer::new(:zet_debug_session_handle_t)
      result = ZE.zetDebugAttach(@handle, config, ph_debug)
      ZE.error_check(result)
      Debug::new(ph_debug.read_zet_debug_session_handle_t)
    end

    def debug_get_register_set_properties
      p_count = MemoryPointer::new(:uint32)
      result = ZE.zetDebugGetRegisterSetProperties(@handle, p_count, nil)
      ZE.error_check(result)
      count = p_count.read_uint32
      return [] if count == 0
      p_regset_props = MemoryPointer::new(:zet_debug_regset_properties_t, count)
      result = ZE.zetDebugGetRegisterSetProperties(@handle, p_count, p_regset_props)
      ZE.error_check(result)
      sz = ZETDebugRegsetProperties.size
      count.times.collect { |i| ZETDebugRegsetProperties::new(p_regset_props.slice(i*sz, sz) ) }
    end

    def sysman_properties
      props = ZESDeviceProperties::new
      result = ZE.zesDeviceGetProperties(@handle, props)
      ZE.error_check(result)
      props
    end

    def state
      state = ZESDeviceState::new
      result = ZE.zesDeviceGetState(@handle, state)
      ZE.error_check(result)
      state
    end

    def reset(force: false)
      result = ZE.zesDeviceReset(@handle, force ? 1 : 0)
      ZE.error_check(result)
      self
    end

    def event_register(events)
      result = ZE.zesDeviceEventRegister(@handle, events)
      ZE.error_check(result)
      self
    end

    def global_timestamps
      hostTimestamp = MemoryPointer::new(:uint64)
      deviceTimestamp = MemoryPointer::new(:uint64)
      result = ZE.zeDeviceGetGlobalTimestamps(@handle, hostTimestamp, deviceTimestamp)
      return [hostTimestamp.read_uint64, deviceTimestamp.read_uint64]
    end

    def global_timestamps_ns
      hostTimestamp, deviceTimestamp = global_timestamps
      deviceTimestamp &= (1 << properties[:kernelTimestampValidBits]) - 1 
      return [hostTimestamp, deviceTimestamp * properties[:timerResolution]]
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

  class CommandQueue < ZEManagedObject
    @destructor = :zeCommandQueueDestroy

    def execute_command_lists(command_lists, fence: nil)
      cl = [command_lists].flatten
      num_command_lists = cl.length
      ph_command_lists = MemoryPointer::new(:ze_command_list_handle_t, num_command_lists)
      ph_command_lists.write_array_of_ze_command_list_handle_t(cl.collect(&:handle))
      result = ZE.zeCommandQueueExecuteCommandLists(@handle, num_command_lists, ph_command_lists, fence)
      ZE.error_check(result)
      self
    end

    def synchronize(timeout: ZE::UINT64_MAX)
      result = ZE.zeCommandQueueSynchronize(@handle, timeout)
      ZE.error_check(result)
      result
    end

    def fence_create(flags: 0)
      desc = ZEFenceDesc::new
      desc[:flags] = flags
      ph_fence = MemoryPointer::new(:ze_fence_handle_t)
      result = ZE.zeFenceCreate(@handle, desc, ph_fence)
      ZE.error_check(result)
      return Fence::new(ph_fence.read_ze_fence_handle_t)
    end
  end

  class CommandList < ZEManagedObject
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

    def append_write_global_timestamp(dstptr, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      result = ZE.zeCommandListAppendWriteGlobalTimestamp(@handle, dstptr, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_barrier(signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      result = ZE.zeCommandListAppendBarrier(@handle, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_memory_ranges_barrier(*ranges, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      num_ranges = ranges.length
      if num_ranges > 0
        pRangesSizes = MemoryPointer::new(:size_t, num_ranges)
        pRanges = MemoryPointer::new(:pointer, num_ranges)
        rranges = ranges.collect { |r|
          case r
          when Array
            [r.last, r.first]
          when Range
            [r.size, r.first]
          when Pointer
            [r.size, r.address]
          else
            ZE.error_check(:ZE_RESULT_ERROR_INVALID_ARGUMENT)
          end
        }
        sizes, starts = rranges.transpose
        pRangesSizes.write_array_of_size_t(sizes)
        pRanges.write_array_of_pointer(starts)
      else
        pRanges = nil
        pRangesSizes = nil
      end
      result = ZE.zeCommandListAppendMemoryRangesBarrier(@handle, num_ranges, pRangesSizes, pRanges, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_memory_copy(dstptr, srcptr, size: nil, signal_event: nil, wait_events: nil, context_src: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      unless size
        size = [dstptr.size, srcptr.size].min
      end
      if context_src
        result = ZE.zeCommandListAppendMemoryCopyFromContext(@handle, dstptr, context_src, srcptr, size, signal_event, count, ph_wait_events)
      else
        result = ZE.zeCommandListAppendMemoryCopy(@handle, dstptr, srcptr, size, signal_event, count, ph_wait_events)
      end
      ZE.error_check(result)
      self
    end

    def append_memory_fill(ptr, pattern, size: nil, pattern_size: nil, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      size = ptr.size unless size
      pattern_size = pattern.size unless pattern_size
      result = ZE.zeCommandListAppendMemoryFill(@handle, ptr, pattern, pattern_size, size, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_memory_copy_region(dstptr, srcptr, width, height, depth = 0,
                                  dst_origin: [0, 0, 0], src_origin: [0, 0, 0],
                                  dst_pitch: width, src_pitch: width,
                                  dst_slice_pitch: depth == 0 ? 0 : dst_pitch * height,
                                  src_slice_pitch: depth == 0 ? 0 : src_pitch * height,
                                  signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      dst_region = ZECopyRegion::new
      dst_region[:originX] = dst_origin[0]
      dst_region[:originY] = dst_origin[1]
      dst_region[:originZ] = dst_origin[2]
      dst_region[:width] = width
      dst_region[:height] = height
      dst_region[:depth] = depth
      src_region = ZECopyRegion::new
      src_region[:originX] = src_origin[0]
      src_region[:originY] = src_origin[1]
      src_region[:originZ] = src_origin[2]
      src_region[:width] = width
      src_region[:height] = height
      src_region[:depth] = depth
      result = ZE.zeCommandListAppendMemoryCopyRegion(@handle, dstptr, dst_region, dst_pitch, dst_slice_pitch, srcptr, src_region, src_pitch, src_slice_pitch, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_image_copy(dst_image, src_image, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      result = ZE.zeCommandListAppendImageCopy(@handle, dst_image, src_image, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_image_copy_region(dst_image, src_image, width, height = 1, depth = 1,
                                 dst_origin: [0, 0, 0], src_origin: [0, 0, 0],
                                 signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      dst_region = ZEImageRegion::new
      dst_region[:originX] = dst_origin[0]
      dst_region[:originY] = dst_origin[1]
      dst_region[:originZ] = dst_origin[2]
      dst_region[:width] = width
      dst_region[:height] = height
      dst_region[:depth] = depth
      src_region = ZEImageRegion::new
      src_region[:originX] = src_origin[0]
      src_region[:originY] = src_origin[1]
      src_region[:originZ] = src_origin[2]
      src_region[:width] = width
      src_region[:height] = height
      src_region[:depth] = depth
      result = ZE.zeCommandListAppendImageCopyRegion(@handle, dst_image, src_image, dst_region, src_region, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_image_copy_to_memory(dstptr, image, ranges: nil, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      src_region = nil
      src_region = _create_image_region_from_ranges(ranges) if ranges
      result = ZE.zeCommandListAppendImageCopyToMemory(@handle, dstptr, image, src_region, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_image_copy_from_memory(image, srcptr, ranges: nil, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      dst_region = nil
      dst_region = _create_image_region_from_ranges(ranges) if ranges
      result = ZE.zeCommandListAppendImageCopyFromMemory(@handle, image, srcptr, dst_region, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_memory_prefetch(ptr, size: nil)
      size = ptr.to_ptr.size unless size
      result = ZE.zeCommandListAppendMemoryPrefetch(@handle, ptr, size)
      ZE.error_check(result)
      self
    end

    def append_mem_advise(device, ptr, advice, size: nil)
      size = ptr.to_ptr.size unless size
      result = ZE.zeCommandListAppendMemAdvise(@handle, device, ptr, size, advice)
      ZE.error_check(result)
      self
    end

    def append_signal_event(event)
      result = ZE.zeCommandListAppendSignalEvent(@handle, event)
      ZE.error_check(result)
      self
    end

    def append_wait_on_events(events)
      count, ph_events = _create_event_list(events)
      result = ZE.zeCommandListAppendWaitOnEvents(@handle, count, ph_events)
      ZE.error_check(result)
      self
    end

    def append_event_reset(event)
      result = ZE.zeCommandListAppendEventReset(@handle, event)
      ZE.error_check(result)
      self
    end

    def append_query_kernel_timestamps(*events, dstptr, offsets: nil, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      num, ph_events = _create_event_list(events)
      if offsets
        p_offsets = MemoryPointer(:size_t, count)
        p_offsets.write_array_of_size_t(offsets)
        offsets = p_offsets
      end
      result = ZE.zeCommandListAppendQueryKernelTimestamps(@handle, num, ph_events, dstptr, offsets, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_launch_kernel(kernel, x, y = 1, z = 1, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      group_count = ZEGroupCount::new
      group_count[:groupCountX] = x
      group_count[:groupCountY] = y
      group_count[:groupCountZ] = z
      result = ZE.zeCommandListAppendLaunchKernel(@handle, kernel, group_count, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_launch_cooperative_kernel(kernel, x, y = 1, z = 1, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      group_count = ZEGroupCount::new
      group_count[:groupCountX] = x
      group_count[:groupCountY] = y
      group_count[:groupCountZ] = z
      result = ZE.zeCommandListAppendLaunchCooperativeKernel(@handle, kernel, group_count, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_launch_kernel_indirect(kernel, group_count, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      group_count = ZEGroupCount::new(group_count.to_ptr)
      result = ZE.zeCommandListAppendLaunchKernelIndirect(@handle, kernel, group_count, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_launch_nultiple_kernels_indirect(kernels, count_buffer, group_counts, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      kernels = [kernels].flatten
      num_kernels = kernels.length
      ph_kernels = MemoryPointer::new(:ze_kernel_handle_t, num_kernels)
      ph_kernels.write_array_of_ze_kernel_handle_t(kernels.collect(&:to_ptr))
      group_counts = ZEGroupCount::new(group_counts.to_ptr)
      result = ZE.zeCommandListAppendLaunchMultipleKernelsIndirect(@handle, num_kernels, kernels, count_buffer, group_counts, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_metric_streamer_marker(metric_streamer, value)
      result = ZE.zetCommandListAppendMetricStreamerMarker(@handle, metric_streamer, value)
      ZE.error_check(result)
      self
    end

    def append_metric_query_begin(metric_query)
      result = ZE.zetCommandListAppendMetricQueryBegin(@handle, metric_query)
      ZE.error_check(result)
      self
    end

    def append_metric_query_end(metric_query, signal_event: nil, wait_events: nil)
      count, ph_wait_events = _create_event_list(wait_events)
      result = ZE.zetCommandListAppendMetricQueryEnd(@handle, metric_query, signal_event, count, ph_wait_events)
      ZE.error_check(result)
      self
    end

    def append_metric_memory_barrier
      result = ZE.zetCommandListAppendMetricMemoryBarrier(@handle)
      ZE.error_check(result)
      self
    end

    private
    def _create_event_list(wait_events)
      count = 0
      ph_wait_events = nil
      if wait_events
        events = [wait_events].flatten
        count = events.length
        if count > 0
          ph_wait_events = MemoryPointer::new(:ze_event_handle_t, count)
          ph_wait_events.write_array_of_ze_event_handle_t(events.collect(&:handle).collect(&:address))
        end
      end
      [count, ph_wait_events]
    end

    def _create_image_region_from_ranges(ranges)
      ranges.collect! do r
        case r
        when Range
          [r.size, r.first]
        when Integer
          [r, 0]
        when Array
          [r.last, r.first]
        else
          ZE.error_check(:ZE_RESULT_ERROR_INVALID_ARGUMENT)
        end
      end
      ranges += [[1, 0]] * (3 - ranges.length) if ranges.length < 3
      region = ZEImageRegion::new
      region[:originX] = ranges[0][1]
      region[:originY] = ranges[1][1]
      region[:originZ] = ranges[2][1]
      region[:width] = ranges[0][0]
      region[:height] = ranges[1][0]
      region[:depth] = ranges[2][0]
      region
    end
  end

  class Image < ZEManagedObject
    @destructor = :zeImageDestroy
  end

  class Module < ZEManagedObject
    @destructor = :zeModuleDestroy
    attr_reader :device
    class BuildLog < ZEManagedObject
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
    add_property :properties, memoize: false

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
      pptr = MemoryPointer::(:pointer)
      p_size = MemoryPointer::(:size_t)
      p_name = MemoryPointer::from_string(name)
      result = ZE.zeModuleGetGlobalPointer(@handle, p_name, p_size, pptr)
      ZE.error_check(result)
      return pptr.read_pointer.slice(0, p_size.read_size_t)
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

    def debug_info(format: :ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF)
      p_size = MemoryPointer::new(:size_t)
      result = ZE.zetModuleGetDebugInfo(@handle, format, p_size, nil)
      ZE.error_check(result)
      p_debug_info = MemoryPointer::new(p_size.read_size_t)
      result = ZE.zetModuleGetDebugInfo(@handle, format, p_size, p_debug_info)
      ZE.error_check(result)
      p_debug_info.read_bytes(p_size.read_size_t)
    end

  end

  class Kernel < ZEManagedObject
    @destructor = :zeKernelDestroy
    add_property :properties

    def set_group_size(group_size_x, group_size_y = 1, group_size_z = 1)
      result = ZE.zeKernelSetGroupSize(@handle, group_size_x, group_size_y, group_size_z)
      ZE.error_check(result)
      return self
    end

    def suggest_group_size(global_size_x, global_size_y = 1, global_size_z = 1)
      ptr = MemoryPointer::new(:uint32, 3)
      result = ZE.zeKernelSuggestGroupSize(@handle, global_size_x, global_size_y, global_size_z, ptr, ptr + 4, ptr + 8)
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

    def set_indirect_access(flags)
      result = ZE.zeKernelSetIndirectAccess(@handle, flags)
      ZE.error_check(result)
      self
    end
    def indirect_access=(flags)
      set_indirect_access(flags)
      flags
    end

    def indirect_acess
      ptr = MemoryPointer::new(:ze_kernel_indirect_access_flags_t)
      result = ZE.zeKernelGetIndirectAccess(@handle, ptr)
      ZE.error_check(result)
      ptr.read_ze_kernel_indirect_access_flags_t
    end

    def source_attributes
      ptr = MemoryPointer::new(:pointer)
      result = ZE.zeKernelGetSourceAttributes(@handle, 0, ptr)
      ZE.error_check(result)
      ptr.read_pointer.read_string.split(" ")
    end

    def set_cache_config(flags)
      result = ZE.zeKernelSetCacheConfig(@handle, flags)
      ZE.error_check(result)
      self
    end

    def name
      @name ||= begin
        p_size = MemoryPointer::new(:size_t)
        result = zeKernelGetName(@handle, p_size, nil)
        ZE.error_check(result)
        p_name = MemoryPointer::new(:char, p_size.read_size_t)
        result = zeKernelGetName(@handle, p_size, p_name)
        ZE.error_check(result)
        p_name.read_string
      end
    end

    # This is not supported by intel and documentation makes no sense...
    def profile_info
      info = ZETProfileProperties::new
      result = ZE.zetKernelGetProfileInfo(@handle, info)
      ZE.error_check(result)
      info
    end
  end

  class EventPool < ZEManagedObject
    @destructor = :zeEventPoolDestroy

    def event_create(index, signal: 0, wait: 0)
      desc = ZEEventDesc::new
      desc[:index] = index
      desc[:signal] = signal
      desc[:wait] = wait
      ph_event = MemoryPointer::new(:ze_event_handle_t)
      result = ZE.zeEventCreate(@handle, desc, ph_event)
      ZE.error_check(result)
      Event::new(ph_event.read_ze_event_handle_t)
    end

    def ipc_handle
      ph_ipc = ZEIPCEventPoolHandle::new
      result = ZE.zeEventPoolGetIpcHandle(@handle, ph_ipc)
      ZE.error_check(result)
      ph_ipc
    end
  end

  class IPCEventPool < EventPool
    @destructor = :zeEventPoolCloseIpcHandle

    alias close destroy
  end

  class Event < ZEManagedObject
    @destructor = :zeEventDestroy

    def host_signal
      result = ZE.zeEventHostSignal(@handle)
      ZE.error_check(result)
      self
    end
    alias signal host_signal

    def host_synchronize(timeout: ZE::UINT64_MAX)
      result = ZE.zeEventHostSynchronize(@handle, timeout)
      ZE.error_check(result)
      result
    end
    alias synchronize host_synchronize

    def query_status
      result = ZE.zeEventQueryStatus(@handle)
      ZE.error_check(result)
      result
    end
    alias status query_status

    def host_reset
      result = ZE.zeEventHostReset(@handle)
      ZE.error_check(result)
      self
    end
    alias reset host_reset

    def query_kernel_timestamp
      res = ZEKernelTimestampResult::new()
      result = ZE.zeEventQueryKernelTimestamp(@handle, res)
      ZE.error_check(result)
      res
    end
    alias kernel_timestamp query_kernel_timestamp

  end

  class Sampler < ZEManagedObject
    @destructor = :zeSamplerDestroy
  end

  class PhysicalMem < ZEManagedObject
    @destructor = :zePhysicalMemDestroy
  end

  class Fence < ZEManagedObject
    @destructor = :zeFenceDestroy

    def host_synchronize(timeout: ZE::UINT64_MAX)
      result = ZE.zeFenceHostSynchronize(@handle, timeout)
      ZE.error_check(result)
      result
    end
    alias synchronize host_synchronize

    def query_status
      result = ZE,zeFenceQueryStatus(@handle)
      ZE.error_check(result)
      result
    end
    alias status query_status

    def reset
      result = ZE.zeFenceReset(@handle)
      ZE.error_check(result)
      self
    end
  end

  class MetricGroup < ZETObject
    add_property :properties, sname: :ZETMetricGroupProperties, fname: :zetMetricGroupGetProperties, memoize: true
    add_object_array :metrics, :Metric, :zetMetricGet, memoize: true

    def calculate_metric_values(raw_data)
      if raw_data.kind_of?(FFI::Pointer)
        p_raw_data = raw_data
        raw_data_size = raw_data.size
      else
        raw_data_size = raw_data.bytesize
        p_raw_data = MemoryPointer::new(raw_data_size)
        p_raw_data.write_bytes(raw_data)
      end
      p_metric_value_count = MemoryPointer::new(:uint32_t)
      result = ZE.zetMetricGroupCalculateMetricValues(@handle, raw_data_size, p_raw_data, p_metric_value_count, nil)
      ZE.error_check(result)
      count = p_metric_value_count.read_uint32
      return [] if count == 0
      p_metric_values = MemoryPointer::new(ZETTypedValue, count)
      sz = ZETTypedValue.size
      metric_values = count.times.collect { |i| ZETTypedValue::new(p_metric_values.slice(sz * i, sz)) }
      result = ZE.zetMetricGroupCalculateMetricValues(@handle, raw_data_size, p_raw_data, p_metric_value_count, metric_values.first)
      ZE.error_check(result)
      metric_values
    end
  end

  class Metric < ZETObject
    add_property :properties, sname: :ZETMetricProperties, fname: :zetMetricGetProperties, memoize: true
  end

  class MetricStreamer < ZETManagedObject
    @destructor = :zetMetricStreamerClose

    def read_data(max_report_count: UINT32_MAX)
      p_raw_data_size = MemoryPointer::new(:size_t)
      result = ZE.zetMetricStreamerReadData(@handle, max_report_count, p_raw_data_size, nil)
      ZE.error_check(result)
      raw_data_size = p_raw_data_size.read_size_t
      p_raw_data = MemoryPointer::new(raw_data_size)
      result = ZE.zetMetricTracerReadData(@handle, max_report_count, p_raw_data_size, p_raw_data)
      ZE.error_check(result)
      p_raw_data
    end
  end

  class MetricQueryPool < ZETManagedObject
    @destructor = :zetMetricQueryPoolDestroy

    def metric_query_create(index)
      ph_metric_query = MemoryPointer::new(:zet_metric_query_handle_t)
      result = ZE.zetMetricQueryCreate(@handle, index, ph_metric_query)
      ZE.error_check(result)
      MetricQuery::new(ph_metric_query.read_zet_metric_query_handle_t)
    end
  end

  class MetricQuery < ZETManagedObject
    @destructor = :zetMetricQueryDestroy

    def reset
      result = ZE.zetMetricQueryReset(@handle)
      ZE.error_check(result)
      self
    end

    def data
      p_raw_data_size = MemoryPointer::new(:size_t)
      result = ZE.zetMetricQueryGetData(@handle, p_raw_data_size, nil)
      ZE.error_check(result)
      raw_data_size = p_raw_data_size.read_size_t
      p_raw_data = MemoryPointer::new(raw_data_size)
      result = ZE.zetMetricQueryGetData(@handle, p_raw_data_size, p_raw_data)
      ZE.error_check(result)
      p_raw_data
    end
  end

  class Debug < ZETManagedObject
    @destructor = :zetDebugDetach

    def read_event(timeout: UINT64_MAX)
      ev = ZETDebugEvent::new
      result = ZE.zetDebugReadEvent(@handle, timeout, ev.size, ev)
      ZE.error_check(result)
      ev
    end

    def interrupt(threadid = nil, slice: nil, subslice: nil, eu: nil, thread: nil)
      unless threadid
        threadid = ZEDeviceThread::new
        threadid[:slice] = slice
        threadid[:subslice] = subslice
        threadid[:eu] = eu
        threadid[:thread] = thread
      end
      result = ZE.zetDebugInterrupt(@handle, threadid)
      ZE.error_check(result)
      self
    end

    def resume(threadid = nil, slice: nil, subslice: nil, eu: nil, thread: nil)
      unless threadid
        threadid = ZEDeviceThread::new
        threadid[:slice] = slice
        threadid[:subslice] = subslice
        threadid[:eu] = eu
        threadid[:thread] = thread
      end
      result = ZE.zetDebugResume(@handle, threadid)
      ZE.error_check(result)
      self
    end

    def read_memory(threadid, address, type: 0, size: nil, slice: nil, subslice: nil, eu: nil, thread: nil)
      unless threadid
        threadid = ZEDeviceThread::new
        threadid[:slice] = slice
        threadid[:subslice] = subslice
        threadid[:eu] = eu
        threadid[:thread] = thread
      end
      size = address.size unless size
      p_buffer = MemoryPointer::new(size)
      desc = ZETDebugMemorySpaceDesc::new
      desc[:address] = address
      desc[:type] = type
      result = ZE.zetDebugReadMemory(@handle, threadid, desc, size, p_buffer)
      ZE.error_check(result)
      p_buffer
    end

    def write_memory(threadid, address, buffer, type: 0, size: nil, slice: nil, subslice: nil, eu: nil, thread: nil)
      unless threadid
        threadid = ZEDeviceThread::new
        threadid[:slice] = slice
        threadid[:subslice] = subslice
        threadid[:eu] = eu
        threadid[:thread] = thread
      end
      unless size
        if address.respond_to?(:size)
          size = [address.size, buffer.size].min
        else
          size = buffer.size
        end
      end
      desc = ZETDebugMemorySpaceDesc::new
      desc[:address] = address
      desc[:type] = type
      result = ZE.zetDebugWriteMemory(@module, threadid, desc, size, buffer)
      ZE.error_check(result)
      self
    end

    def read_registers(threadid, type, start, count, slice: nil, subslice: nil, eu: nil, thread: nil)
      unless threadid
        threadid = ZEDeviceThread::new
        threadid[:slice] = slice
        threadid[:subslice] = subslice
        threadid[:eu] = eu
        threadid[:thread] = thread
      end
      ptr = MemoryPointer::new(count)
      result = ZE.zetDebugReadRegisters(@handle, threadid, start, count, ptr)
      ZE.error_check(result)
      ptr
    end

    def write_registers(threadid, type, start, count, values, slice: nil, subslice: nil, eu: nil, thread: nil)
      unless threadid
        threadid = ZEDeviceThread::new
        threadid[:slice] = slice
        threadid[:subslice] = subslice
        threadid[:eu] = eu
        threadid[:thread] = thread
      end
      result = ZE.zetDebugWriteRegisters(@handle, threadid, start, count, values)
      ZE.error_check(result)
      self
    end

  end

  class TracerExp < ZETManagedObject
    @destructor = :zetTracerExpDestroy
    attr_reader :prologues
    attr_reader :epilogues

    def initialize(*args)
      super
      @callbacks = []
      @prologues = Hash::new { |h, k| h[k] = {} }
      @prologues_ze = ZECallbacks::new
      @epilogues = Hash::new { |h, k| h[k] = {} }
      @epilogues_ze = ZECallbacks::new
    end

    def set_prologues
      copy_table(@prologues_ze, @prologues)
      result = ZE.zetTracerExpSetPrologues(@handle, @prologues_ze)
      ZE.error_check(result)
      self
    end

    def set_epilogues
      Tracer.copy_table(@epilogues_ze, @epilogues)
      result = ZE.zetTracerExpSetEpilogues(@handle, @epilogues_ze)
      ZE.error_check(result)
      self
    end

    def set_enabled(enable)
      result = ZE.zetTracerExpSetEnabled(@handle, enable ? 1 : 0)
      ZE.error_check(result)
      self
    end
    private

    def copy_table(table_ze, table)
      table.each { |group, t|
        g = table_ze[group]
        t.each { |name, block|
           g[name] = block
           @callbacks.push block
        }
      }
      nil
    end
  end

  class Diagnostics < ZESObject
    add_property :properties, sname: :ZESDiagProperties, fname: :zesDiagnosticsGetProperties, memoize: true
    add_array_property :tests, :ZESDiagTest, :zesDiagnosticsGetTests, memoize: true

    def run_tests(start: ZES_DIAG_FIRST_TEST_INDEX, stop: ZES_DIAG_LAST_TEST_INDEX)
      res = ZETDiagResult::new
      result = ZE.zesDiagnosticsRunTests(@handle, start, stop, res)
      ZE.error_check(result)
      res
    end
  end

  class Engine < ZESObject
    add_property :properties, sname: :ZESEngineProperties, fname: :zesEngineGetProperties
    add_property :activity, sname: :ZESEngineStats, fname: :zesEngineGetActivity
  end

  class FabricPort < ZESObject
    add_property :properties, sname: :ZESFabricPortProperties, fname: :zesFabricPortGetProperties, memoize: true
    add_property :link_type, sname: :ZESFabricLinkType, fname: :zesFabricPortGetLinkType, memoize: true
    add_property :config, sname: :ZESFabricPortConfig, fname: :zesFabricPortGetConfig
    add_property :state, sname: :ZESFabricPortState, fname: :zesFabricPortGetState
    add_property :throughput, sname: :ZESFabricPortThroughput, fname: :zesFabricPortGetThroughput

    def set_config(enabled:, beaconing:)
      config = ZESFabricPortConfig::new
      config[:enabled] = enabled ? 1 : 0
      config[:beaconing] = beaconing ? 1 : 0
      result = ZE.zesFabricPortSetConfig(@handle, config)
      ZE.error_check(result)
      self
    end
  end

  class Fan < ZESObject
    add_property :properties, sname: :ZESFanProperties, fname: :zesFanGetProperties, memoize: true
    add_property :config, sname: :ZESFanConfig, fname: :zesFanGetConfig

    def set_default_mode
      result = ZE.zesFanSetDefaultMode(@handle)
      ZE.error_check(result)
      self
    end

    def set_fixed_speed_mode(speed, units)
      fan_speed = ZESFanSpeed::new
      fan_speed[:speed] = speed
      fan_speed[:units] = units
      result = zesFanSetFixedSpeedMode(@handle, fan_speed)
      ZE.error_check(result)
      self
    end

    def set_speed_table_mode(temperature_speed_map, units)
      ZE.error_check(:ZE_RESULT_ERROR_INVALID_VALUE) if temperature_speed_map.size > ZES_FAN_TEMP_SPEED_PAIR_COUNT
      fan_speed_table = ZESFanSpeedTable::new
      temperature_speed_map.to_a.sort.each { |k, v|
        fan_speed_table[:table][:temperature] = k
        fan_speed_table[:table][:speed][:speed] = v
        fan_speed_table[:table][:speed][:units] = units
      }
      fan_speed_table[:numPoints] = fan_speed_table.size
      result = ZE.zesFanSetFixedSpeedMode(@handle, fan_speed_table)
      ZE.error_check(result)
    end

    def state(units: :ZET_FAN_SPEED_UNITS_PERCENT)
      p_speed = MemoryPointer::new(:uint32_t)
      result = ZE.zesFanGetState(@handle, units, p_speed)
      ZE.error_check(result)
      return p_speed.read_uint32
    end
  end

  class Firmware < ZESObject
    add_property :properties, sname: :ZESFirmwareProperties, fname: :zesFirmwareGetProperties

    def flash(firmware)
      size = firmware.bytesize
      p_image = MemoryPointer::new(size)
      p_image.write_bytes(firmware)
      result = ZE.zesFirmwareFlash(@handle, p_image, size)
      ZE.error_check(result)
      self
    end
  end

  class Frequency < ZESObject
    add_property :properties, sname: :ZESFreqProperties, fname: :zesFrequencyGetProperties, memoize: true
    add_property :range, sname: :ZESFreqRange, fname: :zesFrequencyGetRange
    add_property :state, sname: :ZESFreqState, fname: :zesFrequencyGetState
    add_property :throttle_time, sname: :ZESFreqThrottleTime, fname: :zesFrequencyGetThrottleTime
    add_property :oc_capabilities, sname: :ZESOcCapabilities, fname: :zesFrequencyOcGetCapabilities

    def available_clocks
      pCount = MemoryPointer::new(:uint32_t)
      result = ZE.zesFrequencyGetAvailableClocks(@handle, pCount, nil)
      ZE.error_check(result)
      count = pCount.read(:uint32)
      return [] if count == 0
      pArr = MemoryPointer::new(:double, count)
      result = ZE.zesFrequencyGetAvailableClocks(@handle, pCount, pArr)
      ZE.error_check(result)
      return pArr.read_array_of_double(count)
    end
    alias clocks available_clocks

    def set_range(min, max)
      limits = ZESFreqRange::new
      limits[:min] = min
      limits[:max] = max
      result = ZE.zesFrequencySetRange(@handle, limits)
      ZE.error_check(result)
      self
    end

    def oc_frequency_target
      p_cur_freq = MemoryPointer::new(:double)
      result = ZE.zesFrequencyOcGetFrequencyTarget(@handle, p_cur_freq)
      ZE.error_check(result)
      return p_cur_freq.read_double
    end

    def set_oc_frequency_target(frequency)
      result = ZE.zesFrequencyOcSetFrequencyTarget(@handle, frequency)
      ZE.error_check(result)
      self
    end
    def oc_frequency_target=(frequency)
      set_oc_frequency_target(frequency)
      frequency
    end

    def oc_voltage_target
      p_cur_vt = MemoryPointer::new(:double)
      p_cur_vo = MemoryPointer::new(:double)
      result = ZE.zesFrequencyOcGetVoltageTarget(@handle, p_cur_vt, p_cur_vo)
      ZE.error_check(result)
      [p_cur_vt.read_double, p_cur_vo.read_double]
    end

    def set_oc_voltage_target(voltage_target, voltage_offset)
      result = ZE.zesFrequencyOcSetVoltageTarget(@handle, voltage_target, voltage_offset)
      ZE.error_check(result)
      self
    end

    def set_oc_mode(mode)
      result = ZE.zesFrequencyOcSetMode(mode)
      ZE.error_check(result)
      self
    end
    def oc_mode=(mode)
      set_oc_mode(mode)
      mode
    end

    def oc_mode
      p_mode = MemoryPointer::new(:zes_oc_mode_t)
      result = ZE.zesFrequencyOcGetMode(@handle, p_mode)
      ZE.error_check(result)
      ZESOcMode.from_native(p_mode.read_zes_oc_mode_t)
    end

    def oc_icc_max
      p_oc_icc_max = MemoryPointer::new(:double)
      result = ZE.zesFrequencyOcGetIccMax(@handle, p_oc_icc_max)
      ZE.error_check(result)
      p_oc_icc_max.read_double
    end

    def set_oc_icc_max(oc_icc_max)
      result = ZE.zesFrequencyOcSetIccMax(@handle, oc_icc_max)
      ZE.error_check(result)
      self
    end
    def oc_icc_max=(oc_icc_max)
      set_oc_icc_max(oc_icc_max)
      self
    end

    def oc_tj_max
      p_oc_tj_max = MemoryPointer::new(:double)
      result = ZE.zesFrequencyOcGetTjMax(@handle, p_oc_tj_max)
      ZE.error_check(result)
      p_oc_tj_max.read_double
    end

    def oc_set_tj_max(tj_max)
      result = ZE.zesFrequencyOcSetTjMax(@handle, tj_max)
      ZE.error_check(result)
      self
    end
    def oc_tj_max=(tj_max)
      oc_set_tj_max(tj_max)
      tj_max
    end

  end

  class Led < ZESObject
    add_property :properties, sname: :ZESLedProperties, fname: :zesLedGetProperties, memoize: true
    add_property :state, sname: :ZESLedState, fname: :zesLedGetState

    def set_state(enable)
      result = ZE.zesLedSetState(@handle, enable ? 1 : 0)
      ZE.error_check(result)
      self
    end
    def on!
      set_state(true)
    end
    def off!
      set_state(false)
    end

    def set_color(red, green, blue)
      normalize = lambda { |c| (c.kind_of?(Int) ? c/255.0 : c).clamp(0, 1.0) }
      state = ZESLedColor::new
      state[:red] = normalize.call(red)
      state[:green] = normalize.call(green)
      state[:blue] = normalize.call(blue)
      result = ZE.zesLedSetColor(@handle, state)
      ZE.error_check(result)
      self
    end
  end

  class Memory < ZESObject
    add_property :properties, sname: :ZESMemProperties, fname: :zesMemoryGetProperties, memoize: true
    add_property :state, sname: :ZESMemState, fname: :zesMemoryGetState
    add_property :bandwidth, sname: :ZESMemBandwidth, fname: :zesMemoryGetBandwidth
  end

  class PerformanceFactor < ZESObject
    add_property :properties, sname: :ZESPerfProperties, fname: :zesPerformanceFactorGetProperties, memoize: true

    def config
      p_factor = MemoryPointer::new(:double)
      result = ZE.zesPerformanceFactorGetConfig(@handle, p_factor)
      ZE.error_check(result)
      p_factor.read_double
    end

    def set_config(factor)
      result = ZE.zesPerformanceFactorSetConfig(@handle, factor)
      ZE.error_check(result)
      self
    end
    def config=(factor)
      set_config(factor)
      factor
    end

  end

  class Power < ZESObject
    add_property :properties, sname: :ZESPowerProperties, fname: :zesPowerGetProperties, memoize: true
    add_property :energy_counter, sname: :ZESPowerEnergyCounter, fname: :zesPowerGetEnergyCounter
    add_property :energy_threshold, sname: :ZESEnergyThreshold, fname: :zesPowerGetEnergyThreshold

    def limits
      sustained_limit = ZESPowerSustainedLimit::new
      burst_limit = ZESPowerBurstLimit::new
      peak_limit = ZESPowerPeakLimit::new
      result = ZE.zesPowerGetLimits(@handle, sustained_limit, burst_limit, peak_limit)
      ZE.error_check(result)
      [sustained_limit, burst_limit, peak_limit]
    end

    def sustained_limit
      sustained_limit = ZESPowerSustainedLimit::new
      result = ZE.zesPowerGetLimits(@handle, sustained_limit, nil, nil)
      ZE.error_check(result)
      sustained_limit
    end

    def burst_limit
      burst_limit = ZESPowerBurstLimit::new
      result = ZE.zesPowerGetLimits(@handle, nil, burst_limit, nil)
      ZE.error_check(result)
      burst_limit
    end

    def peak_limit
      peak_limit = ZESPowerPeakLimit::new
      result = ZE.zesPowerGetLimits(@handle, nil, nil, peak_limit)
      ZE.error_check(result)
      peak_limit
    end

    def set_limits(sustained_enabled: false, sustained_power: 0, sustained_interval: 0,
                   burst_enabled: false, burst_power: 0,
                   peak_power_ac: 0, peak_power_dc: 0)
      sustained_limit = ZESPowerSustainedLimit::new
      sustained_limit[:enabled] = sustained_enabled ? 1 : 0
      sustained_limit[:power] = sustained_power
      sustained_limit[:interval] = sustained_interval
      burst_limit = ZESPowerBurstLimit::new
      burst_limit[:enabled] = burst_enabled ? 1 : 0
      burst_limit[:power] = burst_power
      peak_limit = ZESPowerPeakLimit::new
      peak_limit[:powerAC] = peak_power_ac
      peak_limit[:powerDC] = peak_power_dc
      result = ZE.zesPowerSetLimits(@handle, sustained_limit, burst_limit, peak_limit)
      ZE.error_check(result)
      self
    end

    def set_sustained_limit(enabled, power, interval)
      sustained_limit = ZESPowerSustainedLimit::new
      sustained_limit[:enabled] = enabled ? 1 : 0
      sustained_limit[:power] = power
      sustained_limit[:interval] = interval
      result = ZE.zesPowerSetLimits(@handle, sustained_limit, nil, nil)
      ZE.error_check(result)
      self
    end

    def set_burst_limit(enabled, power)
      burst_limit = ZESPowerBurstLimit::new
      burst_limit[:enabled] = enabled ? 1 : 0
      burst_limit[:power] = power
      result = ZE.zesPowerSetLimits(@handle, nil, burst_limit, nil)
      ZE.error_check(result)
      self
    end

    def set_peak_limit(power_ac, power_dc)
      peak_limit = ZESPowerPeakLimit::new
      peak_limit[:powerAC] = power_ac
      peak_limit[:powerDC] = power_dc
      result = ZE.zesPowerSetLimits(@handle, nil, nil, peak_limit)
      ZE.error_check(result)
      self
    end

    def set_energy_threshold(threshold)
      result = ZE.zesPowerSetEnergyThreshold(@handle, threshold)
      ZE.error_check(result)
      self
    end
    def energy_threshold=(threshold)
      set_energy_threshold(threshold)
      threshold
    end

  end

  class Psu < ZESObject
    add_property :properties, sname: :ZESPsuProperties, fname: :zesPsuGetProperties, memoize: true
    add_property :state, sname: :ZESPsuState, fname: :zesPsuGetState
  end

  class Ras < ZESObject
    add_property :properties, sname: :ZESRasProperties, fname: :zesRasGetProperties, memoize: true
    add_property :config, sname: :ZESRasConfig, fname: :zesRasGetConfig

    def set_config(total_threshold: 0,
                   num_resets: 0,
                   num_programming_errors: 0,
                   num_driver_errors: 0,
                   num_compute_errors: 0,
                   num_non_compute_errors: 0,
                   num_cache_errors: 0,
                   num_display_errors: 0)
      config = ZESRasConfig::new
      config[:totalThreshold] = total_threshold
      detailed_thresholds = config[:detailedThresholds]
      detailed_thresholds[:stype] = :ZES_STRUCTURE_TYPE_RAS_STATE
      category = detailed_thresholds[:category]
      category[ZESRasErrorCat.to_native(:ZES_RAS_ERROR_CAT_RESET)] = num_resets
      category[ZESRasErrorCat.to_native(:ZES_RAS_ERROR_CAT_PROGRAMMING_ERRORS)] = num_programming_errors
      category[ZESRasErrorCat.to_native(:ZES_RAS_ERROR_CAT_DRIVER_ERRORS)] = num_driver_errors
      category[ZESRasErrorCat.to_native(:ZES_RAS_ERROR_CAT_COMPUTE_ERRORS)] = num_compute_errors
      category[ZESRasErrorCat.to_native(:ZES_RAS_ERROR_CAT_NON_COMPUTE_ERRORS)] = num_non_compute_errors
      category[ZESRasErrorCat.to_native(:ZES_RAS_ERROR_CAT_CACHE_ERRORS)] = num_cache_errors
      category[ZESRasErrorCat.to_native(:ZES_RAS_ERROR_CAT_DISPLAY_ERRORS)] = num_display_errors
      result = ZE.zesRasSetConfig(@handle, config)
      ZE.error_check(result)
      self
    end

    def state(clear: false)
      state = ZESRasState::new
      result = ZE.zesRasGetState(@handle, clear ? 1 : 0, state)
      ZE.error_check(result)
      state
    end
  end

  class Scheduler < ZESObject
    add_property :properties, sname: :ZESSchedProperties, fname: :zesSchedulerGetProperties, memoize: true
    add_enum_property :current_mode, :zes_sched_mode_t, :zesSchedulerGetCurrentMode
    alias mode current_mode

    def timeout_mode_properties(default: false)
      props = ZESSchedTimeoutProperties::new
      result = ZE.zesSchedulerGetTimeoutModeProperties(@handle, default ? 1 : 0, props)
      ZE.error_check(result)
      props
    end

    def timeslice_mode_properties(default: false)
      props = ZESSchedTimesliceProperties::new
      result = ZE.zesSchedulerGetTimesliceModeProperties(@handle, default ? 1 : 0, props)
      ZE.error_check(result)
      props
    end

    def set_timeout_mode(watchdogTimeout)
      props = ZESSchedTimeoutProperties::new
      props[:watchdogTimeout] = watchdogTimeout
      p_need_reload = MemoryPointer::new(:ze_bool_t)
      result = ZE.zesSchedulerSetTimeoutMode(@handle, props, p_need_reload)
      ZE.error_check(result)
      p_need_reload.read_ze_bool_t == 0 ? false : true
    end

    def set_timeslice_mode(interval, yield_timeout)
      props = ZESSchedTimesliceProperties::new
      props[:interval] = interval
      props[:yieldTimeout] = yield_timeout
      p_need_reload = MemoryPointer::new(:ze_bool_t)
      result = ZE.zesSchedulerSetTimesliceMode(@handle, props, p_need_reload)
      ZE.error_check(result)
      p_need_reload.read_ze_bool_t == 0 ? false : true
    end

    def set_exclusive_mode
      p_need_reload = MemoryPointer::new(:ze_bool_t)
      result = ZE.zesSchedulerSetExclusiveMode(@handle, p_need_reload)
      ZE.error_check(result)
      p_need_reload.read_ze_bool_t == 0 ? false : true
    end

    def set_compute_unit_debug_mode
      p_need_reload = MemoryPointer::new(:ze_bool_t)
      result = ZE.zesSchedulerSetComputeUnitDebugMode(@handle, p_need_reload)
      ZE.error_check(result)
      p_need_reload.read_ze_bool_t == 0 ? false : true
    end
  end

  class Standby < ZESObject
    add_property :properties, sname: :ZESStandbyProperties, fname: :zesStandbyGetProperties, memoize: true
    add_enum_property :mode, :zes_standby_promo_mode_t, :zesStandbyGetMode

    def set_mode(mode)
      result = ZE.zesStandbySetMode(@handle, mode)
      ZE.error_check(result)
      self
    end
    def mode=(mode)
      set_mode(mode)
      mode
    end
  end

  class Temperature < ZESObject
    add_property :properties, sname: :ZESTempProperties, fname: :zesTemperatureGetProperties, memoize: true
    add_property :config, sname: :ZESTempConfig, fname: :zesTemperatureGetConfig

    def set_config(enable_critical, threshold1, threshold2)
      config = ZESTempConfig::new
      config[:enableCritical] = enable_critical ? 1 : 0
      config[:threshold1] = threshold1
      config[:threshold2] = threshold2
      result = ZE.zeaTemperatureSetConfig(@handle, config)
      ZE.error_check(result)
      self
    end

    def state
      p_temperature = MemoryPointer::new(:double)
      result = zeaTemperatureGetState(@handle, p_temperature)
      ZE.error_check(result)
      p_temperature.read_double
    end
  end

  def self.ze_init(flags: 0)
    result = zeInit(flags)
    error_check(result)
    nil
  end

  def self.init(flags: 0)
    ze_init(flags: flags)
    nil
  end

  def self.drivers
    return @drivers if @drivers
    p_count = MemoryPointer::new(:uint32_t)
    result = zeDriverGet(p_count, nil);
    error_check(result)
    count = p_count.read_uint32
    return [] if count == 0
    ph_drivers = MemoryPointer::new(:ze_driver_handle_t, count)
    result = zeDriverGet(p_count, ph_drivers);
    error_check(result)
    @drivers = ph_drivers.read_array_of_ze_driver_handle_t(count).collect { |h| Driver::new(h) }
  end

end
