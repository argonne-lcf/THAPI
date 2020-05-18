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
  ZET_EVENT_WAIT_NONE = 0x0
  ZET_EVENT_WAIT_INFINITE = 0xffffffff
  ZET_DIAG_FIRST_TEST_INDEX =  0x0
  ZET_DIAG_LAST_TEST_INDEX = 0xffffffff

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
#  ENV["ZE_ENABLE_PARAMETER_VALIDATION"] = "1"
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
      res.gsub("Uuid","UUID").gsub("Dditable", "DDITable").gsub(/\AFp/, "FP").gsub("Ipc", "IPC").gsub("P2p", "P2P")
    end

    def self.process_ffi_name(name)
      res = name.to_s.gsub(/_t\z/, "").split("_").collect(&:capitalize).join
      res.gsub(/\AZet/,"ZET").gsub(/\AZe/, "ZE").gsub("Uuid","UUID").gsub("Dditable", "DDITable").gsub(/\AFp/, "FP").gsub("Ipc", "IPC").gsub("P2p", "P2P")
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

  class Driver < ZEObject
    add_property :api_version, sname: :ZEApiVersion, memoize: true
    add_property :properties, memoize: true
    add_property :ipc_properties, memoize: true

    def extension_function(name, return_type, param_types, **options )
      p_name = MemoryPointer.from_string(name.to_s)
      pptr = MemoryPointer::new(:pointer)
      result = ZE.zeDriverGetExtensionFunctionAddress(@handle, p_name, pptr)
      ZE.error_check(result)
      return Function::new(return_type, param_types, pptr.read_pointer, options)
    end

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

    def mem_alloc_properties(ptr)
      props = ZEMemoryAllocationProperties::new
      ph_device = MemoryPointer::new(:ze_device_handle_t)
      result = ZE.zeDriverGetMemAllocProperties(@handle, ptr, props, ph_device)
      ZE.error_check(result)
      h_device = ph_device.read_ze_device_handle_t
      device = h_device.null? ? nil : Device::new(h_device, self)
      return [props, device]
    end

    def mem_address_range(ptr)
      p_base = MemoryPointer::new(:pointer)
      p_size = MemoryPointer::new(:size_t)
      result = ZE.zeDriverGetMemAddressRange(@handle, ptr, p_base, p_size)
      ZE.error_check(result)
      return p_base.read_pointer.slice(0, p_size.read_size_t)
    end

    def mem_ipc_handle(ptr)
      ipc_handle = ZEIPCMemHandle.new
      result = ZE.zeDriverGetMemIpcHandle(@handle, ptr, ipc_handle)
      ZE.error_check(result)
      return ipc_handle
    end

    def open_mem_ipc_handle(device, ipc_handle, flags: 0)
      pptr = MemoryPointer::new(:pointer)
      result = ZE.zeDriverOpenMemIpcHandle(@handle, device, ipc_handle, flags, pptr)
      ZE.error_check(result)
      return pptr.read_pointer
    end

    def close_mem_ipc_handle(ptr)
      result = ZE.zeDriverCloseMemIpcHandle(@handle, ptr)
      ZE.error_check(result)
      return self
    end

    def sysman_event_listen(events, timeout: ZET_EVENT_WAIT_NONE)
      count = events.length
      ph_events = MemoryPointer::new(:zet_sysman_event_handle_t, count)
      ph_events.write_array_of_zet_sysman_event_handle_t(events.collect(&:to_ptr))
      p_events = MemoryPointer::new(:uint32_t)
      result = ZE.zetSysmanEventListen(@handle, timeout, count, ph_events, p_events)
      ZE.error_check(result)
      p_events.read_array_of_uint32(count).collect { |e| ZETSysmanEventType.from_native(e, nil) }
    end

  end

  class Device < ZEObject
    attr_reader :driver

    def initialize(handle, driver)
      super(handle)
      @driver = driver
    end

    add_property :properties, memoize: true
    add_object_array :sub_devices, :Device, :zeDeviceGetSubDevices, memoize: true
    add_property :compute_properties, memoize: true
    add_property :kernel_properties, memoize: true
    add_array_property :memory_properties, :ZEDeviceMemoryProperties, :zeDeviceGetMemoryProperties, memoize: true
    add_property :memory_access_properties, memoize: true
    add_property :cache_properties, memoize: true
    add_property :image_properties, memoize: true

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

    def make_memory_resident(ptr, size: nil)
      size = ptr.to_ptr.size unless size
      result = ZE.zeDeviceMakeMemoryResident(@handle, ptr, size)
      ZE.error_check(result)
      ptr.to_ptr.slice(0, size)
    end

    def evict_memory(ptr, size: nil)
      size = ptr.to_ptr.size unless size
      result = ZE.zeDeviceEvictMemory(@handle, ptr, size)
      ZE.error_check(result)
      ptr.to_ptr.slice(0, size)
    end

    def make_image_resident(image)
      result = ZE.zeDeviceMakeImageResident(@handle, image)
      ZE.error_check(result)
      self
    end

    def evict_image(image)
      result = ZE.zeDeviceEvictImage(@handle, image)
      ZE.error_check(result)
      self
    end

    def register_cl_memory(mem, context: nil)
      context = mem.context unless context
      ptr = MemoryPointer::new(:pointer)
      result = ZE.zeDeviceRegisterCLMemory(@handle, context, mem, ptr)
      ZE.error_check(result)
      ptr.read_pointer.slice(0, mem.size)
    end

    def register_cl_program(program, context: nil)
      context = program.context unless context
      ptr = MemoryPointer::new(:ze_module_handle_t)
      result = ZE.zeDeviceRegisterCLProgram(@handle, context, program, ptr)
      ZE.error_check(result)
      Module::new(ptr.read_ze_module_handle_t)
    end

    def register_cl_command_queue(command_queue, context: nil)
      context = command_queue.context unless context
      ptr = MemoryPointer::new(:ze_command_queue_handle_t)
      result = ZE.zeDeviceRegisterCLCommandQueue(@handle, context, command_queue, ptr)
      ZE.error_check(result)
      CommandQueue::new(ptr.read_ze_command_queue_handle_t)
    end

    def sampler_create(address_mode:, filter_mode:, normalized:)
      desc = ZESamplerDesc::new
      desc[:addressMode] = address_mode
      desc[:filterMode] = filter_mode
      desc[:isNormalized] = normalized
      ptr = MemoryPointer::new(:ze_sampler_handle_t)
      result = ZE.zeSamplerCreate(@handle, desc, ptr)
      ZE.error_check(result)
      Sampler::new(ptr.read_ze_sampler_handle_t)
    end

    def sysman
      ph_sysman = MemoryPointer::new(:zet_sysman_handle_t)
      result = ZE.zetSysmanGet(@handle, @driver.api_version.to_ptr, ph_sysman)
      ZE.error_check(result)
      Sysman::new(ph_sysman.read_zet_sysman_handle_t)
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

    def synchronize(timeout: ZE::UINT32_MAX)
      result = ZE.zeCommandQueueSynchronize(@handle, timeout)
      ZE.error_check(result)
      result
    end

    def fence_create(flags: 0)
      desc = ZEFenceDesc::new
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
      result = ZE.zeCommandListAppendMemoryRangesBarrier(@handle, num_ranges, pRangesSizes, pRanges, signal_event, count, ph_wait_events)
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

    def append_memory_copy(dstptr, srcptr, size: nil, signal_event: nil)
      unless size
        size = [dstptr.size, srcptr.size].min
      end
      result = ZE.zeCommandListAppendMemoryCopy(@handle, dstptr, srcptr, size, signal_event)
      ZE.error_check(result)
      self
    end

    def append_memory_fill(ptr, pattern, size: nil, pattern_size: nil, signal_event: nil)
      size = ptr.size unless size
      pattern_size = pattern.size unless size
      result = zeCommandListAppendMemoryFill(@handle, ptr, pattern, pattern_size, size, signal_event)
      ZE.error_check(result)
      self
    end

    def append_memory_copy_region(dstptr, srcptr, width, height, depth = 0,
                                  dst_origin: [0, 0, 0], src_origin: [0, 0, 0],
                                  dst_pitch: width, src_pitch: width,
                                  dst_slice_pitch: depth == 0 ? 0 : dst_pitch * height,
                                  src_slice_pitch: depth == 0 ? 0 : src_pitch * height,
                                  signal_event: nil)
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
      result = ZE.zeCommandListAppendMemoryCopyRegion(@handle, dstptr, dst_region, dst_pitch, dst_slice_pitch, srcptr, src_region, src_pitch, src_slice_pitch, signal_event)
      ZE.error_check(result)
      self
    end

    def append_image_copy(dst_image, src_image, signal_event: nil)
      result = ZE.zeCommandListAppendImageCopy(@handle, dst_image, src_image, signal_event)
      ZE.error_check(result)
      self
    end

    def append_image_copy_region(dst_image, src_image, width, height = 1, depth = 1,
                                 dst_origin: [0, 0, 0], src_origin: [0, 0, 0],
                                 signal_event: nil)
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
      result = ZE.zeCommandListAppendImageCopyRegion(@handle, dst_image, src_image, dst_region, src_region, signal_event)
      ZE.error_check(result)
      self
    end

    def append_image_copy_to_memory(dstptr, image, ranges: nil, signal_event: nil)
      src_region = nil
      src_region = _create_image_region_from_ranges(ranges) if ranges
      result = ZE.zeCommandListAppendImageCopyToMemory(@handle, dstptr, image, src_region, signal_event)
      ZE.error_check(result)
      self
    end

    def append_image_copy_from_memory(image, srcptr, ranges: nil, signal_event: nil)
      dst_region = nil
      dst_region = _create_image_region_from_ranges(ranges) if ranges
      result = ZE.zeCommandListAppendImageCopyFromMemory(@handle, image, srcptr, dst_region, signal_event)
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

    private
    def _create_event_list(wait_events)
      count = 0
      ph_wait_events = nil
      if wait_events
        events = [wait_events].flatten
        count = events.length
        if count > 0
          ph_wait_events = MemoryPointer::new(:ze_event_handle_t, count)
          ph_wait_events.write_array_of_event_handle_t(events.collect(&:handle).collect(&:address))
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

  class Kernel < ZEManagedObject
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
      return Event::new(ph_event.read_ze_event_handle_t)
    end

    def ipc_handle
      ph_ipc = ZEIPCEventPoolHandle::new
      result = ZE.zeEventPoolGetIpcHandle(@handle, ph_ipc)
      ZE.error_check(result)
      return ph_ipc
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

    def host_synchronize(timeout: ZE::UINT32_MAX)
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

    def timestamp(type)
      dstptr = MemoryPointer::new(:uint64_t)
      result = ZE.zeEventGetTimestamp(@handle, type, dstptr)
      ZE.error_check(result)
      dstptr.read_uint64
    end

    def timestamps
      ZEEventTimestampType.symbol_map.collect { |k, v| [k, timestamp(v)] }.to_h
    end
  end

  class Sampler < ZEManagedObject
    @destructor = :zeSamplerDestroy
  end

  class Fence < ZEManagedObject
    @destructor = :zeFenceDestroy

    def host_synchronize(timeout: ZE::UINT32_MAX)
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

  class Sysman < ZETObject
    add_property :device_properties, sname: :ZETSysmanProperties, fname: :zetSysmanDeviceGetProperties, memoize: true
    add_enum_array_property :scheduler_supported_modes, :zet_sched_mode_t, :zetSysmanSchedulerGetSupportedModes, memoize: true
    add_enum_property :scheduler_current_mode, :zet_sched_mode_t, :zetSysmanSchedulerGetCurrentMode
    add_enum_array_property :performance_profile_supported, :zet_perf_profile_t, :zetSysmanPerformanceProfileGetSupported, memoize: true
    add_enum_property :performance_profile, :zet_perf_profile_t, :zetSysmanPerformanceProfileGet
    add_array_property :processes_state, :ZETProcessState, :zetSysmanProcessesGetState
    add_enum_property :device_repair_status, :zet_repair_status_t, :zetSysmanDeviceGetRepairStatus
    add_property :pci_properties, sname: :ZETPciProperties, fname: :zetSysmanPciGetProperties, memoize: true
    add_property :pci_state, sname: :ZETPciState, fname: :zetSysmanPciGetState
    add_array_property :pci_bars, :ZETPciBarProperties, :zetSysmanPciGetBars
    add_property :pci_stats, sname: :ZETPciStats, fname: :zetSysmanPciGetStats
    add_object_array :powers, :Pwr, :zetSysmanPowerGet, memoize: true
    add_object_array :frequencies, :Freq, :zetSysmanFrequencyGet, memoize: true
    add_object_array :engines, :Engine, :zetSysmanEngineGet, memoize: true
    add_object_array :standbies, :Standby, :zetSysmanStandbyGet, memoize: true
    add_object_array :firmwares, :Firmware, :zetSysmanFirmwareGet, memoize: true
    add_object_array :memories, :Mem, :zetSysmanMemoryGet, memoize: true
    add_object_array :fabric_ports, :FabricPort, :zetSysmanFabricPortGet, memoize: true
    add_object_array :temperatures, :Temp, :zetSysmanTemperatureGet, memoize: true
    add_object_array :psus, :Psu, :zetSysmanPsuGet, memoize: true
    add_object_array :fans, :Fan, :zetSysmanFanGet, memoize: true
    add_object_array :leds, :Led, :zetSysmanLedGet, memoize: true
    add_object_array :rases, :Ras, :zetSysmanRasGet, memoize: true
    add_object_array :diagnostics, :Diag, :zetSysmanDiagnosticsGet, memoize: true

    def scheduler_timeout_mode_properties(default: false)
      config = ZETSchedTimeoutProperties::new
      result = ZE.zetSysmanSchedulerGetTimeoutModeProperties(@handle, default ? 1 : 0, config)
      ZE.error_check(result)
      config
    end

    def scheduler_timeslice_mode_properties(default: false)
      config = ZETSchedTimesliceProperties::new
      result = ZE.zetSysmanSchedulerGetTimesliceModeProperties(@handle, default ? 1 : 0, config)
      ZE.error_check(result)
      config
    end

    def scheduler_set_timeout_mode(watchdog_timeout)
      properties = ZETSchedTimeoutProperties::new
      properties[:watchdogTimeout]
      p_need_reboot = MemoryPointer::new(:ze_bool_t)
      result = ZE.zetSysmanSchedulerSetTimeoutMode(@handle, properties, p_need_reboot)
      ZE.error_check(result)
      p_need_reboot.read_ze_bool_t != 0
    end

    def scheduler_set_timeslice_mode(interval, yield_timeout)
      properties = ZETSchedTimesliceProperties::new
      properties[:interval] = interval
      properties[:yieldTimeout] = yield_timeout
      p_need_reboot = MemoryPointer::new(:ze_bool_t)
      result = ZE.zetSysmanSchedulerSetTimesliceMode(@handle, properties, p_need_reboot)
      ZE.error_check(result)
      p_need_reboot.read_ze_bool_t != 0
    end

    def scheduler_set_exclusive_mode
      p_need_reboot = MemoryPointer::new(:ze_bool_t)
      result = ZE.zetSysmanSchedulerSetExclusiveMode(@handle, p_need_reboot)
      ZE.error_check(result)
      p_need_reboot.read_ze_bool_t != 0
    end

    def scheduler_set_compute_unit_debug_mode
      p_need_reboot = MemoryPointer::new(:ze_bool_t)
      result = ZE.zetSysmanSchedulerSetComputeUnitDebugMode(@handle, p_need_reboot)
      ZE.error_check(result)
      p_need_reboot.read_ze_bool_t != 0
    end

    def performace_profile_set(profile)
      result = ZE.zetSysmanPerformanceProfileSet(@handle, profile)
      ZE.error_check(result)
      profile
    end
    alias performace_profile= performace_profile_set

    def device_reset
      result = ZE.zetSysmanDeviceReset(@handle)
      ZE.error_check(result)
      self
    end

    def event
      ph_event = MemoryPointer::new(:zet_sysman_event_handle_t)
      result = ZE.zetSysmanEventGet(@handle, ph_event)
      ZE.error_check(result)
      Event::new(ph_event.read_zet_sysman_event_handle_t)
    end

  end

  class Sysman::Pwr < ZETObject
    add_property :properties, sname: :ZETPowerProperties, fname: :zetSysmanPowerGetProperties, memoize: true
    add_property :energy_counter, sname: :ZETPowerEnergyCounter, fname: :zetSysmanPowerGetEnergyCounter
    add_property :energy_threshold, sname: :ZETEnergyThreshold, fname: :zetSysmanPowerGetEnergyThreshold

    def limits
      sustained_limit = ZETPowerSustainedLimit::new
      burst_limit = ZETPowerBurstLimit::new
      peak_limit = ZETPowerPeakLimit::new
      result = ZE.zetSysmanPowerGetLimits(@handle, sustained_limit, burst_limit, peak_limit)
      ZE.error_check(result)
      [sustained_limit, burst_limit, peak_limit]
    end

    def sustained_limit
      sustained_limit = ZETPowerSustainedLimit::new
      result = ZE.zetSysmanPowerGetLimits(@handle, sustained_limit, nil, nil)
      ZE.error_check(result)
      sustained_limit
    end

    def burst_limit
      burst_limit = ZETPowerBurstLimit::new
      result = ZE.zetSysmanPowerGetLimits(@handle, nil, burst_limit, nil)
      ZE.error_check(result)
      burst_limit
    end

    def peak_limit
      peak_limit = ZETPowerPeakLimit::new
      result = ZE.zetSysmanPowerGetLimits(@handle, nil, nil, peak_limit)
      ZE.error_check(result)
      peak_limit
    end

    def set_limits(sustained_enabled: false, sustained_power: 0, sustained_interval: 0,
                   burst_enabled: false, burst_power: 0,
                   peak_power_ac: 0, peak_power_dc: 0)
      sustained_limit = ZETPowerSustainedLimit::new
      sustained_limit[:enabled] = sustained_enabled ? 1 : 0
      sustained_limit[:power] = sustained_power
      sustained_limit[:interval] = sustained_interval
      burst_limit = ZETPowerBurstLimit::new
      burst_limit[:enabled] = burst_enabled ? 1 : 0
      burst_limit[:power] = burst_power
      peak_limit = ZETPowerPeakLimit::new
      peak_limit[:powerAC] = peak_power_ac
      peak_limit[:powerDC] = peak_power_dc
      result = ZE.zetSysmanPowerSetLimits(@handle, sustained_limit, burst_limit, peak_limit)
      ZE.error_check(result)
      self
    end

    def set_sustained_limit(enabled, power, interval)
      sustained_limit = ZETPowerSustainedLimit::new
      sustained_limit[:enabled] = enabled ? 1 : 0
      sustained_limit[:power] = power
      sustained_limit[:interval] = interval
      result = ZE.zetSysmanPowerSetLimits(@handle, sustained_limit, nil, nil)
      ZE.error_check(result)
      self
    end

    def set_burst_limit(enabled, power)
      burst_limit = ZETPowerBurstLimit::new
      burst_limit[:enabled] = enabled ? 1 : 0
      burst_limit[:power] = power
      result = ZE.zetSysmanPowerSetLimits(@handle, nil, burst_limit, nil)
      ZE.error_check(result)
      self
    end

    def set_peak_limit(power_ac, power_dc)
      peak_limit = ZETPowerPeakLimit::new
      peak_limit[:powerAC] = power_ac
      peak_limit[:powerDC] = power_dc
      result = ZE.zetSysmanPowerSetLimits(@handle, nil, nil, peak_limit)
      ZE.error_check(result)
      self
    end

    def set_energy_threshold(threshold)
      result = ZE.zetSysmanPowerSetEnergyThreshold(@handle, threshold)
      ZE.error_check(result)
      threshold
    end
    alias energy_threshold= set_energy_threshold
  end

  class Sysman
    Power = Pwr
  end

  class Sysman::Freq < ZETObject
    add_property :properties, sname: :ZETFreqProperties, fname: :zetSysmanFrequencyGetProperties, memoize: true
    add_property :range, sname: :ZETFreqRange, fname: :zetSysmanFrequencyGetRange
    add_property :state, sname: :ZETFreqState, fname: :zetSysmanFrequencyGetState
    add_property :throttle_time, sname: :ZETFreqThrottleTime, fname: :zetSysmanFrequencyGetThrottleTime
    add_property :oc_capabilities, sname: :ZETOcCapabilities, fname: :zetSysmanFrequencyOcGetCapabilities
    add_property :oc_config, sname: :ZETOcConfig, fname: :zetSysmanFrequencyOcGetConfig

    def clocks
      pCount = MemoryPointer::new(:uint32_t)
      result = ZE.zetSysmanFrequencyGetAvailableClocks(@handle, pCount, nil)
      ZE.error_check(result)
      count = pCount.read(:uint32)
      return [] if count == 0
      pArr = MemoryPointer::new(:double, count)
      result = ZE.zetSysmanFrequencyGetAvailableClocks(@handle, pCount, pArr)
      ZE.error_check(result)
      return pArr.read_array_of_double(count)
    end

    def set_range(min, max)
      limits = ZETFreqRange::new
      limits[:min] = min
      limits[:max] = max
      result = ZE.zetSysmanFrequencySetRange(@handle, limits)
      ZE.error_check(result)
      limits
    end

    def oc_set_config(mode, frequency, voltage_target, voltage_offset)
      config = ZETOcConfig::new
      config[:mode] = mode
      config[:frequency] = frequency
      config[:voltageTarget] = voltage_target
      config[:voltageOffset] = voltage_offset
      p_need_reboot = MemoryPointer::new(:ze_bool_t)
      result = ZE.zetSysmanFrequencyOcSetConfig(@handle, config, p_need_reboot)
      ZE.error_check(result)
      p_need_reboot.read_ze_bool_t != 0
    end

    def oc_icc_max
      p_oc_icc_max = MemoryPointer::new(:double)
      result = ZE.zetSysmanFrequencyOcGetIccMax(@handle, p_oc_icc_max)
      ZE.error_check(result)
      p_oc_icc_max.read_double
    end

    def oc_set_icc_max(oc_icc_max)
      result = ZE.zetSysmanFrequencyOcSetIccMax(@handle, oc_icc_max)
      ZE.error_check(result)
      oc_icc_max
    end
    alias oc_icc_max= oc_set_icc_max

    def oc_tj_max
      p_oc_tj_max = MemoryPointer::new(:double)
      result = ZE.zetSysmanFrequencyOcGetTjMax(@handle, p_oc_tj_max)
      ZE.error_check(result)
      p_oc_tj_max.read_double
    end

    def oc_set_tj_max(tj_max)
      result = ZE.zetSysmanFrequencyOcSetTjMax(@handle, tj_max)
      ZE.error_check(result)
      tj_max
    end
    alias oc_tj_max= oc_set_tj_max
  end

  class Sysman
    Frequency = Freq
  end

  class Sysman::Engine < ZETObject
    add_property :properties, sname: :ZETEngineProperties, fname: :zetSysmanEngineGetProperties
    add_property :activity, sname: :ZETEngineStats, fname: :zetSysmanEngineGetActivity
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

  class Sysman::Standby < ZETObject
    add_property :properties, sname: :ZETStandbyProperties, fname: :zetSysmanStandbyGetProperties, memoize: true
    add_enum_property :mode, :zet_standby_promo_mode_t, :zetSysmanStandbyGetMode

    def set_mode(mode)
      result = ZE.zetSysmanStandbySetMode(@handle, mode)
      ZE.error_check(result)
      mode
    end
    alias mode= set_mode
  end

  class Sysman::Firmware < ZETObject
    add_property :properties, sname: :ZETFirmwareProperties, fname: :zetSysmanFirmwareGetProperties

    def checksum
      p_checksum = MemoryPointer::new(:uint32_t)
      result = ZE.zetSysmanFirmwareGetChecksum(@handle, p_checksum)
      ZE.error_check(result)
      p_checksum.read_uint32
    end

    def flash(firmware)
      size = firmware.bytesize
      p_image = MemoryPointer::new(size)
      p_image.write_bytes(firmware)
      result = ZE.zetSysmanFirmwareFlash(@handle, p_image, size)
      ZE.error_check(result)
      self
    end
  end

  class Sysman::Mem < ZETObject
    add_property :properties, sname: :ZETMemProperties, fname: :zetSysmanMemGetProperties, memoize: true
    add_property :state, sname: :ZETMemState, fname: :zetSysmanMemoryGetState
    add_property :bandwidth, sname: :ZETMemBandwidth, fname: :zetSysmanMemoryGetBandwidth
  end

  class Sysman
    Memory = Mem
  end

  class Sysman::FabricPort < ZETObject
    add_property :properties, sname: :ZETFabricPortProperties, fname: :zetSysmanFabricPortGetProperties, memoize: true
    add_property :config, sname: :ZETFabricPortConfig, fname: :zetSysmanFabricPortGetConfig
    add_property :state, sname: :ZETFabricPortState, fname: :zetSysmanFabricPortGetState
    add_property :throughput, sname: :ZETFabricPortThroughput, fname: :zetSysmanFabricPortGetThroughput

    def link_type(verbose: false)
      link_type = ZETFabricLinkType::new
      result = ZE.zetSysmanFabricPortGetLinkType(@handle, verbose ? 1 : 0, link_type)
      ZE.error_check(result)
      link_type
    end

    def set_config(enabled:, beaconing:)
      config = ZETFabricPortConfig::new
      config[:enabled] = enabled ? 1 : 0
      config[:beaconing] = beaconing ? 1 : 0
      result = ZE.zetSysmanFabricPortSetConfig(@handle, config)
      ZE.error_check(result)
      self
    end
  end

  class Sysman::Temp < ZETObject
    add_property :properties, sname: :ZETTempProperties, fname: :zetSysmanTemperatureGetProperties, memoize: true
    add_property :config, sname: :ZETTempConfig, fname: :zetSysmanTemperatureGetConfig

    def set_config(enable_critical:, threshold1:, threshold2:)
      config = ZETTempConfig::new
      config[:enableCritical] = enable_critical ? 1 : 0
      config[:threshold1] = threshold1
      config[:threshold2] = threshold2
      result = ZE.zetSysmanTemperatureSetConfig(@handle, config)
      ZE.error_check(result)
      self
    end

    def state
      p_temperature = MemoryPointer::new(:double)
      result = zetSysmanTemperatureGetState(@handle, p_temperature)
      ZE.error_check(result)
      p_temperature.read_double
    end
  end

  class Sysman
    Temperature = Temp
  end

  class Sysman::Psu < ZETObject
    add_property :properties, sname: :ZETPsuProperties, fname: :zetSysmanPsuGetProperties, memoize: true
    add_property :state, sname: :ZETPsuState, fname: :zetSysmanPsuGetState
  end

  class Sysman::Fan < ZETObject
    add_property :properties, sname: :ZETFanProperties, fname: :zetSysmanFanGetProperties, memoize: true
    add_property :config, sname: :ZETFanConfig, fname: :zetSysmanFanGetConfig

    def set_config(mode: , speed: 0, speed_units: :ZET_FAN_SPEED_UNITS_PERCENT, points: [])
      config = ZETFanConfig::new
      config[:mode] = mode
      config[:speed] = speed
      config[:speedUnits] = speed_units
      config[:numPoints] = points.length
      points.each_with_index { |args, i|
        t = config[:table][i]
        t[:temperature] = args[0]
        t[:speed] = args[1]
        t[:units] = args[2] || :ZET_FAN_SPEED_UNITS_PERCENT 
      }
      result = ZE.zetSysmanFanSetConfig(@handle, config)
      ZE.error_check(result)
      self
    end

    def state(units: :ZET_FAN_SPEED_UNITS_PERCENT)
      p_speed = MemoryPointer::new(:uint32_t)
      result = ZE.zetSysmanFanGetState(@handle, units, p_speed)
      ZE.error_check(result)
      return p_speed.read_uint32
    end
  end

  class Sysman::Led < ZETObject
    add_property :properties, sname: :ZETLedProperties, fname: :zetSysmanLedGetProperties, memoize: true
    add_property :state, sname: :ZETLedState, fname: :zetSysmanLedGetState

    def set_state(is_on:, red:, green:, blue:)
      normalize = lambda { |c| (c.kind_of?(Float) ? (c*255).to_i : c).clamp(0, 255) }
      state = ZETLedState::new
      state[:isOn] = is_on ? 1 : 0
      state[:red] = normalize.call(red)
      state[:green] = normalize.call(green)
      state[:blue] = normalize.call(blue)
      result = ZE.zetSysmanLedSetState(@handle, state)
      ZE.error_check(result)
      self
    end
  end

  class Sysman::Ras < ZETObject
    add_property :properties, sname: :ZETRasProperties, fname: :zetSysmanRasGetProperties, memoize: true
    add_property :config, sname: :ZETRasConfig, fname: :zetSysmanRasGetConfig

    def set_config(total_threshold: 0,
                   num_resets: 0,
                   num_programming_errors: 0,
                   num_driver_errors: 0,
                   num_compute_errors: 0,
                   num_non_compute_errors: 0,
                   num_cache_errors: 0,
                   num_display_errors: 0)
      config = ZETRasConfig::new
      config[:totalThreshold] = total_threshold
      detailed_thresholds = config[:detailedThresholds]
      detailed_thresholds[:numResets] = num_resets
      detailed_thresholds[:numProgrammingErrors] = num_programming_errors
      detailed_thresholds[:numDriverErrors] = num_driver_errors
      detailed_thresholds[:numComputeErrors] = num_compute_errors
      detailed_thresholds[:numNonComputeErrors] = num_non_compute_errors
      detailed_thresholds[:numCacheErrors] = num_cache_errors
      detailed_thresholds[:numDisplayErrors] = num_display_errors
      result = ZE.zetSysmanRasSetConfig(@handle, config)
      ZE.error_check(result)
      self
    end

    def state(clear: false)
      details = ZETRasDetails::new
      p_total_errors = MemoryPointer::new(:uint64_t)
      result = ZE.zetSysmanRasGetState(@handle, clear ? 1 : 0, p_total_errors, details)
      ZE.error_check(result)
      [ p_total_errors.read_uint64, details ]
    end
  end

  class Sysman::Event < ZETObject
    add_property :config, sname: :ZETEventConfig, fname: :zetSysmanEventGetConfig

    def set_config(registered)
      config = ZETEventConfig::new
      config[:registered] = ZETSysmanEventType.to_native(registered, nil)
      result = ZE.zetSysmanEventSetConfig(@handle, config)
      ZE.error_check(result)
      self
    end

    def state(clear: false)
      p_events = MemoryPointer::new(:zet_sysman_event_type_t)
      result = ZE.zetSysmanEventGetState(@handle, clear ? 1 : 0, p_events)
      ZE.error_check(result)
      ZETSysmanEventType.from_native(p_events.read_zet_sysman_event_type_t, nil)
    end
  end

  class Sysman::Diag < ZETObject
    add_property :properties, sname: :ZETDiagProperties, fname: :zetSysmanDiagnosticsGetProperties, memoize: true
    add_array_property :tests, :ZETDiagTest, :zetSysmanDiagnosticsGetTests, memoize: true

    def run_tests(start: ZET_DIAG_FIRST_TEST_INDEX, stop: ZET_DIAG_LAST_TEST_INDEX)
      res = ZETDiagResult::new
      result = ZE.zetSysmanDiagnosticsRunTests(@handle, start, stop, res)
      ZE.error_check(result)
      res
    end
  end

  class Sysman
    Diagnostics = Diag
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
