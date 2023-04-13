require_relative 'hip_model'
require_relative 'gen_probe_base.rb'

$all_types = $hip_api["typedefs"]
$all_structs = $hip_api["structs"]
$all_unions = $hip_api["unions"]
$all_enums = $hip_api["enums"]
$all_funcs = $hip_api["functions"]

$all_enum_names = []
$all_bitfield_names = []
$all_struct_names = []
$all_union_names = []

$objects = $all_types.select { |t|
  t.type.kind_of?(YAMLCAst::Pointer) &&
  t.type.type.kind_of?(YAMLCAst::Struct)
}.collect { |t| t.name }
$objects.push "hipGraphicsResource_t"

$all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && HIP_OBJECTS.include?(t.type.name)
    $objects.push t.name
  end
}

$int_scalars = {}
$all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && HIP_INT_SCALARS.include?(t.type.name)
    $int_scalars[t.name] = t.type.name
  end
}

def to_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').downcase
end

def to_class_name(name)
  mod = to_name_space(name)
  mod = "" unless mod
  n = name.gsub(/\A#{mod}/, "")
  mod.capitalize! if mod == 'hip'
  res = mod << n
  if res[0].match(/[[:lower:]]/)
    res[0] = res[0].upcase
  end
  res
end

def to_scoped_class_name(name)
  "HIP::#{to_class_name(name)}"
end

def to_ffi_name(name)
  case name
  when nil
    return ":anonymous"
  when "unsigned int"
    return ":uint"
  when "unsigned short", "unsigned short int"
    return ":ushort"
  when "unsigned char"
    return ":uchar"
  when "unsigned long long int"
    return ":uint64"
  when "size_t"
    return ":size_t"
  when "_Bool"
    return ":bool"
  end
  name.to_sym.inspect
end

def to_name_space(name)
  case name
  when /\Ahip/
    "hip"
  when /\AHIP/
    "HIP"
  else
    nil
  end
end

$all_types.each { |t|
  if t.type.kind_of? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    if false
      $all_bitfield_names.push t.name
    else
      $all_enum_names.push t.name
    end
  elsif t.type.kind_of? YAMLCAst::Struct
    $all_struct_names.push t.name
  elsif t.type.kind_of? YAMLCAst::Union
    $all_union_names.push t.name
  end
}

module YAMLCAst
  class Struct
    def to_ffi
      res = []
      members.each { |m|
        mt = case m.type
        when Array
          m.type.to_ffi
        when Pointer
          ":pointer"
        else
          if !m.type.name
            print_lambda = lambda { |m|
              s = "#{m[0]}, "
              if m[1].kind_of?(::Array)
                s << "[ #{m[1][0]}, #{m[1][1]} ]"
              else
                s << "#{m[1]}"
              end
              s
            }
            case m.type
            when Struct
              membs = m.type.to_ffi
              "(Class::new(FFI::HIPStruct) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
            when Union
              membs = m.type.to_ffi
              "(Class::new(FFI::HIPUnion) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
            else
              raise "Error type unknown!"
            end
          else
            to_ffi_name(m.type.name)
          end
        end
        res.push [m.name.to_sym.inspect, mt]
      }
      res
    end
  end

  class Union
    def to_ffi
      res = []
      members.each { |m|
        mt = case m.type
        when Array
          m.type.to_ffi
        when Pointer
          ":pointer"
        else
          if !m.type.name
            print_lambda = lambda { |m|
              s = "#{m[0]}, "
              if m[1].kind_of?(::Array)
                s << "[ #{m[1][0]}, #{m[1][1]} ]"
              else
                s << "#{m[1]}"
              end
              s
            }
            case m.type
            when Struct
              membs = m.type.to_ffi
              "(Class::new(FFI::HIPStruct) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
            when Union
              membs = m.type.to_ffi
              "(Class::new(FFI::HIPUnion) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
            else
              raise "Error type unknown!"
            end
          else
            to_ffi_name(m.type.name)
          end
        end
        res.push [m.name.to_sym.inspect, mt]
      }
      res
    end
  end

  class Array
    def to_ffi
      t = case type
      when Pointer
        ":pointer"
      else
       to_ffi_name(type.name)
      end
      [ t, length ]
    end
  end

  class Function
    def to_ffi
      t = to_ffi_name(type.name)
      p = params.collect { |par|
        if par.type.kind_of?(Pointer)
          if par.type.type.respond_to?(:name) &&
             $all_struct_names.include?(par.type.type.name)
            "#{to_class_name(par.type.type.name)}.ptr"
          else
            ":pointer"
          end
        else
          to_ffi_name(par.type.name)
        end
      }
      [t, p]
    end
  end
end
