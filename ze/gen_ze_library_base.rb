require_relative 'ze_model'
require_relative 'gen_probe_base.rb'

$all_types = $ze_api["typedefs"] + $zet_api["typedefs"] + $zes_api["typedefs"] + $zel_api["typedefs"]
$all_structs = $ze_api["structs"] + $zet_api["structs"] + $zes_api["structs"] + $zel_api["structs"]
$all_unions = $zet_api["unions"]
$all_enums = $ze_api["enums"] + $zet_api["enums"] + $zes_api["enums"] + $zel_api["enums"]
$all_funcs = $ze_api["functions"] + $zet_api["functions"] + $zes_api["functions"] + $zel_api["functions"]
$all_types_map = $all_types.collect { |t| [t.name, t.type] }.to_h

$all_enum_names = []
$all_bitfield_names = []
$all_struct_names = []
$all_union_names = []

$objects = $all_types.select { |t|
  t.type.kind_of?(YAMLCAst::Pointer) &&
  t.type.type.kind_of?(YAMLCAst::Struct)
}.collect { |t| t.name }

$all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && ZE_OBJECTS.include?(t.type.name)
    $objects.push t.name
  end
}

$int_scalars = {}
$all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && ZE_INT_SCALARS.include?(t.type.name)
    $int_scalars[t.name] = t.type.name
  end
}

def to_class_name(name)
  mod = to_name_space(name)
  n = name.gsub(/_t\z/, "").gsub(/\Aze[stl]?_/, "").split("_").collect(&:capitalize).join
  mod << n.gsub("Uuid","UUID").gsub("Dditable", "DDITable").gsub(/\AFp/, "FP").gsub("P2p", "P2P")
end

def to_ffi_name(name)
  name.to_sym.inspect
end

def to_name_space(name)
  name.match(/\A(ze[stl]?)_/)[1].upcase
end

$all_types.each { |t|
  if t.type.kind_of? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    if enum.members.find { |m| m.val && m.val.match("ZE_BIT") }
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

$all_bitfield_names += $all_bitfield_names.select { |n| n.end_with?("_flag_t") }.map { |n| n.gsub("_flag_t", "_flags_t") }

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
          to_ffi_name(m.type.name)
        end
        res.push [to_ffi_name(m.name), mt]
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
          to_ffi_name(m.type.name)
        end
        res.push [to_ffi_name(m.name), mt]
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
      if type.respond_to?(:name)
        t = to_ffi_name(type.name)
      elsif type.kind_of?(Pointer)
        t = ":pointer"
      else
        raise "unknown return type: #{type}"
      end
      p = if params
        params.collect { |par|
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
      else
        []
      end
      [t, p]
    end
  end
end
