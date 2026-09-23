require_relative 'yaml_ast'

# Read a backend's meta-parameter YAML (relative to SRC_DIR) and return the
# spec it describes, keyed by section. The sections hold the same kind of fact
# and differ in what a row keys on: a function's parameter, a struct's member,
# a function's byte-array parameter. Unlisted functions read back as [], so
# callers can `spec[name]` unconditionally, and an absent section as {}.
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
  spec = { meta_parameters: Hash.new { [] }, meta_parameters_struct: {}, meta_parameters_function: {} }
  filenames.each do |filename|
    path = File.join(SRC_DIR, filename)
    declared_sections(yaml_load_file_cached(path), path).each do |section, rows|
      spec[section].merge!(rows) do |name, _, _|
        raise "#{name} is declared twice, second time in #{path}"
      end
    end
  end
  spec
end

def declared_sections(content, path)
  entries = content['meta_parameters']
  raise "#{path} has no 'meta_parameters' mapping" unless entries.is_a?(Hash)

  rows = entries.transform_values do |list|
    list.collect { |type, *args| [Kernel.const_get(type), args] }
  end
  { meta_parameters: rows,
    meta_parameters_struct: indexed_rows(content, path, 'meta_parameters_struct'),
    meta_parameters_function: indexed_rows(content, path, 'meta_parameters_function') }
end

# A section whose rows read `[ value, key ]`, indexed by key: these sections
# write the renderer first, and every reader looks up the member.
# Naming one key twice would silently keep a single value, so it raises.
def indexed_rows(content, path, section)
  content.fetch(section, {}).transform_values do |list|
    twice = list.collect(&:last).tally.select { |_, n| n > 1 }.keys
    raise "#{path}: #{twice.join(', ')} declared twice" unless twice.empty?

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
