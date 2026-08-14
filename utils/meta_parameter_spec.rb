require 'yaml'

# Read a backend's meta-parameter YAML (relative to SRC_DIR) and return the
# spec it describes: a Hash mapping each function name to a list of
# [MetaParameter subclass, args] rows, ready to hand to a Command. Unlisted
# functions read back as [], so callers can `spec[name]` unconditionally.
#
# The file maps each function name to a list of [type, *args] rows under a
# top-level `meta_parameters` key. A backend with none of its own ships no
# file and builds its commands without a spec, so that key must be a mapping:
# a missing, misspelled or empty one means a typo, which would otherwise yield
# an empty spec and look identical to having none.
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
    content = YAML.load_file(path)
    entries = content['meta_parameters']
    raise "#{path} has no 'meta_parameters' mapping" unless entries.is_a?(Hash)

    rows = entries.transform_values do |list|
      list.collect { |type, *args| [Kernel.const_get(type), args] }
    end
    spec.merge!(rows) { |func, _, _| raise "#{func} is declared twice, second time in #{path}" }
  end
  spec
end

# Raise unless every function the spec names is one of `commands`. A spec is
# written by hand against an API that keeps moving, so a key matching nothing
# is a typo or a function that has since been dropped -- either way its rows
# would apply to no command at all, silently.
def check_meta_parameters(spec, commands)
  unknown = spec.keys - commands.collect(&:name)
  raise "Unknown method#{'s' if unknown.size > 1}: #{unknown.join(', ')}!" unless unknown.empty?
end
