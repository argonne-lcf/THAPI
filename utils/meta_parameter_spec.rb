require_relative 'yaml_ast'

# Read a backend's meta-parameter YAML (relative to SRC_DIR) and return the
# spec it describes. Unlisted functions read back as [], so callers can
# `spec[name]` unconditionally.
#
# A backend with no meta-parameters of its own ships no file at all, so a file
# that exists must have the `meta_parameters` key: a missing or misspelled one
# would otherwise yield an empty spec and look identical to having none.
#
# Pass several filenames to merge their specs (ze splits its rows per
# namespace, cuda across its two APIs). Each function must be declared in at
# most one of them: listing it twice would silently concatenate both sets of
# rows, so it raises instead.
#
# The row types are resolved with const_get against the top level, so each
# backend gets its own: the AST backends the classes in utils/meta_parameters,
# opencl the ones it defines itself. That is what lets this be shared, and why
# it lives here rather than next to either set.
def load_meta_parameters(*filenames)
  spec = Hash.new { [] }
  filenames.each do |filename|
    path = File.join(SRC_DIR, filename)
    content = yaml_load_file_cached(path)
    entries = content['meta_parameters']
    raise "#{path} has no 'meta_parameters' mapping" unless entries.is_a?(Hash)

    rows = entries.transform_values do |list|
      list.collect { |type, *args| [Kernel.const_get(type), args] }
    end
    spec.merge!(rows) { |func, _, _| raise "#{func} is declared twice, second time in #{path}" }
  end
  spec
end

# How a struct's byte-array members should be read, from the same YAML the
# meta-parameters come from and shaped like them -- one entry per struct, whose
# rows are `[ <renderer>, <member> ]`:
#
#   meta_parameters_struct:
#     ze_kernel_uuid_t:
#       - [ UuidReversed, kid ]
#       - [ UuidReversed, mid ]
#
# A byte array cannot say from its shape what it holds: an opaque blob, a UUID
# and a fixed-width string are all `uint8_t x[N]` -- and `char x[N]` is all
# three too, so not even the element type separates them. The header's answer
# is written down here instead of guessed from the member's name.
#
# Rows are per MEMBER because most byte arrays share a struct with other
# members: zes_device_properties_t is six strings among ten fields, and
# ze_kernel_uuid_t is two UUIDs side by side. A per-struct answer could not
# describe either.
#
# Reads back {} for a file that declares none, so callers can ask
# unconditionally. Merged across filenames like the rows above.
def load_meta_parameters_struct(*filenames)
  filenames.each_with_object({}) do |filename, spec|
    path = File.join(SRC_DIR, filename)
    entries = yaml_load_file_cached(path).fetch('meta_parameters_struct', {})
    rows = entries.transform_values do |list|
      list.each_with_object({}) do |(renderer, member), members|
        raise "#{path}: #{member} is rendered twice" if members.key?(member)

        members[member] = renderer
      end
    end
    spec.merge!(rows) { |name, _, _| raise "#{name} is declared twice, second time in #{path}" }
  end
end

# Raise unless every function the spec names is one of `commands`. A spec is
# written by hand against an API that keeps moving, so a key matching nothing
# is a typo or a function that has since been dropped -- either way its rows
# would apply to no command at all, silently.
def check_meta_parameters(spec, commands)
  unknown = spec.keys - commands.collect(&:name)
  raise "Unknown method#{'s' if unknown.size > 1}: #{unknown.join(', ')}!" unless unknown.empty?
end
