require_relative 'ze_model'
require_relative 'gen_probe_base.rb'

$all_types = $ze_api["typedefs"]# + $zet_api["typedefs"]
$all_structs = $ze_api["structs"]# + $zet_api["structs"]
$all_enums = $ze_api["enums"]# + $zet_api["enums"]

$all_enum_names = []
$all_bitfield_names = []
$all_struct_names = []

$objects = $all_types.select { |t|
  t.type.kind_of?(YAMLCAst::Pointer) &&
  t.type.type.kind_of?(YAMLCAst::Struct)
}.collect { |t| t.name }

$all_types.each { |t|
  if t.type.kind_of?(YAMLCAst::CustomType) && ZE_OBJECTS.include?(t.type.name)
    $objects.push t.name
  end
}

def to_class_name(name)
  n = name.gsub(/_t\z/, "").gsub(/\Aze_/, "").split("_").collect(&:capitalize).join
  n.gsub("Uuid","UUID").gsub("Dditable", "DDITable").gsub(/\AFp/, "FP").gsub("Ipc", "IPC").gsub("Api","API").gsub("P2p", "P2P")
end

def to_ffi_name(name)
  name.to_sym.inspect
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
  end
}
