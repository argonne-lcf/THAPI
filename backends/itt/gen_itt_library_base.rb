require_relative 'itt_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

$all_types = $itt_api['typedefs']
$all_structs = $itt_api['structs']
$all_unions = $itt_api['unions']
$all_enums = $itt_api['enums']
$all_funcs = $itt_api['functions']

$all_enum_names = []
$all_bitfield_names = []
$all_struct_names = []
$all_union_names = []

$objects = $all_types.select do |t|
  t.type.is_a?(YAMLCAst::Pointer) &&
    t.type.type.is_a?(YAMLCAst::Struct)
end.collect { |t| t.name }

$all_types.each do |t|
  $objects.push t.name if t.type.is_a?(YAMLCAst::CustomType) && OBJECT_TYPES.include?(t.type.name)
end

$int_scalars = {}
$all_types.each do |t|
  $int_scalars[t.name] = t.type.name if t.type.is_a?(YAMLCAst::CustomType) && INT_TYPES.include?(t.type.name)
end

def to_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').downcase
end

# Convert C / ITT names (e.g. "__itt_domain_t") to Ruby CamelCase class names
def to_class_name(name)
  # Derive namespace (e.g. "ITT::" or "")
  mod = to_name_space(name)
  mod = mod.to_s
  mod = mod.gsub(/\A_+/, '') # drop leading underscores
  mod << '::' unless mod.empty? || mod.end_with?('::')

  base = name.to_s

  # Drop common suffixes and prefixes
  base = base.sub(/_t\z/i, '')               # remove _t or _T
  base = base.sub(/\A_+/, '')                # remove leading underscores
  base = base.sub(/\A__?itt[dt]?_/i, '')     # remove _itt_ /__itt_ / __ittd_ / __ittt_ (case-insensitive)

  # Convert snake_case or camelCase into CamelCase
  if base.include?('_')
    base = base.split('_').map { |s| s.capitalize }.join
  elsif base[0] =~ /[a-z]/
    base = base[0].upcase + base[1..]
  end

  # Ensure the result starts with a capital letter
  base = "ITT#{base}" unless base =~ /\A[A-Z]/

  mod + base
end

def to_scoped_class_name(name)
  "ITT::#{to_class_name(name)}"
end

def to_name_space(name)
  name.match(/\A(__itt[dt]?)_/)[1].upcase
end

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    enum = $all_enums.find { |e| t.type.name == e.name }
    # Handle anonymous enum, and typedef enum
    if enum&.name&.end_with?('flag_t')
      $all_bitfield_names.push t.name
    else
      $all_enum_names.push t.name
    end
  elsif t.type.is_a? YAMLCAst::Struct
    $all_struct_names.push t.name
  elsif t.type.is_a? YAMLCAst::Union
    $all_union_names.push t.name
  end
end

$all_bitfield_names += $all_bitfield_names.select do |n|
  n.end_with?('_flag_t')
end.map { |n| n.gsub('_flag_t', '_flags_t') }

FFI_STRUCT = 'FFI::ITTStruct'
FFI_UNION = 'FFI::ITTUnion'
