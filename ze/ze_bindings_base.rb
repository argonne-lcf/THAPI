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

  class Driver < Object
    add_property :api_version, sname: :ZEApiVersion
    add_property :properties
    add_property :ipc_properties
    add_object_array :devices, :Device, :zeDeviceGet
  end

  class Device < Object
    add_property :properties
    add_object_array :sub_devices, :Device, :zeDeviceGetSubDevices
    add_property :compute_properties
    add_property :kernel_properties
    add_array_property :memory_properties, :ZEDeviceMemoryProperties, :zeDeviceGetMemoryProperties
    add_property :memory_access_properties
    add_property :cache_properties
    add_property :image_properties

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

    alias last_level_cache_config= set_last_level_cache_config
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
