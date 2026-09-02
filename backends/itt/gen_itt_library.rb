require_relative 'gen_itt_library_base'
require 'set'

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
    #{NAMING.class_name(name)} = ittenum #{to_ffi_name(name)},
      [ #{resolved.map(&pair).join(",\n        ")} ]
  RUBY
end

print_ffi_module(:ITT)

puts <<~EOF
  module ITT
    extend FFI::Library

EOF

# itt defines its callbacks at the end of the file, so a struct member typed as
# one would name a callback FFI has not seen yet. Those members are emitted as
# a plain :pointer, which is the same machine type.
def print_struct(name, struct, fnptr_syms)
  as_pointer = ->(t) { fnptr_syms.include?(t.to_s) ? ':pointer' : t }
  members = struct.to_ffi(NAMING).map do |field, type|
    [field, type.is_a?(Array) ? [as_pointer.call(type[0]), type[1]] : as_pointer.call(type)]
  end
  print_struct_with_namespace(NAMING, name, struct, members: members)
end

# Build a set of all function pointer typedef symbols to detect struct fields
# that should be converted to :pointer when generating layouts
fnptr_syms = Set.new(
  API.types.select do |t|
    t.type.is_a?(YAMLCAst::Pointer) && t.type.type.is_a?(YAMLCAst::Function)
  end.map { |t| to_ffi_name(t.name).to_s }
)

callbacks = []

print_typedefs(
  NAMING,
  enum: ->(name, t) { print_enum(name, API.enum(t.type)) },
  struct: ->(name, t) { print_struct(name, API.struct(t.type), fnptr_syms) },
  function_pointer: ->(name, t) { callbacks << [name, t.type.type] }
)

# Deferred to the end of the file so every type they name is already defined.
callbacks.each do |name, func|
  print_function_pointer_type(NAMING, name, func)
end

puts <<~EOF
  end
EOF
