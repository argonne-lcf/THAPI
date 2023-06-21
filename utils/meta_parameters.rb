START = "entry"
STOP = "exit"
SUFFIXES = { :start => START, :stop => STOP }
LTTNG_AVAILABLE_PARAMS = 25
LTTNG_USABLE_PARAMS = LTTNG_AVAILABLE_PARAMS - 1

module In
  def lttng_in_type
    @lttng_in_type
  end
end

module Out
  def lttng_out_type
    @lttng_out_type
  end
end

class MetaParameter
  attr_reader :name
  attr_reader :command
  attr_reader :lttng_type
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
    list = expr.split("->")
    if list.length == 1
      if incl
        return [expr]
      else
        return []
      end
    else
      res = []
      pre = ""
      list[0..(incl ? -1 : -2)].each { |n|
        pre += n
        res.push(pre)
        pre += "->"
      }
      return res
    end
  end

  def sanitize_expression(expr, checks = check_for_null(expr, false), default: 0)
    if checks.empty?
      expr
    else
      "(#{checks.join(" && ")} ? #{expr} : #{default})"
    end
  end
end

class StringMetaParameter < MetaParameter
  def initialize(command, name, size = nil)
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    ev = LTTng::TracepointField::new
    if size
      s = command[size]
      raise "Invalid parameter: #{size} for #{command.name}!" unless s
      ev.macro = :ctf_sequence_text
      if s.type.kind_of?(YAMLCAst::Pointer)
        checks = check_for_null("#{size}") + check_for_null("#{name}")
        sz = sanitize_expression("*#{size}", checks)
        st = "#{s.type.type}"
      else
        checks = check_for_null("#{name}")
        sz = sanitize_expression("#{size}", checks)
        st = "#{s.type}"
      end
      ev.type = "char"
      ev.length = sz
      ev.length_type = st
    else
      ev.macro = :ctf_string
    end
    ev.name = "#{name}_val"
    ev.expression = sanitize_expression("#{name}", default: 'NULL')
    @lttng_type = ev
  end
end

class InString < StringMetaParameter
  prepend In
  def initialize(command, name, size = nil)
    super
    @lttng_in_type = @lttng_type
  end
end

class OutString < StringMetaParameter
  prepend Out
  def initialize(command, name, size = nil)
    super
    @lttng_out_type = @lttng_type
  end
end

class ReturnString < MetaParameter
  prepend Out
  def initialize(command)
    super(command, :result)
    raise "Command does not return!" unless command.has_return_type?
    raise "Return type is not a pointer: #{command.type}!" unless command.type.kind_of?(YAMLCAst::Pointer)
    ev = LTTng::TracepointField::new
    ev.macro = :ctf_string
    ev.name = "#{RESULT_NAME}_val"
    ev.expression = "#{RESULT_NAME}"
    @lttng_out_type = ev
  end
end

class OutPtrString < MetaParameter
  prepend Out
  def initialize(command, name)
    super
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    ev = LTTng::TracepointField::new
    ev.macro = :ctf_string
    ev.name = "#{name}_val_val"
    ev.expression = sanitize_expression("*#{name}")
    @lttng_out_type = ev
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
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    if type
      st = eval(type)
    else
      st = t.type
    end
    lttngt = st.lttng_type
    lttngt.name = name + "_val"
    if lttngt.macro == :ctf_sequence_text
      lttngt.expression = sanitize_expression("#{name}", default: 'NULL')
      checks = check_for_null("#{name}")
      lttngt.length = sanitize_expression("#{lttngt.length}", checks)
      lttngt.length_type = "size_t"
    elsif type
      checks = check_for_null("#{name}")
      lttngt.expression = sanitize_expression("*(#{YAMLCAst::Pointer::new(type: st)})#{name}", checks)
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
  def initialize(command, name, type = nil)
    super
    @lttng_out_type = @lttng_in_type = @lttng_type
  end
end

class OutScalar < ScalarMetaParameter
  prepend Out
  def initialize(command, name, type = nil)
    super
    @lttng_out_type = @lttng_type
  end
end

class InScalar < ScalarMetaParameter
  prepend In
  def initialize(command, name, type = nil)
    super
    @lttng_in_type = @lttng_type
  end
end

class ArrayMetaParameter < MetaParameter
  attr_reader :size

  def initialize(command, name, size)
    @size = size
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    s = command[size]
    raise "Invalid parameter: #{size} for #{command.name}!" unless s
    if s.type.kind_of?(YAMLCAst::Pointer)
      checks = check_for_null("#{size}") + check_for_null("#{name}")
      size = "*#{size}"
      size = "(#{size} < 0 ? 0 : (size_t)#{size})" if INT_SIGN_MAP["#{s.type.type}"]
      sz = sanitize_expression("#{size}", checks)
      st = INT_SIGN_MAP["#{s.type.type}"] ? "size_t"  : "#{s.type.type}"
    else
      checks = check_for_null("#{name}")
      size = "(#{size} < 0 ? 0 : (size_t)#{size})" if INT_SIGN_MAP["#{s.type}"]
      sz = sanitize_expression("#{size}", checks)
      st = INT_SIGN_MAP["#{s.type}"] ? "size_t" : "#{s.type}"
    end
    if t.type.kind_of?(YAMLCAst::Void)
      tt = YAMLCAst::CustomType::new(name: "uint8_t")
    else
      tt = t.type
    end
    y = YAMLCAst::Array::new(type: tt)
    lttngt = y.lttng_type(length: sz, length_type: st)
    lttngt.name = name + "_vals"
    lttngt.expression = sanitize_expression("#{name}", default: 'NULL')
    @lttng_type = lttngt
  end
end

class OutArray < ArrayMetaParameter
  prepend Out
  def initialize(command, name, size)
    super
    @lttng_out_type = @lttng_type
  end
end

class InArray < ArrayMetaParameter
  prepend In
  def initialize(command, name, size)
    super
    @lttng_in_type = @lttng_type
  end
end

class FixedArrayMetaParameter < MetaParameter
  attr_reader :size

  def initialize(command, name, size)
    @size = size
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" if !t.kind_of?(YAMLCAst::Pointer)
    checks = check_for_null("#{name}")
    if t.type.kind_of?(YAMLCAst::Void)
      tt = YAMLCAst::CustomType::new(name: "uint8_t")
    else
      tt = t.type
    end
    y = YAMLCAst::Array::new(type: tt)
    lttngt = y.lttng_type(length: sanitize_expression("#{size}", checks), length_type: nil)
    lttngt.name = name + "_vals"
    lttngt.expression = sanitize_expression("#{name}")
    @lttng_type = lttngt
  end
end

class InFixedArray < FixedArrayMetaParameter
  prepend In
  def initialize(command, name, size)
    super
    @lttng_in_type = @lttng_type
  end
end

class OutFixedArray < FixedArrayMetaParameter
  prepend Out
  def initialize(command, name, size)
    super
    @lttng_out_type = @lttng_type
  end
end

class ArrayByRefMetaParameter < MetaParameter
  attr_reader :size

  def initialize(command, name, size)
    @size = size
    super(command, name)
    a = command[name]
    raise "Invalid parameter: #{name} for #{command.name}!" unless a
    t = a.type
    raise "Type is not a pointer: #{t}!" unless t.kind_of?(YAMLCAst::Pointer)
    raise "Type is not a pointer to an array: #{t}!" if !t.type.kind_of?(YAMLCAst::Pointer)
    s = command[size]
    raise "Invalid parameter: #{size} for #{command.name}!" unless s
    if s.type.kind_of?(YAMLCAst::Pointer)
      checks = check_for_null("#{size}") + check_for_null("#{name}") + check_for_null("*#{name}")
      sz = sanitize_expression("*#{size}", checks)
      st = "#{s.type.type}"
    else
      checks = check_for_null("#{name}") + check_for_null("*#{name}")
      sz = sanitize_expression("#{size}", checks)
      st = "#{s.type}"
    end
    if t.type.type.kind_of?(YAMLCAst::Void)
      tt = YAMLCAst::CustomType::new(name: "uint8_t")
    else
      tt = t.type.type
    end
    y = YAMLCAst::Array::new(type: tt)
    lttngt = y.lttng_type(length: sz, length_type: st)
    lttngt.name = name + "_val_vals"
    lttngt.expression = sanitize_expression("*#{name}", default: 'NULL')
    @lttng_type = lttngt
  end
end

class OutArrayByRef < ArrayByRefMetaParameter
  prepend Out
  def initialize(command, name, size)
    super
    @lttng_out_type = @lttng_type
  end
end

class OutLTTng < MetaParameter
  prepend Out
  def initialize(command, name, *args)
    raise "Invalid parameter: #{name} for #{command.name}!" unless command[name]
    super(command, name)
    @lttng_out_type = LTTng::TracepointField::new(*args)
  end
end

class InLTTng < MetaParameter
  prepend In
  def initialize(command, name, *args)
    raise "Invalid parameter: #{name} for #{command.name}!" unless command[name]
    super(command, name)
    @lttng_in_type = LTTng::TracepointField::new(*args)
  end
end


