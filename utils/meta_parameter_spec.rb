require_relative 'yaml_ast'

# Read a backend's meta-parameter YAML (relative to SRC_DIR) and return what it
# declares: `[:meta_parameters]`, the per-function rows, and the two rendering
# sections -- `[:meta_parameters_struct]` for a struct's byte-array members,
# `[:meta_parameters_function]` for a function's byte-array parameters.
# Unlisted functions read back as [], so callers can `spec[name]`
# unconditionally, and a section a file omits reads back as {}.
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
  structs = {}
  functions = {}
  filenames.each do |filename|
    path = File.join(SRC_DIR, filename)
    content = yaml_load_file_cached(path)
    entries = content['meta_parameters']
    raise "#{path} has no 'meta_parameters' mapping" unless entries.is_a?(Hash)

    rows = entries.transform_values do |list|
      list.collect { |type, *args| [Kernel.const_get(type), args] }
    end
    spec.merge!(rows) { |func, _, _| raise "#{func} is declared twice, second time in #{path}" }

    structs.merge!(rendering_rows(content, path, 'meta_parameters_struct')) do |name, _, _|
      raise "#{name} is declared twice, second time in #{path}"
    end
    functions.merge!(rendering_rows(content, path, 'meta_parameters_function')) do |name, _, _|
      raise "#{name} is declared twice, second time in #{path}"
    end
  end
  { meta_parameters: spec, meta_parameters_struct: structs, meta_parameters_function: functions }
end

# One rendering section, as `{ owner => { thing => renderer } }`. A row reads
# `[ renderer, thing ]` and the map is keyed the other way, because every user
# asks "how do I print this member?".
#
# Naming the same thing twice would silently keep one renderer, so it raises.
def rendering_rows(content, path, section)
  content.fetch(section, {}).transform_values do |list|
    twice = list.collect(&:last).tally.select { |_, n| n > 1 }.keys
    raise "#{path}: #{twice.join(', ')} rendered twice" unless twice.empty?

    list.to_h(&:reverse)
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
