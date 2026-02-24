require_relative 'gen_itt_library_base'

def unwrap_typedef_to_concrete(t)
  # unwrap chains like Typedef -> Typedef -> Enum
  seen = {}.compare_by_identity
  while t.respond_to?(:type) && !seen[t]
    seen[t] = true
    t = t.type
  end
  t
end

def enum_with_members_or_nil(e)
  e if e && e.respond_to?(:members) && e.members && !e.members.empty?
end

def find_enum_by_name(name, api)
  base = name.to_s
  base_stripped = base.sub(/_t\z/, '')

  enums = Array(api['enums'])
  typedefs = Array(api['typedefs'])

  # direct match (array or hash)
  if api['enums'].is_a?(Hash)
    e = api['enums'][base] || api['enums'][base_stripped]
    return e if enum_with_members_or_nil(e)
  else
    e = enums.find { |x| x.respond_to?(:name) && [base, base_stripped].include?(x.name) }
    return e if enum_with_members_or_nil(e)
  end

  # typedef: name or stripped name
  td = typedefs.find { |t| t.respond_to?(:name) && [base, base_stripped].include?(t.name) }
  if td
    e = unwrap_typedef_to_concrete(td)
    return e if enum_with_members_or_nil(e)
  end

  # forward-declared enum object; look for another enum with same name but members set
  if e && e.respond_to?(:name)
    alt = enums.find { |x| x != e && x.respond_to?(:name) && x.name == e.name && enum_with_members_or_nil(x) }
    return alt if alt
  end

  # look for any anonymous enum that has members
  if td
    anon = unwrap_typedef_to_concrete(td)
    return anon if enum_with_members_or_nil(anon)
  end

  nil
end

def print_enum(name, enum)
  by_name = {}
  current = -1

  resolve_alias = lambda do |ref|
    by_name.fetch(ref.to_sym) { raise "enum #{name}: alias #{ref} not defined yet" }
  end

  parse_string = lambda do |s|
    s = s.strip
    return resolve_alias.call(s[1..]) if s.start_with?(':')               # Ruby-style symbol alias
    return resolve_alias.call(s)      if /\A[_A-Za-z]\w*\z/.match?(s)     # C-style ident alias

    # Last resort: numeric/expr
    Integer(s)
  rescue ArgumentError
    eval(s)
  end

  resolved = enum.members.map do |m|
    n = m.name.to_sym
    v = m.val

    current =
      case v
      when nil       then current + 1
      when Integer   then v
      when Symbol    then resolve_alias.call(v)
      when String    then parse_string.call(v)
      else
        raise "enum #{name}: unsupported val #{v.inspect} for #{n}"
      end

    by_name[n] = current
    [n, current]
  end

  pair = lambda { |sym_val|
    sym, val = sym_val
    "#{sym.inspect}, #{val}"
  }

  puts <<~RUBY
    #{to_class_name(name)} = ittenum #{to_ffi_name(name)},
      [ #{resolved.map(&pair).join(",\n        ")} ]
  RUBY
end

def print_itt_object(object)
  print_object(object)
end

print_ffi_module(:ITT)

puts <<~EOF
  module ITT
    extend FFI::Library

EOF

def print_union(name, union)
  print_union_with_namespace(:ITT, name, union)
end

def print_struct(name, struct)
  members = struct.to_ffi

  # Replace function pointer field types with :pointer.
  # This avoids unresolved type errors when callbacks are defined later.
  if defined?($fnptr_syms) && $fnptr_syms
    members = members.map do |m|
      t = m[1]
      if t.is_a?(Array)
        elem_t = t[0]
        [m[0], [($fnptr_syms.include?(elem_t.to_s) ? :pointer : elem_t), t[1]]]
      else
        [m[0], ($fnptr_syms.include?(t.to_s) ? :pointer : t)]
      end
    end
  end

  print_lambda = lambda { |m|
    # Render symbols with a leading colon
    fmt = lambda { |x|
      case x
      when Symbol then ":#{x}"
      else x
      end
    }
    s = "#{m[0]}, "
    s << if m[1].is_a?(Array)
           "[ #{fmt.call(m[1][0])}, #{m[1][1]} ]"
         else
           "#{fmt.call(m[1])}"
         end
    s
  }

  puts <<EOF
  class #{to_class_name(name)} < FFI::ITTStruct
EOF
  puts <<EOF
    layout #{members.collect(&print_lambda).join(",\n" + (' ' * 11))}
  end
  typedef #{to_class_name(name)}.by_value, #{to_ffi_name(name)}

EOF
  close_type(name)
end

# Build a set of all function pointer typedef symbols to detect struct fields
# that should be converted to :pointer when generating layouts
require 'set'
$fnptr_syms = Set.new(
  $all_types.select do |t|
    t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function)
  end.map { |t| to_ffi_name(t.name).to_s }
)

# Collect callbacks to print after all other types
callbacks = []

$all_types.each do |t|
  if t.type.is_a? YAMLCAst::Enum
    print_enum(t.name, find_enum_by_name(t.name, $itt_api))
  elsif $objects.include?(t.name)
    print_itt_object(t.name)
  elsif t.type.is_a? YAMLCAst::Struct
    struct = $all_structs.find { |s| t.type.name == s.name }
    next unless struct

    print_struct(t.name, struct)
  elsif t.type.is_a? YAMLCAst::Union
    union = $all_unions.find { |s| t.type.name == s.name }
    next unless union

    print_union(t.name, union)
  elsif t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function)
    # Defer callbacks until the end so all referenced types are defined
    callbacks << [t.name, t.type.type]
  elsif t.type.is_a?(YAMLCAst::Pointer)
    print_pointer_type(t.name)
  elsif t.type.is_a?(YAMLCAst::Int) || t.type.is_a?(YAMLCAst::Char)
    print_int_type(t.name, t.type.name)
  else
    # $stderr.puts t.inspect
  end
end

# Emit collected callbacks
callbacks.each do |name, func|
  print_function_pointer_type(name, func)
end

puts <<~EOF
  end
EOF
