require_relative 'api_model'

# What a backend owes the shared library generator: where its types come from,
# and how their names are spelled in Ruby.
#
# Every field a backend sets away from the default is a real divergence between
# that API and the others, stated in one place instead of hidden in a redefined
# function or a hardcoded constant.
class NamingContext
  attr_reader :api, :module_name

  # `api` is the one place that says which model the shared generators below
  # ask about a name.
  #
  # `class_namer` defaults to the prefix rule hip and mpi follow. A backend
  # whose headers are camelCase recases every word instead, and passes its own.
  #
  # `upcase_namespace` reports an API whose C prefix is lowercase but whose Ruby
  # class prefix is not (ze_ -> ZE); it is a property of the headers, not a
  # style choice, so it is stated rather than folded into each namer.
  def initialize(module_name:, api:, namespace_pattern:, strict: false,
                 upcase_namespace: false, class_namer: nil, scoped_namer: nil)
    @upcase_namespace = upcase_namespace
    @module_name = module_name
    @api = api
    @namespace_pattern = namespace_pattern
    @strict = strict
    # A namer is handed the context so it can reuse name_space rather than
    # restate the pattern.
    @class_namer = class_namer || ->(ctx, name) { prefixed_class_name(name, ctx.name_space(name)) }
    @scoped_namer = scoped_namer || ->(ctx, name) { "#{ctx.module_name}::#{ctx.class_name(name)}" }
  end

  # The FFI base class a layout of this kind subclasses. print_ffi_module
  # declares these under the same module name, and a named layout and an
  # anonymous one have to name the same base -- so both ask here.
  def ffi_base(kind)
    "FFI::#{@module_name}#{kind}"
  end

  # The backend's short name, as the build system spells it in filenames and
  # the trace in stream-class names. Every backend's Ruby module is its short
  # name upcased, so this is derived rather than passed in beside it.
  def backend
    @module_name.downcase
  end

  def name_space(name)
    ns = match_name_space(name, @namespace_pattern, strict: @strict)
    @upcase_namespace ? ns.upcase : ns
  end

  def class_name(name)
    @class_namer.call(self, name)
  end

  # The name the babeltrace model records for a class, as Ruby must resolve it
  # at runtime. Five backends qualify the class name with the module; itt's
  # class names already carry it, so it passes a namer that does not repeat it.
  def scoped_class_name(name)
    @scoped_namer.call(self, name)
  end
end

# Class name for an API that already spells its types in the target case, so
# the only work is the namespace prefix. hip and mpi each write theirs two ways
# -- hipDeviceProp_t and HIP_ARRAY_DESCRIPTOR -- and only the lowercase
# spelling is title-cased, leaving HipDeviceProp_t and HIP_ARRAY_DESCRIPTOR.
# The rest of the name is left exactly as the header spells it.
#
# The backends whose headers are camelCase (cuda, ze, omp, itt) do more than
# this -- they split on '_' and recase every word -- so they keep their own.
def prefixed_class_name(name, namespace)
  namespace = namespace.to_s
  rest = name.sub(/\A#{namespace}/, '')
  prefix = namespace.match?(/[[:lower:]]/) ? namespace.capitalize : namespace
  res = prefix + rest
  res[0] = res[0].upcase if res[0]&.match(/[[:lower:]]/)
  res
end

# The namespace prefix `name` starts with, as the pattern captured it, or nil.
#
# Whether nil is reachable is a property of the API, not a style choice, so it
# is opted into rather than left to whichever expression happened to be
# written. ze/omp/itt/mpi headers declare only their own types, so every name
# matches: they pass `strict: true`, and a future unprefixed type raises here
# by name instead of surfacing as a NoMethodError on nil deep inside a
# generator. cuda and hip vendor foreign types -- GLuint, dim3, VdpDevice, the
# OpenCL interop typedefs -- that belong to no namespace, so nil is a real
# answer their callers already handle.
def match_name_space(name, pattern, strict: false)
  m = name.match(pattern)
  raise "#{name} does not start with a known namespace" if m.nil? && strict

  m && m[1]
end

# What a typedef declares, as the library generators need to tell apart.
#
# An object -- a pointer to a struct the API never defines -- is a handle,
# emitted as an opaque pointer rather than a layout, so it is asked about
# before the pointer case that would otherwise claim it.
#
# A kind this returns nil for is one no backend emits from the typedef itself:
# an alias of another typedef, a bare function type, a void.
def typedef_kind(api, t)
  return :object if api.object?(t.name)

  case t.type
  when YAMLCAst::Enum then :enum
  when YAMLCAst::Struct then :struct
  when YAMLCAst::Union then :union
  when YAMLCAst::Pointer then t.type.type.is_a?(YAMLCAst::Function) ? :function_pointer : :pointer
  when YAMLCAst::Int, YAMLCAst::Char then :integer
  end
end

# Emit an FFI declaration for every typedef the API names.
#
# The walk and the classification are the same for all six backends; only the
# printing differs, so each kind is a keyword argument defaulting to the shared
# printer. A backend passes one only where its API really diverges -- cuda
# prepends a UUID module to matching structs, itt defers its callbacks -- and
# passes `nil` for a kind its bindings do not carry, which is how omp emits
# enums alone.
#
# `types` is the list to walk, the API's own unless the backend reordered it:
# ze sorts its typedefs so a layout is defined before it is used.
def print_typedefs(naming, api: naming.api, types: api.types,
                   enum: ->(name, t) { print_enum_with_namespace(naming, name, api.enum(t.type)) },
                   object: ->(name, _t) { print_object(name) },
                   struct: ->(name, t) { print_struct_with_namespace(naming, name, api.struct(t.type)) },
                   union: ->(name, t) { print_union_with_namespace(naming, name, api.union(t.type)) },
                   function_pointer: ->(name, t) { print_function_pointer_type(naming, name, t.type.type) },
                   pointer: ->(name, _t) { print_pointer_type(name) },
                   integer: ->(name, t) { print_int_type(name, t.type.name) })
  printers = { enum: enum, object: object, struct: struct, union: union,
               function_pointer: function_pointer, pointer: pointer, integer: integer }
  types.each do |t|
    printer = printers[typedef_kind(api, t)]
    printer&.call(t.name, t)
  end
end

# The C types FFI spells differently from the header. Every other type keeps
# the header's own name, as a symbol.
FFI_NAMES = {
  nil => ':anonymous',
  'unsigned int' => ':uint',
  'short' => ':short', 'short int' => ':short',
  'unsigned short' => ':ushort', 'unsigned short int' => ':ushort',
  'unsigned char' => ':uchar',
  'long' => ':long', 'long int' => ':long',
  'unsigned long' => ':ulong', 'unsigned long int' => ':ulong',
  'long long' => ':int64', 'long long int' => ':int64',
  'unsigned long long' => ':uint64', 'unsigned long long int' => ':uint64',
  'size_t' => ':size_t',
  '_Bool' => ':bool'
}.freeze

# The naming for types to_ffi_name's own rules cannot name, consulted only
# after they decline; it returns nil to defer. cuda is the one backend that
# sets it.
#
# It is a global rather than a NamingContext field because to_ffi_name is
# called from places that are handed no naming context, among them the AST-node
# methods Array#to_ffi and BitfieldAccumulator#flush_to_result. One process
# generates one backend, so the single slot is never contended -- but a second
# setter would silently replace the first.
module FFIName
  class << self
    attr_accessor :fallback
  end
  self.fallback = ->(_name) {}
end

def to_ffi_name(name)
  return FFI_NAMES[name] if FFI_NAMES.key?(name)
  return ":#{Regexp.last_match(1)}" if name.match(/^(u?int\d+)_t$/)

  FFIName.fallback.call(name) || name.to_sym.inspect
end

# FFI stores a flag by bit position and cannot express a multi-bit alias like
# ALL = READ|WRITE, so composites are printed as a comment instead.
BitfieldMembers = Struct.new(:flags, :default, :composites)

# A header writes an enum value as a decimal or hex literal, which Integer()
# reads directly. Anything else is a macro, which only the API whose header
# defines it can evaluate -- so that API passes a `resolver`, and a value
# neither rule can read raises rather than guessing.
def enum_value(val, resolver)
  return val unless val.is_a?(String)

  Integer(val, exception: false) ||
    resolver.call(val) ||
    raise("cannot read enum value #{val.inspect}; the API's resolver declined it")
end

def classify_bitfield_members(enum, resolver: ->(_val) {})
  flags = []
  composites = []
  default = nil
  counter = 0
  enum.members.each do |m|
    counter = if m.val
                enum_value(m.val, resolver)
              else
                counter + 1
              end
    if counter.positive? && counter.nobits?(counter - 1)
      flags.push [m.name.to_sym, counter.bit_length - 1]
    elsif counter.zero?
      default = m.name.to_sym
    else
      composites.push [m.name.to_sym, m.val || counter]
    end
  end
  BitfieldMembers.new(flags, default, composites)
end

# `check_flags` is true for an API that also spells a one-flag type in the
# plural: ze and omp declare both names for the same type.
def print_bitfield_with_namespace(naming, name, enum, check_flags: false, resolver: ->(_val) {})
  klass = naming.class_name(name)
  members = classify_bitfield_members(enum, resolver: resolver)
  pair = ->(m) { "#{m[0].inspect}, #{m[1]}" }
  ffi_name = to_ffi_name(name)

  puts <<EOF
  #{klass} = #{naming.module_name.downcase}bitmask #{ffi_name},
    [ #{members.flags.collect(&pair).join(",\n      ")} ]
EOF
  puts <<EOF if members.default
  #{klass}.default = #{members.default.inspect}
EOF
  puts <<EOF unless members.composites.empty?
  # #{members.composites.collect(&pair).join(",\n  # ")}
EOF
  if check_flags && ffi_name.match('_flag_t')
    puts "  #{klass}s = #{klass}"
    puts "  typedef #{ffi_name}, #{ffi_name.gsub('_flag_t', '_flags_t')}"
  end
  puts "\n"
end

# A member is printed with its value only where the header gives one; FFI
# numbers the rest itself.
#
# `filter_members` drops a member the bindings must not declare: omp's headers
# still carry a callback its runtime removed.
def print_enum_with_namespace(naming, name, enum, filter_members: ->(_m) { true })
  members = enum.members.filter(&filter_members).collect do |m|
    m.val ? "#{m.name.to_sym.inspect}, #{m.val}" : m.name.to_sym.inspect
  end
  puts <<EOF
  #{naming.class_name(name)} = #{naming.module_name.downcase}enum #{to_ffi_name(name)},
    [ #{members.join(",\n      ")} ]

EOF
end

def print_object(object)
  puts <<EOF
  typedef :pointer, #{to_ffi_name(object)}

EOF
end

# Shared by cuda/hip/mpi. ze inlines its own -- :data/:id fields, and a UUID
# printed back to front.
def print_handle_uuid_modules
  puts <<'EOF'
  module Handle
    def to_s
      s = '{ reserved: "'
      s << self[:reserved].to_a.collect { |v| "\\x%02x" % ((v + 256)%256) }.join
      s << '" }'
    end
  end

  module UUID
    def to_s
      a = self[:bytes].to_a.collect { |v| v < 0 ? 0x100 + v : v }
      s = "{ id: "
      s << "%02x" % a[0]
      s << "%02x" % a[1]
      s << "%02x" % a[2]
      s << "%02x" % a[3]
      s << "-"
      s << "%02x" % a[4]
      s << "%02x" % a[5]
      s << "-"
      s << "%02x" % a[6]
      s << "%02x" % a[7]
      s << "-"
      s << "%02x" % a[8]
      s << "%02x" % a[9]
      s << "-"
      s << "%02x" % a[10]
      s << "%02x" % a[11]
      s << "%02x" % a[12]
      s << "%02x" % a[13]
      s << "%02x" % a[14]
      s << "%02x" % a[15]
      s << " }"
    end
  end
EOF
end

# The FFI base classes every backend's bindings open with.
#
# `struct`, `union` and `inline_array` are false for a backend whose bindings
# never declare one: omp binds enums alone. `enclosing_module` is false for ze,
# whose classes resolve their own names.
def print_ffi_module(namespace, struct: true, union: true, inline_array: true,
                     enclosing_module: true)
  puts <<~EOF
    require 'ffi'
    module FFI

  EOF

  print_ffi_struct_class(namespace, enclosing_module) if struct
  print_ffi_enum_class(namespace)
  print_ffi_union_class(namespace, enclosing_module) if union
  print_ffi_bitmask_class(namespace)
  print_ffi_library_module(namespace)
  print_ffi_inline_array_class if inline_array
  print_ffi_epilogue
end

def print_ffi_struct_class(namespace, enclosing_module)
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

def print_ffi_enum_class(namespace)
  puts <<EOF
  class #{namespace}Enum < Enum
  end
EOF
end

def print_ffi_union_class(namespace, enclosing_module)
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

# `default` is the value the API spells as zero: FFI has no name for an empty
# flag set, so the bitmask maps it both ways itself.
def print_ffi_bitmask_class(namespace)
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

def print_ffi_library_module(namespace)
  puts <<EOF
  module Library
    def #{namespace.downcase}bitmask(*args)
      generic_enum(FFI::#{namespace}Bitmask, *args)
    end
    def #{namespace.downcase}enum(*args)
      generic_enum(FFI::#{namespace}Enum, *args)
    end
  end
EOF
end

def print_ffi_inline_array_class
  puts <<EOF

  class Struct::InlineArray
    def to_s
      s = "[ "
      s << to_a.join(", ")
      s << " ]"
    end
  end
EOF
end

def print_ffi_epilogue
  puts <<~EOF

      class Pointer
        def to_s
          "0x%016x" % address
        end
      end
    end
  EOF
end

def close_type(naming, name)
  naming.api.aliases_of(name).each do |t|
    puts <<EOF
  typedef #{to_ffi_name(name)}, #{to_ffi_name(t.name)}

EOF
    close_type(naming, t.name)
  end
end

# One `layout` argument: `:name, :type`, or `:name, [ :type, count ]` for an
# inline array.
def ffi_layout_member(name, type)
  spelled = type.is_a?(Array) ? "[ #{type[0]}, #{type[1]} ]" : type.to_s
  "#{name}, #{spelled}"
end

# Continuation lines align under the first item, which starts where the text
# introducing it ends -- the literals below are that text, as the heredocs
# spell it.
LAYOUT_CONTINUATION = "\n#{' ' * '    layout '.length}".freeze
CALLBACK_CONTINUATION = "\n#{' ' * '  callback [ '.length}".freeze

def ffi_layout(members)
  members.collect { |name, type| ffi_layout_member(name, type) }.join(",#{LAYOUT_CONTINUATION}")
end

def inline_layout(members)
  members.collect { |name, type| ffi_layout_member(name, type) }.join(', ')
end

def print_union_with_namespace(naming, name, union)
  members = union.to_ffi(naming)
  puts <<EOF
  class #{naming.class_name(name)} < #{naming.ffi_base('Union')}
    layout #{ffi_layout(members)}
  end
  typedef #{naming.class_name(name)}.by_value, #{to_ffi_name(name)}

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

def print_function_pointer_type(naming, name, func)
  type, params = func.to_ffi(naming)
  puts <<EOF
  callback #{to_ffi_name(name)},
           [ #{params.join(",#{CALLBACK_CONTINUATION}")} ],
           #{type}

EOF
end

def print_struct_prepending_uuid(naming, name, struct)
  prepends = naming.class_name(name).match('UUID') ? ['UUID'] : []
  print_struct_with_namespace(naming, name, struct, prepends: prepends)
end

# `members` defaults to the struct's own layout. A backend overrides it when it
# has to rewrite member types before emitting -- itt refers to function
# pointers it only defines further down the file, so it passes :pointer for
# them instead of a name FFI cannot resolve yet.
def print_struct_with_namespace(naming, name, struct, prepends: [], initializer: nil, close: true,
                                members: struct.to_ffi(naming))
  puts <<EOF
  class #{naming.class_name(name)} < #{naming.ffi_base('Struct')}
EOF
  prepends.each do |prep|
    puts <<EOF
    prepend #{prep}
EOF
  end
  puts <<EOF
    layout #{ffi_layout(members)}
EOF
  puts initializer if initializer
  puts <<EOF
  end
  typedef #{naming.class_name(name)}.by_value, #{to_ffi_name(name)}

EOF
  close_type(naming, name) if close
end

module YAMLCAst
  module Composite
    def to_ffi(naming)
      unamed_count = 0
      result = []
      bitfields = BitfieldAccumulator.new
      members.each do |m|
        if m.num_bits
          bitfields.add(m)
        else
          bitfields.flush_to_result(result)
          result << [m.name ? m.name.to_sym.inspect : ":_unamed_#{unamed_count += 1}",
                     member_to_ffi(naming, m.type)]
        end
      end
      bitfields.flush_to_result(result)
      result
    end

    private

    def member_to_ffi(naming, type)
      api = naming.api
      case type
      when Array
        type.to_ffi
      when Pointer
        ':pointer'
      when Struct, Union
        # A named layout the API also typedefs can be referred to by that name.
        # An anonymous one, or one this API never typedefs, has to be spelled
        # out as a class defined right here.
        if type.name && api.typedef?(type.name)
          to_ffi_name(type.name)
        else
          base, definition = if type.is_a?(Struct)
                               [naming.ffi_base('Struct'), api.struct(type)]
                             else
                               [naming.ffi_base('Union'), api.union(type)]
                             end
          "(Class::new(#{base}) { layout #{inline_layout(definition.to_ffi(naming))} }.by_value)"
        end
      else
        type.name ? to_ffi_name(type.name) : raise("unknown type: #{type}")
      end
    end

    # TODO: FFI doesn't support bitfields. Consecutive bitfield members are
    # collapsed into a single integer field named `_aggregated_bitfields_N`,
    # using the declared type of the first bitfield as the storage type.
    # For now Will crash if differnce storage type are used
    # Individual bit flags are not accessible; to_s prints the raw integer value.
    #
    # Example: struct { unsigned a:1; unsigned b:1; int x; unsigned c:3; }
    # becomes: layout :_aggregated_bitfields_1, :uint,  # a + b
    #                 :x, :int,
    #                 :_aggregated_bitfields_2, :uint   # c
    class BitfieldAccumulator
      def initialize
        @count = 0
        @group = 0
        @type = nil
      end

      def add(member)
        if @type && @type != member.type.name
          raise "Mixed bitfield types (#{@type} vs #{member.type.name}) not supported"
        end

        @type ||= member.type.name
        @count += member.num_bits
      end

      def flush_to_result(result)
        return if @count.zero?

        @group += 1
        result << [":_aggregated_bitfields_#{@group}", to_ffi_name(@type)]
        @count = 0
        @type = nil
      end
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
    def to_ffi(naming)
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
             naming.api.struct_names.include?(par.type.type.name)
            "#{naming.class_name(par.type.type.name)}.ptr"
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
