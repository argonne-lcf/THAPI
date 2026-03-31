require_relative 'yaml_ast'

def has_typedef?(name)
  $all_types.any? { |t| t.type.respond_to?(:name) && t.type.name == name }
end

def to_ffi_name(name, default = true)
  case name
  when nil
    return ':anonymous'
  when 'unsigned int'
    return ':uint'
  when 'unsigned short', 'unsigned short int'
    return ':ushort'
  when 'unsigned char'
    return ':uchar'
  when 'unsigned long long', 'unsigned long long int'
    return ':uint64'
  when 'size_t'
    return ':size_t'
  when /^(u?int\d+)_t$/
    return ":#{Regexp.last_match(1)}"
  when '_Bool'
    return ':bool'
  end
  name.to_sym.inspect if default
end

def popcount(x)
  raise 'Unsigned integer needed!' if x < 0

  c = 0
  while x != 0
    c += 1 if x & 1 != 0
    x >>= 1
  end
  c
end

def firstbitpos(x)
  raise 'Signed positive integer needed!' if x <= 0

  r = 0
  while x & 1 == 0
    r += 1
    x >>= 1
  end
  r
end

def print_bitfield_with_namespace(namespace, name, enum, check_flags: false)
  special_values = []
  members = []
  default = nil
  counter = 0
  enum.members.each do |m|
    if m.val
      counter = m.val.is_a?(String) ? eval(m.val) : m.val
    else
      counter += 1
    end
    if counter > 0 && popcount(counter) == 1
      members.push [m.name.to_sym, firstbitpos(counter)]
    elsif counter == 0
      default = m.name.to_sym
    else
      r = [m.name.to_sym]
      r.push m.val || counter
      special_values.push r
    end
  end
  print_lambda = lambda { |m|
    "#{m[0].inspect}, #{m[1]}"
  }
  puts <<EOF
  #{to_class_name(name)} = #{namespace.downcase}bitmask #{to_ffi_name(name)},
    [ #{members.collect(&print_lambda).join(",\n      ")} ]
EOF
  if default
    puts <<EOF
  #{to_class_name(name)}.default = #{default.inspect}
EOF
  end
  unless special_values.empty?
    puts <<EOF
  # #{special_values.collect(&print_lambda).join(",\n  # ")}
EOF
  end
  if check_flags && to_ffi_name(name).match('_flag_t')
    puts "  #{to_class_name(name)}s = #{to_class_name(name)}"
    puts "  typedef #{to_ffi_name(name)}, #{to_ffi_name(name).gsub('_flag_t', '_flags_t')}"
  end
  puts "\n"
end

def print_enum_with_namespace(namespace, name, enum, filter_members: ->(_m) { true })
  members = enum.members.filter(&filter_members).collect do |m|
    r = [m.name.to_sym]
    r.push m.val if m.val
    r
  end
  print_lambda = lambda { |m|
    s = "#{m[0].inspect}"
    s << ", #{m[1]}" if m.size == 2
    s
  }
  puts <<EOF
  #{to_class_name(name)} = #{namespace.downcase}enum #{to_ffi_name(name)},
    [ #{members.collect(&print_lambda).join(",\n      ")} ]

EOF
end

def print_object(object)
  puts <<EOF
  typedef :pointer, #{to_ffi_name(object)}

EOF
end

def print_ffi_module(namespace, struct: true, union: true, enum: true, bitmask: true, inline_array: true,
                     enclosing_module: true)
  puts <<~EOF
    require 'ffi'
    module FFI

  EOF

  if struct
    puts <<EOF
  class #{namespace}Struct < Struct
EOF
    puts <<EOF if enclosing_module
    def self.enclosing_module
      #{namespace}
    end

EOF
    puts <<EOF
    def to_h
      members.zip(values).to_h
    end

    def to_s
      s = "{ "
      s << to_h.each.collect { |k, v|
        if v.kind_of? Array
          "\#{k}: [ \#{v.join(", ")} ]"
        else
          "\#{k}: \#{v}"
        end
      }.join(", ")
      s << " }"
      s
    end
  end
EOF
  end

  if enum
    puts <<EOF
  class #{namespace}Enum < Enum
  end
EOF
  end

  if union
    puts <<EOF
  class #{namespace}Union < Union
EOF
    puts <<EOF if enclosing_module
    def self.enclosing_module
      #{namespace}
    end
EOF
    puts <<EOF
  end
EOF
  end

  if bitmask
    puts <<EOF
  class #{namespace}Bitmask < Bitmask
    def default=(default)
      @default = default
    end

    def []
      if @default && query.size == 1
        if query[0] == 0
          return @default
        elsif query[0] == @default
          return 0
        end
      end
      super
    end

    def to_native(query, ctx)
      return 0 if query.nil?
      flat_query = [query].flatten
      return 0 if flat_query.size == 1 && query[0] == @default
      super
    end

    def from_native(val, ctx)
      return [@default] if val == 0 && @default
      super
    end
  end
EOF
  end

  if bitmask || enum
    puts <<EOF
  module Library
EOF
    puts <<EOF if bitmask
    def #{namespace.downcase}bitmask(*args)
      generic_enum(FFI::#{namespace}Bitmask, *args)
    end
EOF
    puts <<EOF if enum
    def #{namespace.downcase}enum(*args)
      generic_enum(FFI::#{namespace}Enum, *args)
    end
EOF
    puts <<EOF
  end
EOF
  end
  puts <<EOF if inline_array

  class Struct::InlineArray
    def to_s
      s = "[ "
      s << to_a.join(", ")
      s << " ]"
    end
  end
EOF
  puts <<~EOF

      class Pointer
        def to_s
          "0x%016x" % address
        end
      end
    end
  EOF
end

def close_type(name)
  $all_types.select do |t|
    t.type.is_a?(YAMLCAst::CustomType) && t.type.name == name
  end.each do |t|
    puts <<EOF
  typedef #{to_ffi_name(name)}, #{to_ffi_name(t.name)}

EOF
    close_type(t.name)
  end
end

def print_union_with_namespace(namespace, name, union)
  members = union.to_ffi
  print_lambda = lambda { |m|
    s = "#{m[0]}, "
    s << if m[1].is_a?(Array)
           "[ #{m[1][0]}, #{m[1][1]} ]"
         else
           "#{m[1]}"
         end
    s
  }
  puts <<EOF
  class #{to_class_name(name)} < FFI::#{namespace}Union
    layout #{members.collect(&print_lambda).join(",\n" + (' ' * 11))}
  end
  typedef #{to_class_name(name)}.by_value, #{to_ffi_name(name)}

EOF
end

def print_pointer_type(name)
  puts <<EOF
  typedef :pointer, #{to_ffi_name(name)}

EOF
end

def print_int_type(name, t_name)
  puts <<EOF
  typedef #{to_ffi_name(t_name)}, #{to_ffi_name(name)}
EOF
end

def print_function_pointer_type(name, func)
  type, params = func.to_ffi
  puts <<EOF
  callback #{to_ffi_name(name)},
           [ #{params.join(",\n" + (' ' * 13))} ],
           #{type}

EOF
end

def print_struct_with_namespace(namespace, name, struct, prepends: [], initializer: nil, close: true)
  members = struct.to_ffi
  print_lambda = lambda { |m|
    s = "#{m[0]}, "
    s << if m[1].is_a?(Array)
           "[ #{m[1][0]}, #{m[1][1]} ]"
         else
           "#{m[1]}"
         end
    s
  }
  puts <<EOF
  class #{to_class_name(name)} < FFI::#{namespace}Struct
EOF
  prepends.each do |prep|
    puts <<EOF
    prepend #{prep}
EOF
  end
  puts <<EOF
    layout #{members.collect(&print_lambda).join(",\n" + (' ' * 11))}
EOF
  puts initializer if initializer
  puts <<EOF
  end
  typedef #{to_class_name(name)}.by_value, #{to_ffi_name(name)}

EOF
  close_type(name) if close
end

module YAMLCAst
  module Composite
    def to_ffi
      unamed_count = 0
      members.map do |m|
        mt = if m.type.is_a?(Array)
               m.type.to_ffi
             elsif m.type.is_a?(Pointer)
               ':pointer'
             elsif m.type.name && has_typedef?(m.type.name)
               to_ffi_name(m.type.name)
             elsif m.type.is_a?(Struct)
               s = m.type.name ? $all_structs.find { |st| st.name == m.type.name } : m.type
               "(Class::new(#{FFI_STRUCT}) { layout #{gen_layout(s.to_ffi)} }.by_value)"
             elsif m.type.is_a?(Union)
               u = m.type.name ? $all_unions&.find { |un| un.name == m.type.name } : m.type
               "(Class::new(#{FFI_UNION}) { layout #{gen_layout(u.to_ffi)} }.by_value)"
             elsif m.type.name
               to_ffi_name(m.type.name)
             else
               raise "unknown type: #{m.type}"
             end
        [m.name ? m.name.to_sym.inspect : ":_unamed_#{unamed_count += 1}", mt]
      end
    end

    private

    def gen_layout(membs)
      membs.map do |a, b|
        s = "#{a}, "
        s << (b.is_a?(::Array) ? "[ #{b[0]}, #{b[1]} ]" : b)
      end.join(', ')
    end
  end

  class Struct
    include Composite
  end

  class Union
    include Composite
  end

  class Array
    def to_ffi
      t = case type
          when Pointer
            ':pointer'
          else
            to_ffi_name(type.name)
          end
      [t, length]
    end
  end

  class Function
    def to_ffi
      t = if type.respond_to?(:name)
            to_ffi_name(type.name)
          elsif type.is_a?(Pointer)
            ':pointer'
          else
            raise "unknown return type: #{type}"
          end
      p = (params || []).collect do |par|
        if par.type.is_a?(Pointer)
          if par.type.type.respond_to?(:name) &&
             $all_struct_names.include?(par.type.type.name)
            "#{to_class_name(par.type.type.name)}.ptr"
          else
            ':pointer'
          end
        else
          to_ffi_name(par.type.name)
        end
      end
      [t, p]
    end
  end
end
