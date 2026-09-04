MEMBER_SEPARATOR = '__'

# A meta-parameter names either a function parameter or a path to a member of
# one, written as it would be in C: `nodeParams->extra`. Everything that has to
# take such a name apart reads the grammar from here.
module MemberPath
  def self.segments(name)
    name.split('->')
  end

  # Every prefix of the path, outermost first -- the pointers C must find
  # non-NULL before the whole expression can be read. `incl: false` drops the
  # last, for a caller guarding an expression that already reads it.
  def self.prefixes(name, incl: true)
    path = segments(name)
    path = path[0..-2] unless incl
    path.each_index.map { |i| path[0..i].join('->') }
  end

  # A path is one identifier once it names a tracepoint field, which cannot
  # carry an arrow.
  def self.flatten(name)
    name.gsub('->', MEMBER_SEPARATOR)
  end
end

def upper_snake_case(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1').upcase
end

module LTTng
  class TracepointField
    FIELDS = {
      ctf_array: %i[type name expression length],
      ctf_array_hex: %i[type name expression length],
      ctf_array_network: %i[type name expression length],
      ctf_array_network_hex: %i[type name expression length],
      ctf_array_text: %i[type name expression length],
      ctf_enum: %i[provider_name enum_name type name expression],
      ctf_float: %i[type name expression],
      ctf_integer: %i[type name expression],
      ctf_integer_hex: %i[type name expression],
      ctf_integer_network: %i[type name expression],
      ctf_integer_network_hex: %i[type name expression],
      ctf_sequence: %i[type name expression length_type length],
      ctf_sequence_hex: %i[type name expression length_type length],
      ctf_sequence_network: %i[type name expression length_type length],
      ctf_sequence_network_hex: %i[type name expression length_type length],
      ctf_sequence_text: %i[type name expression length_type length],
      ctf_string: %i[name expression],
    }
    attr_accessor :macro, :expression, :type, :provider_name, :enum_name, :length, :length_type, :cast
    attr_reader :name

    def initialize(*args)
      return unless args.length > 0

      desc = FIELDS[args[0].to_sym]
      raise "Invalid field #{args[0]}!" unless desc

      @macro = args[0].to_sym
      raise "Invalid field parameters #{args[1..-1]}!" unless args[1..-1].length == desc.length

      desc.zip(args[1..-1]).each do |sym, v|
        instance_variable_set(:"@#{sym}", v)
      end
      m = @expression.match(/\((.*?)\)(.*)/)
      return unless m

      @cast = m[1]
      @expression = m[2]
    end

    def call_string
      str = "#{@macro}("
      str << [@provider_name, @enum_name, @type, @name, @cast ? "(#{@cast})(#{@expression})" : @expression,
              @length_type, @length].compact.join(', ')
      str << ')'
    end

    def name=(n)
      @name = MemberPath.flatten(n)
    end
  end

  def self.print_enum(namespace, en)
    puts <<~EOF
      TRACEPOINT_ENUM(
        #{namespace},
        #{en['name']},
        TP_ENUM_VALUES(
    EOF
    print '    '
    puts en['values'].collect { |(f, sy, *args)|
      "#{f}(#{sy.to_s.inspect}, #{args.join(', ')})"
    }.join("\n    ")
    puts <<~EOF
        )
      )

    EOF
  end

  def self.print_tracepoint(namespace, tp, phase = nil, suffix: nil)
    puts <<~EOF
      TRACEPOINT_EVENT(
        #{namespace},
        #{tp['name']}#{"_#{suffix}" if suffix},
        TP_ARGS(
    EOF
    print '    '
    args = tp['args']
    if args.empty?
      puts 'void'
    else
      puts args.collect { |a| a.join(', ') }.join(",\n    ")
    end
    puts <<EOF
  ),
  TP_FIELDS(
EOF
    fields = tp[phase || 'fields']
    if fields
      print '    '
      puts fields.collect { |(f, *args)| "#{f}(#{args.join(', ')})" }.join("\n    ")
    end
    puts <<~EOF
        )
      )

    EOF
  end
end
