require 'yaml'

begin
  require 'opencl_ruby_ffi/opencl_types'
  require 'opencl_ruby_ffi/opencl_arithmetic_gen'
  require 'opencl_ruby_ffi/opencl_ruby_ffi_base_gen'
  require 'opencl_ruby_ffi/opencl_ruby_ffi_base'
rescue LoadError
  module OpenCL
  end
end

opencl_model = YAML.load_file(File.join(DATADIR, 'opencl_model.yaml'))
CL_EVENT_SUFFIXES = opencl_model['suffixes']
CL_START = CL_EVENT_SUFFIXES['start']
CL_STOP = CL_EVENT_SUFFIXES['stop']
infos = YAML.load_file(File.join(DATADIR, 'opencl_infos.yaml'))
enums_by_type = {}
enums_by_value = {}
enums_by_name = {}
enum_values = {}
bitfields_by_type = {}
structs_by_type = {}

class Bitfield
  def self.to_s(v)
    res = []
    if v == 0
      default = const_get(:DEFAULT)
      res.push default if default
    else
      s = const_get(:SPECIAL)
      if s
        n, s = s
        if v & s == s
          res.push n
          v ^= s
        end
      end
      const_get(:FLAGS).each do |n, f|
        if f & v == f
          res.push(n)
          v ^= f
        end
      end
      res.push(v) if v != 0
    end
    "[#{res.join(', ')}]"
  end
end

opencl_model['enums'].collect do |k, v|
  hash = {}
  v['values'].each do |n, str|
    int =
      begin
        eval(str)
      rescue StandardError
        nil
      end
    next unless int

    hash[int] = n
    enum_values[n] = int
  end
  k = v['type_name'] if v['type_name']
  enums_by_type[k] = hash
  enums_by_value[k] = hash.invert
  enums_by_name[v['trace_name']] = hash if v['trace_name']
end

opencl_model['bitfields'].collect do |k, v|
  hash = {}
  default = nil
  special = nil
  v['values'].each do |n, str|
    int =
      begin
        eval(str)
      rescue StandardError
        nil
      end
    if int
      if int > 0
        if int.to_s(2).count('1') > 1
          special = [n, int]
        else
          hash[n] = int
        end
      else
        default = n
      end
    end
  end
  klass = Class.new(Bitfield) do
    if default
      const_set(:DEFAULT, default)
    else
      const_set(:DEFAULT, nil)
    end
    if special
      const_set(:SPECIAL, special)
    else
      const_set(:SPECIAL, nil)
    end
    const_set(:FLAGS, hash)
  end
  bitfields_by_type[k] = klass
end

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

# Terminators are not keys: outside its list, a 0 is a 0. And
# cl_intel_device_partition_by_names (cl_ext_intel.h) is not in cl.xml.
%w[cl_device_partition_property cl_device_partition_property_ext].each do |t|
  enums_by_type[t].reject! { |_, n| n.include?('LIST_END') }
end
enums_by_type['cl_device_partition_property'][0x4052] ||= 'CL_DEVICE_PARTITION_BY_NAMES_INTEL'
%w[cl_device_partition_property cl_device_partition_property_ext].each do |t|
  enums_by_value[t] = enums_by_type[t].invert
end
enum_values['CL_PARTITION_BY_NAMES_LIST_END_INTEL'] ||= -1

list_end = ->(name) { [name, enum_values.fetch(name)] }
flag_name = lambda { |type|
  b = bitfields_by_type.fetch(type)
  ->(val) { b.const_get(:FLAGS).key(val) || b.to_s(val) }
}
enum_name = lambda { |type|
  e = enums_by_type.fetch(type)
  ->(val) { e[val] || val }
}
partition_lists = {
  'cl_device_partition_property' => PartitionList.new(
    enums_by_type['cl_device_partition_property'], {
      'CL_DEVICE_PARTITION_BY_COUNTS' => list_end.call('CL_DEVICE_PARTITION_BY_COUNTS_LIST_END'),
      'CL_DEVICE_PARTITION_BY_NAMES_INTEL' => list_end.call('CL_PARTITION_BY_NAMES_LIST_END_INTEL'),
      'CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN' => flag_name.call('cl_device_affinity_domain'),
    }
  ),
  'cl_device_partition_property_ext' => PartitionList.new(
    enums_by_type['cl_device_partition_property_ext'], {
      'CL_DEVICE_PARTITION_BY_COUNTS_EXT' => list_end.call('CL_PARTITION_BY_COUNTS_LIST_END_EXT'),
      'CL_DEVICE_PARTITION_BY_NAMES_EXT' => list_end.call('CL_PARTITION_BY_NAMES_LIST_END_EXT'),
      'CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN_EXT' => enum_name.call('cl_affinity_domain_ext'),
    }
  ),
}

opencl_model['structures'].collect do |n|
  sname = n.sub(/\Acl_/, '').split('_').collect(&:capitalize).join
  next unless OpenCL.const_defined?(sname)

  t = OpenCL.const_get(sname)
  structs_by_type[n] = t
end

infos_type = {}

type_map = {
  'unsigned int' => 'L',
  'int' => 'l',
  'intptr_t' => 'j',
  'uintptr_t' => 'J',
  'size_t' => 'Q',
  'cl_bool' => 'l',
  'cl_int' => 'l',
  'cl_uint' => 'L',
  'cl_long' => 'q',
  'cl_ulong' => 'Q',
  'cl_short' => 's',
  'cl_ushort' => 'S',
  'cl_char' => 'c',
  'cl_uchar' => 'C',
  'cl_half' => 'S',
  'cl_float' => 'f',
  'cl_double' => 'd',
}
opencl_model['objects'].each do |n|
  type_map[n] = 'J'
end
# Unpack an enum as its C type: cl_context_properties and
# cl_device_partition_property are intptr_t, not 32 bits.
enums_by_type.each_key do |n|
  base = n
  4.times { base = opencl_model['type_map'][base] unless base.nil? || type_map.key?(base) }
  type_map[n] = type_map.fetch(base, 'l')
end
bitfields_by_type.keys.each do |n|
  type_map[n] = 'Q'
end

infos.each do |inf, vers|
  infos_type[inf] = {}
  vers.each do |_vername, list|
    list.each do |i|
      type =
        if i['array']
          if i['pointer']
            ['J*', true, nil]
          elsif type_map[i['type']]
            [type_map[i['type']] + '*', true, i['type']]
          elsif structs_by_type[i['type']]
            ['Z*', true, i['type']]
          end
        elsif i['string']
          ['Z*', false, nil]
        elsif i['pointer']
          ['J', false, nil]
        elsif type_map[i['type']]
          [type_map[i['type']], false, i['type']]
        elsif structs_by_type[i['type']]
          ['Z*', false, i['type']]
        end
      type += [:partition_list] if i['name'] == 'CL_DEVICE_PARTITION_TYPE'
      infos_type[inf][enums_by_value[inf][i['name']]] = type
    end
  end
end

info_str_lambda = lambda { |param_name, name, type|
  <<~EOF
    begin
      if defi["#{name}"].size > 0 && infos_type["#{type}"]
        i = infos_type["#{type}"][defi["#{param_name}"]];
        if i
          v = defi["#{name}"].unpack(i[0])
          v.collect! { |j| '0x' << j.to_s(16) } if i[0].match("J")
          if i[3] == :partition_list
            v = partition_lists[i[2]].to_a(v)
          elsif i[2]
            if enums_by_type[i[2]]
              v.collect! { |val| enums_by_type[i[2]][val] || val }
            elsif bitfields_by_type[i[2]]
              v.collect! { |val| bitfields_by_type[i[2]].to_s(val) }
            elsif structs_by_type[i[2]]
              p = FFI::MemoryPointer.from_string(defi["#{name}"])
              struct = structs_by_type[i[2]]
              sz = struct.size
              v = (p.size / sz).times.collect { |i| struct.new(p.slice(i*sz, sz)) }
            end
          end
          v = v.first unless i[1]
          v = v.inspect if i[0].match("Z\\\\*") && !i[2]
          v = "[ \#{v.join(", ")} ]" if i[1]
          v
        else
          defi["#{name}"].inspect
        end
      else
        defi["#{name}"].inspect
      end
    end
  EOF
}

opencl_model['events'].each do |n, v|
  src = <<~EOF
    lambda { |defi|
      {
  EOF
  src << '  '
  src << v.collect { |name, desc|
    expr =
      if name == 'param_value_vals'
        startn = n.gsub("_#{CL_STOP}", "_#{CL_START}")
        type = opencl_model['events'][startn]['param_name']['type']
        if n == "lttng_ust_opencl:clSetKernelExecInfo_#{CL_START}"
          info_str_lambda.call('param_name', name, type)
        else
          info_str_lambda.call('_param_name', name, type)
        end
      elsif desc['array']
        if partition_lists[desc['type']]
          "partition_lists[\"#{desc['type']}\"].to_a(defi[\"#{name}\"])"
        elsif enums_by_type[desc['type']]
          "defi[\"#{name}\"].collect { |v| (tmp = enums_by_type[\"#{desc['type']}\"][v]) ? tmp : v }"
        elsif bitfields_by_type[desc['type']]
          "defi[\"#{name}\"].collect { |v| bitfields_by_type[\"#{desc['type']}\"].to_s(v) }"
        elsif desc['pointer'] || opencl_model['objects'].include?(desc['type'])
          "\"[\#{defi[\"#{name}\"].collect { |v| \"0x\#{v.to_s(16)}\" }.join(\", \")}]\""
        elsif structs_by_type[desc['type']]
          "#{structs_by_type[desc['type']]}.new(FFI::MemoryPointer.from_string(defi[\"#{name}\"])).to_s"
        else
          "defi[\"#{name}\"].inspect"
        end
      elsif desc['string']
        "defi[\"#{name}\"].inspect"
      elsif enums_by_type[desc['type']]
        "(tmp = enums_by_type[\"#{desc['type']}\"][defi[\"#{name}\"]]) ? tmp : defi[\"#{name}\"]"
      elsif bitfields_by_type[desc['type']]
        "bitfields_by_type[\"#{desc['type']}\"].to_s(defi[\"#{name}\"])"
      elsif desc['pointer'] || opencl_model['objects'].include?(desc['type']) || desc['type'].match('void *')
        "\"0x\#{defi[\"#{name}\"].to_s(16)}\""
      elsif structs_by_type[desc['type']]
        "#{structs_by_type[desc['type']]}.new(FFI::MemoryPointer.from_string(defi[\"#{name}\"])).to_s"
      else # rubocop:disable Lint/DuplicateBranch
        "defi[\"#{name}\"].inspect"
      end
    "\"#{name}\" => (#{expr})"
  }.join(",\n  ")
  src << "\n"
  src << <<~EOF
      }.collect { |k,v| "\#{k}: \#{v}"}.join(", ")
    }
  EOF
  $event_lambdas[n] = eval(src)
end
