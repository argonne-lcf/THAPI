require_relative 'itt_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

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

# to_class_name already carries the ITT:: namespace, and the library declares
# its classes inside `module ITT`, where a leading ITT:: resolves to that same
# module. Prefixing again produced ITT::ITT::Foo, which no constant matches.
def to_scoped_class_name(name)
  to_class_name(name)
end

def to_name_space(name)
  match_name_space(name, /\A(__itt[dt]?)_/, strict: true).upcase
end

FFI_STRUCT = 'FFI::ITTStruct'
FFI_UNION = 'FFI::ITTUnion'
