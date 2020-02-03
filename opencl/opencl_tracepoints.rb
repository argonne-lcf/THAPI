class LTTng
  def self.name(*args)
    case args[0]
    when "ctf_string"
      args[1]
    when "ctf_enum"
      args[4]
    else
      args[2]
    end
  end

  def self.array?(*args)
    args[0].match("array") || args[0].match("sequence")
  end

  def self.string?(*args)
    args[0].match("string")
  end

  def self.expression(*args)
    case args[0]
    when "ctf_string"
      args[2]
    when "ctf_enum"
      args[5]
    else
      args[3]
    end
  end

end

def print_enum(namespace, en)
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

def print_tracepoint(namespace, tp, dir = nil)
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

def get_field(args, field)
  res = {}
  name = LTTng::name(*field)
  if name.match(/_val\z/)
    pname = name.gsub(/_val\z/, "")
    type = args[pname]
  else
    type = args[name]
    unless type
      pname = LTTng.expression(*field)
      type = args[pname]
    end
  end
  pointer = false
  if type.match(/\*\z/)
    type = type.gsub(/\*\z/, "").strip
    pointer = true
  end
  res["type"] = type
  res["pointer"] = pointer if pointer
  res["array"] = true if LTTng.array?(*field)
  res["string"] = true if LTTng.string?(*field)
  res["lttng"] = field[0]
  [ name, res ]
end

def get_fields(args, fields)
  return {} unless fields
  args_h = args.collect { |a| a.reverse }.to_h
  fields.collect { |field|
    get_field(args_h, field)
  }.to_h
end
