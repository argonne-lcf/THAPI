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
# The row types are resolved with const_get against the top level: the classes
# in utils/meta_parameters, plus any a backend defines for an API-specific
# layout (opencl's DeviceFissionPropertyList).
def load_meta_parameters(*filenames)
  spec = Hash.new { [] }
  filenames.each do |filename|
    # A generated spec lives in the build directory: pass its absolute path.
    path = File.expand_path(filename, SRC_DIR)
    content = yaml_load_file_cached(path)
    entries = content['meta_parameters']
    raise "#{path} has no 'meta_parameters' mapping" unless entries.is_a?(Hash)

    rows = entries.transform_values do |list|
      # A trailing mapping holds keyword options: [OutScalar, errcode_ret, {nocheck: true}].
      list.collect do |type, *args|
        options = args.last.is_a?(Hash) ? args.pop.transform_keys(&:to_sym) : {}
        [Kernel.const_get(type), args, options]
      end
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
