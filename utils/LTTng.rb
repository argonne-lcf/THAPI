MEMBER_SEPARATOR = '__'

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
      @name = n.gsub('->', MEMBER_SEPARATOR)
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

  def self.print_tracepoint(namespace, tp, dir = nil)
    puts <<~EOF
      TRACEPOINT_EVENT(
        #{namespace},
        #{tp['name']}#{"_#{SUFFIXES[dir]}" if dir},
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
    dir ||= 'fields'
    if tp[dir]
      print '    '
      puts tp[dir].collect { |(f, *args)| "#{f}(#{args.join(', ')})" }.join("\n    ")
    end
    puts <<~EOF
        )
      )

    EOF
  end
end
