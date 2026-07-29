require_relative '../../utils/LTTng'

# Opencl's model is YAML-intermediate-driven (see gen_opencl_model.rb), so its
# tracepoint fields travel as raw [macro, *args] tuples rather than as
# utils/LTTng.rb's TracepointField objects. LTTngFieldTuple provides the same
# kind of accessors (name/array?/string?/enum?/expression) over that tuple
# representation.
module LTTngFieldTuple
  def self.name(*args)
    case args[0]
    when 'ctf_string'
      args[1]
    when 'ctf_enum'
      args[4]
    else
      args[2]
    end
  end

  def self.array?(*args)
    args[0].match('array') || args[0].match('sequence')
  end

  def self.string?(*args)
    args[0].match('string')
  end

  def self.enum?(*args)
    args[0].match('enum')
  end

  def self.expression(*args)
    case args[0]
    when 'ctf_string'
      args[2]
    when 'ctf_enum'
      args[5]
    else
      args[3]
    end
  end
end

def get_field(args, field)
  res = {}
  name = LTTngFieldTuple.name(*field)
  if name.match(/_val\z/)
    pname = name.gsub(/_val\z/, '')
    type = args[pname]
  else
    type = args[name]
    unless type
      pname = LTTngFieldTuple.expression(*field)
      type = args[pname]
    end
  end
  pointer = false
  if type.match(/\*\z/)
    type = type.gsub(/\*\z/, '').strip
    pointer = true
  end
  res['type'] = type
  res['pointer'] = pointer if pointer
  if LTTngFieldTuple.array?(*field)
    res['array'] = true
    res.delete('pointer')
  end
  if LTTngFieldTuple.string?(*field)
    res['string'] = true
    res.delete('pointer')
  end
  res['enum_type'] = field[2] if LTTngFieldTuple.enum?(*field)
  res['lttng'] = field[0]
  [name, res]
end

def get_fields(args, fields)
  return {} unless fields

  args_h = args.collect { |a| a.reverse }.to_h
  fields.collect do |field|
    get_field(args_h, field)
  end.to_h
end
