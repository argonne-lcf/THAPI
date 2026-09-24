require_relative '../../utils/LTTng'

# Opencl's model is YAML-intermediate-driven (see gen_opencl_model.rb), so its
# tracepoint fields travel as raw [macro, *args] tuples rather than as
# utils/LTTng.rb's TracepointField objects.
module LTTngFieldTuple
  # Which slot holds what is already declared once, per macro, by
  # LTTng::TracepointField::FIELDS. Read the position from there rather than
  # re-encoding it here: a macro with no `type` slot, such as a blob, would
  # otherwise shift every field after it.
  def self.slot(args, key)
    i = LTTng::TracepointField::FIELDS.fetch(args[0].to_sym).index(key)
    i && args[i + 1]
  end

  def self.name(*args)
    slot(args, :name)
  end

  def self.expression(*args)
    slot(args, :expression)
  end

  def self.array?(*args)
    args[0].match('array') || args[0].match('sequence') || args[0].match('blob')
  end

  def self.string?(*args)
    args[0].match('string')
  end

  def self.enum?(*args)
    args[0].match('enum')
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
  res['enum_type'] = LTTngFieldTuple.slot(field, :enum_name) if LTTngFieldTuple.enum?(*field)
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
