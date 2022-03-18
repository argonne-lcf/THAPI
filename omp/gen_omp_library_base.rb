require_relative 'ompt_model'
require_relative 'gen_probe_base.rb'

$all_types = $ompt_api["typedefs"]
$all_structs = $ompt_api["structs"] 
$all_unions = $ompt_api["unions"]
$all_enums = $ompt_api["enums"]
$all_funcs = $ompt_api["functions"]

$all_enum_names = []
$all_bitfield_names = []
$all_struct_names = []
$all_union_names = []

$objects = $all_types.select { |t|
  t.type.kind_of?(YAMLCAst::Pointer) &&
  t.type.type.kind_of?(YAMLCAst::Struct)
}.collect { |t| t.name }

$all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && OMPT_OBJECTS.include?(t.type.name)
    $objects.push t.name
  end
}

$int_scalars = {}
$all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && OMPT_INT_SCALARS.include?(t.type.name)
    $int_scalars[t.name] = t.type.name
  end
}

def to_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').downcase
end

def to_class_name(name)
#  case name
#  when "CUstreamBatchMemOpType"
#    return "CUStreamBatchMemOpType"
# when "CUstreamBatchMemOpParams"
#    return "CUStreamBatchMemOpParams"
#  end
#
  mod = to_name_space(name)
  mod = "" unless mod
  n = name.gsub(/_t\z/, "").gsub(/\A#{mod}/, "").split("_").collect { |s|
    s[0] = s[0].capitalize if s.length > 0
    s
  }.join
# mod << n.gsub("Uuid","UUID").gsub("Ipc", "IPC").gsub("P2p", "P2P")
end

def to_scoped_class_name(name)
  "OMPT::#{to_class_name(name)}"
end

def to_ffi_name(name)
  case name
  when nil
    return ":anonymous"
  when "unsigned int"
    return ":uint"
  when "unsigned short"
    return ":ushort"
  when "unsigned char"
    return ":uchar"
  when "unsigned long long int"
    return ":uint64"
  when "size_t"
    return ":size_t"
#  when "cuuint64_t"
#    return ":cuuint64_t"
#  when "cuuint32_t"
#    return ":cuuint32_t"
  end

  n = to_class_name(name)
  mod = to_name_space(name)
  if mod
    n = n.gsub(/\A#{mod}/, "")
    mod << "_"
    mod.downcase!
  else
    mod = ""
  end
  n = to_snake_case(n).gsub(/\A_+/, "")
  mod << n
  mod.to_sym.inspect
end

def to_name_space(name)
  case name
  when /\AOMPT/
    "OMPT"
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
              "(Class::new(FFI::OMPTStruct) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
            when Union
              membs = m.type.to_ffi
              "(Class::new(FFI::OMPTUnion) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
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
              "(Class::new(FFI::OMPTStruct) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
            when Union
              membs = m.type.to_ffi
              "(Class::new(FFI::OMPTUnion) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
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
