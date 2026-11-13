# What the generated OpenCL event printers (babeltrace_opencl_lib.rb) call for
# the values whose meaning their type does not give: a device-partition
# property list, and what a clGet*Info call returned.
require_relative 'cl_library'

module CL
  # A device-partition property list (clCreateSubDevices(EXT) properties,
  # CL_DEVICE_PARTITION_TYPE): keys, each followed by one value or by a list of
  # values ended by its own terminator, and a 0 after the last key. Each element
  # is named by its place in the list: keys by the key enum, values by their key,
  # terminators by their list.
  class PartitionList
    # The core list is traced signed (intptr_t), the EXT one unsigned (cl_ulong).
    MASK64 = (1 << 64) - 1

    # keys: value => key name. kinds: key name => [terminator name, terminator
    # value] for a key that takes a list, or a lambda naming its one value. Any
    # other key takes one plain value.
    def initialize(keys, kinds)
      @keys = keys
      @kinds = kinds
    end

    def to_a(vals)
      res = []
      idx = 0
      while idx < vals.size
        key = vals[idx]
        return res.push(0).concat(vals[(idx + 1)..]) if key.zero?

        res << (@keys[key] || key)
        idx = take_values(@kinds[res.last], vals, idx + 1, res)
      end
      res
    end

    private

    # Append the values after a key, from vals[idx], to res. Return the next index.
    def take_values(kind, vals, idx, res)
      return take_list(*kind, vals, idx, res) if kind.is_a?(Array)
      return idx if idx >= vals.size

      res << (kind ? kind.call(vals[idx]) : vals[idx])
      idx + 1
    end

    # Append the values up to the list terminator, then the terminator's name.
    def take_list(term_name, term, vals, idx, res)
      len = vals[idx..].index { |v| (v & MASK64) == (term & MASK64) }
      unless len
        res.concat(vals[idx..])
        return vals.size
      end
      res.concat(vals[idx, len]) << term_name
      idx + len + 1
    end
  end

  # value => name of an enum's constants, as strings.
  def self.names(enum)
    enum.to_h.to_h { |name, value| [value, name.to_s] }
  end

  # Terminators are not keys: outside its list, a 0 is a 0. And
  # cl_intel_device_partition_by_names (cl_ext_intel.h) is not in cl.xml.
  PARTITION_KEYS = [DevicePartitionProperty, DevicePartitionPropertyExt].to_h do |enum|
    [enum, names(enum).reject { |_, name| name.include?('LIST_END') }]
  end
  PARTITION_KEYS[DevicePartitionProperty][0x4052] ||= 'CL_DEVICE_PARTITION_BY_NAMES_INTEL'

  list_end = ->(enum, name, default = nil) { [name, enum.to_h.fetch(name.to_sym, default)] }
  one_name = lambda { |enum|
    lambda { |val|
      v = enum.from_native(val, nil)
      v.is_a?(Array) && v.size != 1 ? "[#{v.join(', ')}]" : [v].flatten.first.to_s
    }
  }
  PARTITION_LISTS = {
    DevicePartitionProperty => PartitionList.new(
      PARTITION_KEYS[DevicePartitionProperty], {
        'CL_DEVICE_PARTITION_BY_COUNTS' =>
          list_end.call(DevicePartitionProperty, 'CL_DEVICE_PARTITION_BY_COUNTS_LIST_END'),
        'CL_DEVICE_PARTITION_BY_NAMES_INTEL' =>
          list_end.call(DevicePartitionProperty, 'CL_PARTITION_BY_NAMES_LIST_END_INTEL', -1),
        'CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN' => one_name.call(DeviceAffinityDomain),
      }
    ),
    DevicePartitionPropertyExt => PartitionList.new(
      PARTITION_KEYS[DevicePartitionPropertyExt], {
        'CL_DEVICE_PARTITION_BY_COUNTS_EXT' =>
          list_end.call(DevicePartitionPropertyExt, 'CL_PARTITION_BY_COUNTS_LIST_END_EXT'),
        'CL_DEVICE_PARTITION_BY_NAMES_EXT' =>
          list_end.call(DevicePartitionPropertyExt, 'CL_PARTITION_BY_NAMES_LIST_END_EXT'),
        'CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN_EXT' => one_name.call(AffinityDomainExt),
      }
    ),
  }.freeze

  # A cl_version packs major.minor.patch in 10, 10 and 12 bits.
  module VersionedName
    def to_s
      v = self[:version]
      "{ name: #{self[:name]}, version: #{v >> 22}.#{(v >> 12) & 0x3ff}.#{v & 0xfff} }"
    end
  end
  [NameVersion, NameVersionKhr].each { |named| named.prepend VersionedName }

  # The value a clGet*Info call returned for `key`, read as INFOS says: the
  # unpack format of its bytes, whether it is an array, the class that names
  # its values, and :partition_list for the one that is a partition list.
  def self.info_to_s(infos, key, bytes)
    format, array, klass, kind = infos[key]
    return bytes.inspect if bytes.empty? || format.nil?

    v = info_values(bytes, format, klass, kind)
    return "[ #{v.join(', ')} ]" if array

    klass || format != 'Z*' ? v.first : v.first.inspect
  end

  def self.info_values(bytes, format, klass, kind)
    if kind == :partition_list
      PARTITION_LISTS.fetch(klass).to_a(bytes.unpack(format))
    elsif klass.is_a?(Class)
      p = FFI::MemoryPointer.from_string(bytes)
      (bytes.size / klass.size).times.collect { |i| klass.new(p + (i * klass.size)).to_s }
    elsif PARTITION_KEYS.key?(klass)
      bytes.unpack(format).collect { |val| PARTITION_KEYS[klass][val] || val }
    elsif klass.is_a?(FFI::Bitmask)
      bytes.unpack(format).collect { |val| "[#{klass.from_native(val, nil).join(', ')}]" }
    elsif klass
      bytes.unpack(format).collect { |val| klass.from_native(val, nil) }
    elsif format.start_with?('J')
      bytes.unpack(format).collect { |val| format('0x%016x', val) }
    else
      bytes.unpack(format)
    end
  end
end
