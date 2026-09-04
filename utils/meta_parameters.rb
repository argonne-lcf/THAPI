require_relative 'yaml_ast'
require_relative 'LTTng'

START = 'entry'
STOP = 'exit'
SUFFIXES = { start: START, stop: STOP }
LTTNG_AVAILABLE_PARAMS = 25
LTTNG_USABLE_PARAMS = LTTNG_AVAILABLE_PARAMS - 1

# A meta-parameter's field is the same whichever event carries it; a direction
# says only which events those are. Prepending one is therefore a whole class
# definition.
module In
  def initialize(*args)
    super
    @lttng_in_type = @lttng_type
  end

  def lttng_in_type
    @lttng_in_type
  end
end

module Out
  def initialize(*args)
    super
    @lttng_out_type = @lttng_type
  end

  def lttng_out_type
    @lttng_out_type
  end
end

class MetaParameter
  attr_reader :name, :command, :lttng_type

  # Here rather than in each generator that walks these, so every caller asks
  # the direction question the same way.
  LTTNG_TYPE_BY_DIRECTION = { start: :lttng_in_type, stop: :lttng_out_type, nil => :lttng_type }.freeze

  def lttng_type_for(dir)
    send(LTTNG_TYPE_BY_DIRECTION.fetch(dir))
  end

  def initialize(command, name)
    @command = command
    @name = name
  end

  def lttng_in_type
    nil
  end

  def lttng_out_type
    nil
  end

  def check_for_null(expr, incl = true)
    MemberPath.prefixes(expr, incl: incl)
  end

  def sanitize_expression(expr, checks = check_for_null(expr, false), default = 0)
    if checks.empty?
      expr
    else
      "(#{checks.join(' && ')} ? #{expr} : #{default})"
    end
  end
end

class StringMetaParameter < MetaParameter
  def initialize(command, name, size = nil)
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a

    t = a.type
    raise "Type is not a pointer: #{t}!" unless t.is_a?(YAMLCAst::Pointer)

    ev = LTTng::TracepointField.new
    if size
      s = command[size]
      raise "Invalid parameter: #{size} for #{command.name}!" unless s

      ev.macro = :ctf_sequence_text
      if s.type.is_a?(YAMLCAst::Pointer)
        checks = check_for_null("#{size}") + check_for_null("#{name}")
        sz = sanitize_expression("*#{size}", checks)
        st = "#{s.type.type}"
      else
        checks = check_for_null("#{name}")
        sz = sanitize_expression("#{size}", checks)
        st = "#{s.type}"
      end
      ev.type = 'char'
      ev.length = sz
      ev.length_type = st
    else
      ev.macro = :ctf_string
    end
    ev.name = "#{name}_val"
    ev.expression = sanitize_expression("#{name}")
    @lttng_type = ev
  end
end

class InString < StringMetaParameter
  prepend In
end

class OutString < StringMetaParameter
  prepend Out
end

class ReturnString < MetaParameter
  prepend Out

  def initialize(command)
    super(command, :result)
    raise 'Command does not return!' unless command.has_return_type?
    raise "Return type is not a pointer: #{command.type}!" unless command.type.is_a?(YAMLCAst::Pointer)

    ev = LTTng::TracepointField.new
    ev.macro = :ctf_string
    ev.name = "#{command.result_name}_val"
    ev.expression = command.result_name
    @lttng_type = ev
  end
end

class OutPtrString < MetaParameter
  prepend Out

  def initialize(command, name)
    super
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a

    t = a.type
    raise "Type is not a pointer: #{t}!" unless t.is_a?(YAMLCAst::Pointer)

    ev = LTTng::TracepointField.new
    ev.macro = :ctf_string
    ev.name = "#{name}_val_val"
    ev.expression = sanitize_expression("*#{name}")
    @lttng_type = ev
  end
end

class ScalarMetaParameter < MetaParameter
  attr_reader :type

  def initialize(command, name, type = nil)
    super(command, name)
    @type = type
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a

    t = a.type
    raise "Type is not a pointer: #{t}!" unless t.is_a?(YAMLCAst::Pointer)

    st = if type
           eval(type)
         else
           t.type
         end
    lttngt = st.lttng_type(command.type_classes)
    lttngt.name = name + '_val'
    if lttngt.macro == :ctf_array_text
      lttngt.macro = :ctf_sequence_text
      lttngt.expression = sanitize_expression("#{name}")
      checks = check_for_null("#{name}")
      lttngt.length = sanitize_expression("#{lttngt.length}", checks)
      lttngt.length_type = 'size_t'
    elsif type
      checks = check_for_null("#{name}")
      lttngt.expression = sanitize_expression("*(#{YAMLCAst::Pointer.new(type: st)})#{name}", checks)
    else
      checks = check_for_null("#{name}")
      lttngt.expression = sanitize_expression("*#{name}", checks)
    end
    @lttng_type = lttngt
  end
end

class InOutScalar < ScalarMetaParameter
  prepend In
  prepend Out
end

class OutScalar < ScalarMetaParameter
  prepend Out
end

class InScalar < ScalarMetaParameter
  prepend In
end

class ArrayMetaParameter < MetaParameter
  attr_reader :size

  def initialize(command, name, size)
    @size = size
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a

    t = a.type
    raise "Type is not a pointer: #{t}!" unless t.is_a?(YAMLCAst::Pointer)

    s = command[size]
    raise "Invalid parameter: #{size} for #{command.name}!" unless s

    if s.type.is_a?(YAMLCAst::Pointer)
      checks = check_for_null("#{size}") + check_for_null("#{name}")
      size = "*#{size}"
      size = "(#{size} < 0 ? 0 : (size_t)#{size})" if INT_SIGN_MAP["#{s.type.type}"]
      sz = sanitize_expression("#{size}", checks)
      st = INT_SIGN_MAP["#{s.type.type}"] ? 'size_t' : "#{s.type.type}"
    else
      checks = check_for_null("#{name}")
      size = "(#{size} < 0 ? 0 : (size_t)#{size})" if INT_SIGN_MAP["#{s.type}"]
      sz = sanitize_expression("#{size}", checks)
      st = INT_SIGN_MAP["#{s.type}"] ? 'size_t' : "#{s.type}"
    end
    tt = if t.type.is_a?(YAMLCAst::Void)
           YAMLCAst::CustomType.new(name: 'uint8_t')
         else
           t.type
         end
    y = YAMLCAst::Array.new(type: tt)
    lttngt = y.lttng_type(command.type_classes, length: sz, length_type: st)
    lttngt.name = name + '_vals'
    lttngt.expression = sanitize_expression("#{name}")
    @lttng_type = lttngt
  end
end

class OutArray < ArrayMetaParameter
  prepend Out
end

class InArray < ArrayMetaParameter
  prepend In
end

# A C local the tracer declares and fills itself, then passes to the tracepoint
# alongside the real arguments -- a declaration like any other, plus `fill`, the
# C that computes it, and `dir`, the tracepoint whose block runs that C.
class TracepointParameter < YAMLCAst::Declaration
  attr_reader :fill, :dir

  def initialize(name:, type:, fill:, dir:)
    super(name: name, type: type)
    @fill = fill
    @dir = dir
  end
end

# A NULL-terminated array carries no count, so its length is walked at run time
# and shipped as a synthesized parameter. An output array can only be walked
# once the call has filled it.
module NullTerminated
  def initialize(command, name)
    sname = "_#{MemberPath.flatten(name)}_size"
    checks = check_for_null(name)
    fill = <<EOF
  #{sname} = 0;
  if(#{checks.join(' && ')}) {
    while(#{name}[#{sname}] != 0) {
      #{sname} += 2;
    }
    #{sname} ++;
  }
EOF
    command.tracepoint_parameters.push TracepointParameter.new(
      name: sname, type: YAMLCAst::CustomType.new(name: 'size_t'),
      fill: fill, dir: is_a?(Out) ? :stop : :start
    )
    # Command#[] searches the tracepoint parameters too, so the array below
    # resolves its size to the one just pushed.
    super(command, name, sname)
  end
end

class OutNullArray < OutArray
  prepend NullTerminated
end

class InNullArray < InArray
  prepend NullTerminated
end

class FixedArrayMetaParameter < MetaParameter
  attr_reader :size

  def initialize(command, name, size)
    @size = size
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a

    t = a.type
    raise "Type is not a pointer: #{t}!" unless t.is_a?(YAMLCAst::Pointer)

    check_for_null("#{name}")
    tt = if t.type.is_a?(YAMLCAst::Void)
           YAMLCAst::CustomType.new(name: 'uint8_t')
         else
           t.type
         end
    y = YAMLCAst::Array.new(type: tt)
    lttngt = y.lttng_type(command.type_classes, length: size, length_type: nil)
    lttngt.name = name + '_vals'
    lttngt.expression = sanitize_expression("#{name}")
    @lttng_type = lttngt
  end
end

class InFixedArray < FixedArrayMetaParameter
  prepend In
end

class OutFixedArray < FixedArrayMetaParameter
  prepend Out
end

class ArrayByRefMetaParameter < MetaParameter
  attr_reader :size

  def initialize(command, name, size)
    @size = size
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a

    t = a.type
    raise "Type is not a pointer: #{t}!" unless t.is_a?(YAMLCAst::Pointer)
    raise "Type is not a pointer to an array: #{t}!" unless t.type.is_a?(YAMLCAst::Pointer)

    s = command[size]
    raise "Invalid parameter: #{size} for #{command.name}!" unless s

    if s.type.is_a?(YAMLCAst::Pointer)
      checks = check_for_null("#{size}") + check_for_null("#{name}") + check_for_null("*#{name}")
      sz = sanitize_expression("*#{size}", checks)
      st = "#{s.type.type}"
    else
      checks = check_for_null("#{name}") + check_for_null("*#{name}")
      sz = sanitize_expression("#{size}", checks)
      st = "#{s.type}"
    end
    tt = if t.type.type.is_a?(YAMLCAst::Void)
           YAMLCAst::CustomType.new(name: 'uint8_t')
         else
           t.type.type
         end
    y = YAMLCAst::Array.new(type: tt)
    lttngt = y.lttng_type(command.type_classes, length: sz, length_type: st)
    lttngt.name = name + '_val_vals'
    lttngt.expression = sanitize_expression("*#{name}")
    @lttng_type = lttngt
  end
end

class OutArrayByRef < ArrayByRefMetaParameter
  prepend Out
end

class OutLTTng < MetaParameter
  prepend Out

  def initialize(command, name, *args)
    raise "Invalid parameter: #{name} for #{command.name}!" unless command[name]

    super(command, name)
    @lttng_type = LTTng::TracepointField.new(*args)
  end
end

class InLTTng < MetaParameter
  prepend In

  def initialize(command, name, *args)
    raise "Invalid parameter: #{name} for #{command.name}!" unless command[name]

    super(command, name)
    @lttng_type = LTTng::TracepointField.new(*args)
  end
end
