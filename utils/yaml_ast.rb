require 'yaml'
require 'set'

module YAMLCAst
  class Type
    attr_reader :const, :restrict, :volatile

    def volatile?
      @volatile
    end

    def const?
      @const
    end

    def restrict?
      @restrict
    end

    def initialize(const: nil, restrict: nil, volatile: nil)
      @const = const
      @restrict = restrict
      @volatile = volatile
    end

    def self.from_yaml_ast(node)
      KIND_MAP[node['kind']].from_yaml_ast(node)
    end
  end

  class Declaration
    attr_reader :name, :type, :init, :num_bits, :inline, :storage

    def inline?
      @inline
    end

    def initialize(type:, name: nil, init: nil, num_bits: nil, inline: nil, storage: nil)
      @name = name
      @type = type
      @init = init
      @num_bits = num_bits
      @inline = inline
      @storage = storage
    end

    def self.from_yaml_ast(node)
      new_node = node.dup
      new_node['type'] = Type.from_yaml_ast(node['type'])
      new_node.transform_keys!(&:to_sym)
      new(**new_node)
    end

    def to_s
      str = ''
      str << 'inline ' if inline?
      str << "#{storage} " if storage
      str << type.to_s(name)
      str << " = #{init}" if init
      str << " : #{num_bits}" if num_bits
      str
    end
  end

  class DirectType < Type
    attr_reader :name

    def initialize(name: nil, const: nil, restrict: nil, volatile: nil)
      @name = name
      super(const: const, restrict: restrict, volatile: volatile)
    end

    def self.from_yaml_ast(node)
      new_node = node.dup
      new_node.delete('kind')
      new_node.transform_keys!(&:to_sym)
      new(**new_node)
    end

    def full_name
      @name
    end

    def to_s(name = nil)
      str = ''
      str << 'const ' if const?
      str << 'restrict ' if restrict?
      str << 'volatile ' if volatile?
      str << "#{full_name}"
      str << " #{name}" if name
      str
    end
  end

  class Int < DirectType
  end

  class Void < DirectType
  end

  class Float < DirectType
  end

  class Char < DirectType
  end

  class Bool < DirectType
  end

  class Complex < DirectType
  end

  class Imaginary < DirectType
  end

  class CustomType < DirectType
  end

  class Struct < DirectType
    attr_reader :members

    def initialize(name: nil, members: nil, const: nil, restrict: nil, volatile: nil)
      @members = members
      super(name: name, const: const, restrict: restrict, volatile: volatile)
    end

    def self.from_yaml_ast(node)
      new_node = node.dup
      new_node.delete('kind')
      new_node['members'] = new_node['members'].collect { |m| Declaration.from_yaml_ast(m) } if new_node['members']
      new_node.transform_keys!(&:to_sym)
      new(**new_node)
    end

    def full_name
      str = 'struct'
      str << " #{name}" if name
      str << " {#{members.join('; ')};}" if members
      str
    end
  end

  class Union < DirectType
    attr_reader :members

    def initialize(name: nil, members: nil, const: nil, restrict: nil, volatile: nil)
      @members = members
      super(name: name, const: const, restrict: restrict, volatile: volatile)
    end

    def self.from_yaml_ast(node)
      new_node = node.dup
      new_node.delete('kind')
      new_node['members'] = new_node['members'].collect { |m| Declaration.from_yaml_ast(m) } if new_node['members']
      new_node.transform_keys!(&:to_sym)
      new(**new_node)
    end

    def full_name
      str = 'union'
      str << " #{name}" if name
      str << " {#{members.join('; ')};}" if members
      str
    end
  end

  class Enumerator
    attr_reader :name, :val

    def initialize(name:, val: nil)
      @name = name
      @val = val
    end

    def self.from_yaml_ast(node)
      new_node = node.dup
      new_node.transform_keys!(&:to_sym)
      new(**new_node)
    end

    def to_s
      str = "#{name}"
      str << " = #{val}" if val
      str
    end
  end

  class Enum < DirectType
    attr_reader :members

    def initialize(name: nil, members: nil, const: nil, restrict: nil, volatile: nil)
      @members = members
      super(name: name, const: const, restrict: restrict, volatile: volatile)
    end

    def self.from_yaml_ast(node)
      new_node = node.dup
      new_node.delete('kind')
      new_node['members'] = new_node['members'].collect { |m| Enumerator.from_yaml_ast(m) } if new_node['members']
      new_node.transform_keys!(&:to_sym)
      new(**new_node)
    end

    def full_name
      str = 'enum'
      str << " #{name}" if name
      str << " {#{members.join(', ')}}" if members
      str
    end
  end

  class IndirectType < Type
    attr_reader :type

    def initialize(type: nil, const: nil, restrict: nil, volatile: nil)
      @type = type
      super(const: const, restrict: restrict, volatile: volatile)
    end

    def self.from_yaml_ast(node)
      new_node = node.dup
      new_node.delete('kind')
      new_node['type'] = Type.from_yaml_ast(new_node['type']) if new_node['type']
      new_node.transform_keys!(&:to_sym)
      new(**new_node)
    end
  end

  class Pointer < IndirectType
    def to_s(name = nil)
      str = '*'
      str << 'const ' if const?
      str << 'restrict ' if restrict?
      str << 'volatile ' if volatile?
      str << "#{name}" if name
      str =
        case type
        when Function, Array
          "(#{str})"
        else
          str
        end
      if type
        type.to_s(str)
      else
        str
      end
    end
  end

  class Array < IndirectType
    attr_reader :length

    def initialize(type: nil, length: nil, restrict: nil)
      @length = length
      super(type: type, restrict: restrict)
    end

    def to_s(name = nil)
      str = "#{name}[#{length}]"
      if type
        type.to_s(str)
      else
        str
      end
    end
  end

  class Function < IndirectType
    attr_reader :params, :var_args, :name

    def var_args?
      @var_args
    end

    def initialize(type: nil, params: nil, var_args: nil)
      @params = params
      @var_args = var_args
      super(type: type)
    end

    def self.from_yaml_ast(node)
      new_node = node.dup
      new_node.delete('kind')
      new_node['type'] = Type.from_yaml_ast(new_node['type']) if new_node['type']
      new_node['params'] = new_node['params'].collect { |p| Declaration.from_yaml_ast(p) } if new_node['params']
      new_node.transform_keys!(&:to_sym)
      new(**new_node)
    end

    def to_s(name = nil, no_types = false)
      str = ''
      if params
        str << if params.empty?
                 'void'
               elsif no_types
                 params.collect(&:name).join(', ')
               else
                 params.join(', ')
               end
      end
      str << ', ...' if var_args?
      str = "#{name}(#{str})"
      if type
        type.to_s(str)
      else
        str
      end
    end
  end

  KIND_MAP = {
    'int' => Int,
    'void' => Void,
    'float' => Float,
    'char' => Char,
    'bool' => Bool,
    'complex' => Complex,
    'imaginary' => Imaginary,
    'custom_type' => CustomType,
    'struct' => Struct,
    'union' => Union,
    'enum' => Enum,
    'pointer' => Pointer,
    'array' => Array,
    'function' => Function,
    'declaration' => Declaration,
  }

  # Parse an api.yaml into its five lists of AST nodes, keyed as the file keys
  # them. A header that declares no unions simply has no 'unions' key, so a
  # list the file omits is absent rather than empty -- ApiModel, which is what
  # callers actually want, supplies the defaults.
  def self.load_file(path)
    from_yaml_ast(YAML.load_file(path))
  end

  def self.from_yaml_ast(ast)
    res = {}
    ast.each do |k, v|
      case k
      when 'typedefs'
        res[k] = v.collect { |d| Declaration.from_yaml_ast(d) }
      when 'functions'
        res[k] = v.collect do |d|
          new_d = {}
          new_d['name'] = d['name']
          new_d['type'] = { 'kind' => 'function', 'type' => d['type'], 'params' => d['params'], 'var_args' => d['var_args'] }
          new_d['inline'] = d['inline']
          new_d['storage'] = d['storage']
          Declaration.from_yaml_ast(new_d)
        end
      else
        res[k] = v.collect { |s| KIND_MAP[k.chomp('s')].from_yaml_ast(s) }
      end
    end
    res
  end
end

def transitive_closure(types, arr)
  sz = arr.size
  loop do
    arr.concat(types.filter_map do |t|
      t.name if t.type.is_a?(YAMLCAst::CustomType) && arr.include?(t.type.name)
    end).uniq!
    break if sz == arr.size

    sz = arr.size
  end
  arr
end

def transitive_closure_map(types, map)
  sz = map.size
  loop do
    types.select do |t|
      t.type.is_a?(YAMLCAst::CustomType) && map.include?(t.type.name)
    end.each { |t| map[t.name] = map[t.type.name] }
    break if sz == map.size

    sz = map.size
  end
end

def find_types(types, cast_type, arr = nil)
  res = types.select { |t| t.type.is_a? cast_type }.collect { |t| t.name }
  if arr
    arr.concat res
    res = arr
  end
  transitive_closure(types, res)
end

def find_types_map(types, cast_type, map)
  types.select { |t| t.type.is_a? cast_type }.each do |t|
    map[t.name] = map[t.type.name]
  end
  transitive_closure_map(types, map)
end

# Those are c types and standard library types.
# We don't keep those part of the AST.
# Some are platform dependent. TODO add platform target.

# signed, size, ffi_type
INT_TYPE_MAP = {
  'char' => [false, 1, 'ffi_type_sint8'],
  'unsigned char' => [false, 1, 'ffi_type_uint8'],
  'short' => [true, 2, 'ffi_type_sint16'],
  'unsigned short' => [false, 2, 'ffi_type_uint16'],
  'short int' => [true, 2, 'ffi_type_sint16'],
  'unsigned short int' => [false, 2, 'ffi_type_uint16'],
  'int' => [true, 4, 'ffi_type_sint32'],
  'unsigned int' => [false, 4, 'ffi_type_uint32'],
  'long' => [true, 8, 'ffi_type_sint64'],
  'unsigned long' => [false, 8, 'ffi_type_uint64'],
  'long int' => [true, 8, 'ffi_type_sint64'],
  'unsigned long int' => [false, 8, 'ffi_type_uint64'],
  'long long' => [true, 8, 'ffi_type_sint64'],
  'unsigned long long' => [false, 8, 'ffi_type_uint64'],
  'long long int' => [true, 8, 'ffi_type_sint64'],
  'unsigned long long int' => [false, 8, 'ffi_type_uint64'],
  'int8_t' => [true, 1, 'ffi_type_sint8'],
  'uint8_t' => [false, 1, 'ffi_type_uint8'],
  'int16_t' => [true, 2, 'ffi_type_sint16'],
  'uint16_t' => [false, 2, 'ffi_type_uint16'],
  'int32_t' => [true, 4, 'ffi_type_sint32'],
  'uint32_t' => [false, 4, 'ffi_type_uint32'],
  'int64_t' => [true, 8, 'ffi_type_sint64'],
  'uint64_t' => [false, 8, 'ffi_type_uint64'],
  'ssize_t' => [true, 8, 'ffi_type_pointer'],
  'size_t' => [false, 8, 'ffi_type_pointer'],
  'intptr_t' => [true, 8, 'ffi_type_pointer'],
  'uintptr_t' => [false, 8, 'ffi_type_pointer'],
  '_Bool' => [false, 1, 'ffi_type_uint8'],
}

INT_SIGN_MAP = INT_TYPE_MAP.map { |k, v| [k, v[0]] }.to_h
INT_SIZE_MAP = INT_TYPE_MAP.map { |k, v| [k, v[1]] }.to_h

FFI_INT_TYPE_MAP = INT_TYPE_MAP.map { |k, v| [k, v[2]] }.to_h
INT_TYPES = INT_TYPE_MAP.keys

# Integer types the tracer logs in hex rather than decimal. An API can name
# more of its own -- see find_all_types' extra_hex_ints.
HEX_INT_TYPES = %w[
  intptr_t
  uintptr_t
].freeze

FFI_FLOAT_TYPE_MAP = {
  'float' => 'ffi_type_float',
  'double' => 'ffi_type_double',
}

FFI_TYPE_MAP = {}

# A typedef of pointer-to-struct is an opaque handle -- an "object" -- rather
# than a pointer the tracer should dereference.
#
# The struct is not always named directly: headers also spell it in two steps,
#   typedef struct _Foo Foo;   typedef Foo *Foo_t;
# which puts a CustomType between the pointer and the struct. Resolve those
# aliases through `types` so both spellings classify the same way; otherwise
# Foo_t lands in POINTER_TYPES and the backend has to correct it by hand.
def object_typedef?(t, types)
  return false unless t.type.is_a?(YAMLCAst::Pointer)

  target = t.type.type
  seen = Set.new
  while target.is_a?(YAMLCAst::CustomType) && seen.add?(target.name)
    aliased = types.find { |x| x.name == target.name }
    break unless aliased

    target = aliased.type
  end
  target.is_a?(YAMLCAst::Struct)
end

# How one API's typedef names sort into the categories the tracer generators
# care about. Derived from the typedef list alone, so the same input always
# gives the same object.
#
# `integers` starts from the fixed C scalar names rather than being purely the
# API's own: a typedef chain bottoms out in `int` or `uint32_t`, so the
# category has to contain both to answer "is this an integer?" in one lookup.
TypeClasses = Struct.new(:objects, :integers, :hex_ints, :enums, :structs, :unions, :pointers,
                         keyword_init: true) do
  # The one category a typedef name falls into, or nil when this API never
  # names it. The order is the answer: an object typedef is a pointer under the
  # hood and a hex int is an integer, so the more specific category has to win.
  # The categories are otherwise disjoint (asserted across all seven backends),
  # so nothing below the first match can also apply.
  #
  # Callers pick a tracepoint macro per category rather than per name, which is
  # why the ladder is here once instead of once per lttng_type.
  def category_of(name)
    case name
    when *objects, *pointers then :address
    when *hex_ints then :hex_int
    when *integers then :integer
    when *enums then :enum
    when *structs, *unions then :aggregate
    end
  end

  def aggregate?(name)
    category_of(name) == :aggregate
  end
end

# Sort every typedef in `types` into a TypeClasses. Pure: nothing outside the
# returned object is touched.
#
# `extra_hex_ints` are API-specific integer types to log in hex: cuda's
# CUdeviceptr is an address held in an integer, so hex reads far better than
# decimal. An API declares them here rather than pushing onto the shared list.
def find_all_types(types, extra_hex_ints: [])
  objects = transitive_closure(types, types.filter_map { |t| t.name if object_typedef?(t, types) })
  # Int and Char share one category, and the char pass closes over the list the
  # int pass produced, so a typedef aliasing either resolves the same way.
  integers = find_types(types, YAMLCAst::Int, INT_TYPES.dup)
  integers = find_types(types, YAMLCAst::Char, integers)
  pointers = types.filter_map do |t|
    t.name if (t.type.is_a?(YAMLCAst::Pointer) && !object_typedef?(t, types)) || t.type.is_a?(YAMLCAst::Function)
  end

  TypeClasses.new(
    objects: objects, integers: integers, pointers: pointers,
    hex_ints: HEX_INT_TYPES + extra_hex_ints,
    enums: find_types(types, YAMLCAst::Enum),
    structs: find_types(types, YAMLCAst::Struct),
    unions: find_types(types, YAMLCAst::Union)
  ).each_pair { |_, v| v.freeze }.freeze
end

# Each struct typedef mapped to its member list, so a meta-parameter naming
# `a->b` can be resolved to b's declaration. The layout is either inline on the
# typedef or carried by a separately declared struct of the same name.
def find_struct_map(types, structs)
  struct_map = {}
  types.select { |t| t.type.is_a? YAMLCAst::Struct }.each do |t|
    if t.type.members
      struct_map[t.name] = t.type.members
    else
      mapped = structs.find { |str| str.name == t.type.name }
      struct_map[t.name] = mapped.members if mapped
    end
  end
  transitive_closure_map(types, struct_map)
  struct_map
end

def gen_ffi_type_map(types, type_classes)
  find_types_map(types, YAMLCAst::Int, INT_SIGN_MAP)
  find_types_map(types, YAMLCAst::Int, INT_SIZE_MAP)
  find_types_map(types, YAMLCAst::Int, FFI_INT_TYPE_MAP)

  find_types_map(types, YAMLCAst::Char, INT_SIGN_MAP)
  find_types_map(types, YAMLCAst::Char, INT_SIZE_MAP)
  find_types_map(types, YAMLCAst::Char, FFI_INT_TYPE_MAP)

  find_types_map(types, YAMLCAst::Float, FFI_FLOAT_TYPE_MAP)
  FFI_TYPE_MAP.merge!(FFI_INT_TYPE_MAP, FFI_FLOAT_TYPE_MAP)
  type_classes.objects.each do |o|
    FFI_TYPE_MAP[o] = 'ffi_type_pointer'
    INT_SIZE_MAP[o] = 8
    INT_SIGN_MAP[o] = false
  end
  # Debatable
  type_classes.enums.each do |e|
    FFI_TYPE_MAP[e] = 'ffi_type_sint32'
    INT_SIZE_MAP[e] = 4
    INT_SIGN_MAP[e] = true
  end
  type_classes.pointers.each do |p|
    FFI_TYPE_MAP[p] = 'ffi_type_pointer'
    INT_SIZE_MAP[p] = 8
    INT_SIGN_MAP[p] = false
  end
end
