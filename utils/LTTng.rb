MEMBER_SEPARATOR = "__"

module LTTng
  class TracepointEvent
    attr_accessor :macro
    attr_accessor :name
    attr_accessor :expression
    attr_accessor :type
    attr_accessor :provider_name
    attr_accessor :enum_name
    attr_accessor :length
    attr_accessor :length_type
    attr_accessor :cast

    def call_string
      str = "#{macro}("
      str << [ @provider_name, @enum_name, @type, @name, @cast ? "(#{@cast})(#{@expression})" : @expression, @length_type, @length ].compact.join(", ")
      str << ")"
    end

    def name=(n)
      @name = n.gsub("->", MEMBER_SEPARATOR)
    end
  end

  def self.print_enum(namespace, en)
    puts <<EOF
TRACEPOINT_ENUM(
  #{namespace},
  #{en["name"]},
  TP_ENUM_VALUES(
EOF
    print "    "
    puts en["values"].collect { |(f, sy, *args)|
      "#{f}(#{sy.to_s.inspect}, #{args.join(", ")})"
    }.join("\n    ")
    puts <<EOF
  )
)

EOF
  end

  def self.print_tracepoint(namespace, tp, dir = nil)
    puts <<EOF
TRACEPOINT_EVENT(
  #{namespace},
  #{tp["name"]}#{dir ? "_#{dir}" : ""},
  TP_ARGS(
EOF
    print "    "
    args = tp["args"]
    if args.empty?
      puts "void"
    else
      puts args.collect { |a| a.join(", ") }.join(",\n    ")
    end
    puts <<EOF
  ),
  TP_FIELDS(
EOF
    dir = "fields" unless dir
    if tp[dir]
      print "    "
      puts tp[dir].collect { |(f, *args)| "#{f}(#{args.join(", ")})" }.join("\n    ")
    end
    puts <<EOF
  )
)

EOF
  end

end


