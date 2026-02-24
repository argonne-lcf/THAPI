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
SUFFIXES = opencl_model['suffixes']
START = SUFFIXES['start']
STOP = SUFFIXES['stop']
infos = YAML.load_file(File.join(DATADIR, 'opencl_infos.yaml'))
enums_by_type = {}
enums_by_value = {}
enums_by_name = {}
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
    hash[int] = n if int
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
enums_by_type.keys.each do |n|
  type_map[n] = 'l'
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
          if i[2]
            if enums_by_type[i[2]]
              v.collect! { |val| enums_by_type[i[2]][val] }
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
        startn = n.gsub("_#{STOP}", "_#{START}")
        type = opencl_model['events'][startn]['param_name']['type']
        if n == "lttng_ust_opencl:clSetKernelExecInfo_#{START}"
          info_str_lambda.call('param_name', name, type)
        else
          info_str_lambda.call('_param_name', name, type)
        end
      elsif desc['array']
        if enums_by_type[desc['type']]
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
