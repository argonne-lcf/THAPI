MEMBER_SEPARATOR = '__'

# The suffix each half of a traced call is spelled with, everywhere: in the
# tracepoint the provider declares, in the event class the babeltrace model
# names, and in the trace itself. It is the wire format the two sides agree on,
# so it is stated here rather than per backend.
START = 'entry'
STOP = 'exit'

# The tracepoint macro takes a fixed number of arguments, one of which LTTng
# spends itself; a function with more parameters than the rest can carry has no
# tracepoint generated for it.
LTTNG_AVAILABLE_PARAMS = 25
LTTNG_USABLE_PARAMS = LTTNG_AVAILABLE_PARAMS - 1

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

# How a camelCase C name splits into words: a run of capitals starts a new one.
# cuMemAllocHost -> cu_Mem_Alloc_Host.
def snake_case_parts(str)
  str.gsub(/([A-Z][A-Z0-9]*)/, '_\1')
end

def upper_snake_case(str)
  snake_case_parts(str).upcase
end

def lower_snake_case(str)
  snake_case_parts(str).downcase
end

module LTTng
  # Indented to match the macro the heredocs above and below it spell.
  def self.indented(items, separator: '')
    items.join("#{separator}\n    ").prepend('    ')
  end

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
      lttng_ust_field_fixed_length_blob: %i[name expression length media_type],
      lttng_ust_field_variable_length_blob: %i[name expression length_type length media_type],
    }
    # IANA media type for arbitrary binary struct/buffer data recorded as a blob.
    DEFAULT_MEDIA_TYPE = 'application/octet-stream'.freeze
    attr_accessor :macro, :expression, :type, :provider_name, :enum_name, :length, :length_type, :cast, :media_type
    attr_reader :name

    # Rewrite a positional uint8_t text sequence/array (raw bytes recorded as
    # "text") into the equivalent 2.16 blob macro. Genuine char text is left
    # alone. Shape in:  [ctf_sequence_text, uint8_t, name, expr, len_type, len]
    #             or:    [ctf_array_text,    uint8_t, name, expr, len]
    def self.blobify(args)
      return args unless args.length > 1 && %i[ctf_sequence_text ctf_array_text].include?(args[0].to_sym)
      return args unless args[1].to_sym == :uint8_t

      rest = args[2..-1]
      case args[0].to_sym
      when :ctf_sequence_text
        [:lttng_ust_field_variable_length_blob, *rest, DEFAULT_MEDIA_TYPE]
      when :ctf_array_text
        [:lttng_ust_field_fixed_length_blob, *rest, DEFAULT_MEDIA_TYPE]
      end
    end

    def initialize(*args)
      return unless args.length > 0

      args = self.class.blobify(args)
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
      expr = @cast ? "(#{@cast})(#{@expression})" : @expression
      media_type = "\"#{@media_type || DEFAULT_MEDIA_TYPE}\""
      args =
        case @macro
        when :lttng_ust_field_fixed_length_blob
          [@name, expr, @length, media_type]
        when :lttng_ust_field_variable_length_blob
          [@name, expr, @length_type, @length, media_type]
        else
          [@provider_name, @enum_name, @type, @name, expr, @length_type, @length]
        end
      "#{@macro}(#{args.compact.join(', ')})"
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
    puts indented(en['values'].collect { |(f, sy, *args)|
      "#{f}(#{sy.to_s.inspect}, #{args.join(', ')})"
    })
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
    args = tp['args'].collect { |a| a.join(', ') }
    puts args.empty? ? '    void' : indented(args, separator: ',')
    puts <<EOF
  ),
  TP_FIELDS(
EOF
    fields = tp[phase || 'fields'].to_a.collect { |field| TracepointField.new(*field).call_string }
    puts indented(fields) unless fields.empty?
    puts <<~EOF
        )
      )

    EOF
  end
end
