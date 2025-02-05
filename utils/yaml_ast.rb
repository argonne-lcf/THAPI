module YAMLCAst

  class Type
    attr_reader :const
    attr_reader :restrict
    attr_reader :volatile

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
      KIND_MAP[node["kind"]].from_yaml_ast(node)
    end
  end

  class Declaration
    attr_reader :name
    attr_reader :type
    attr_reader :init
    attr_reader :num_bits
    attr_reader :inline
    attr_reader :storage

    def inline?
      @inline
    end

    def initialize(name: nil, type:, init: nil, num_bits: nil, inline: nil, storage: nil)
      @name = name
      @type = type
      @init = init
      @num_bits = num_bits
      @inline = inline
      @storage = storage
    end

    def self.from_yaml_ast(node)
      new_node = node.dup
      new_node["type"] = Type.from_yaml_ast(node["type"])
      new_node.transform_keys!(&:to_sym)
      self.new(**new_node)
    end

    def to_s
      str = ""
      str << "inline " if inline?
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
      new_node.delete("kind")
      new_node.transform_keys!(&:to_sym)
      self.new(**new_node)
    end

    def full_name
      @name
    end

    def to_s(name = nil)
      str = ""
      str << "const " if const?
      str << "restrict " if restrict?
      str << "volatile " if volatile?
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
      new_node.delete("kind")
      if new_node["members"]
        new_node["members"] = new_node["members"].collect { |m| Declaration.from_yaml_ast(m) }
      end
      new_node.transform_keys!(&:to_sym)
      self.new(**new_node)
    end

    def full_name
      str = "struct"
      str << " #{name}" if name
      str << " {#{members.join("; ")};}" if members
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
      new_node.delete("kind")
      if new_node["members"]
        new_node["members"] = new_node["members"].collect { |m| Declaration.from_yaml_ast(m) }
      end
      new_node.transform_keys!(&:to_sym)
      self.new(**new_node)
    end

    def full_name
      str = "union"
      str << " #{name}" if name
      str << " {#{members.join("; ")};}" if members
      str
    end
  end

  class Enumerator
    attr_reader :name
    attr_reader :val
    def initialize(name:, val: nil)
      @name = name
      @val = val
    end

    def self.from_yaml_ast(node)
      new_node = node.dup
      new_node.transform_keys!(&:to_sym)
      self.new(**new_node)
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
      new_node.delete("kind")
      if new_node["members"]
        new_node["members"] = new_node["members"].collect { |m| Enumerator.from_yaml_ast(m) }
      end
      new_node.transform_keys!(&:to_sym)
      self.new(**new_node)
    end

    def full_name
      str = "enum"
      str << " #{name}" if name
      str << " {#{members.join(", ")}}" if members
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
      new_node.delete("kind")
      if new_node["type"]
        new_node["type"] = Type.from_yaml_ast(new_node["type"])
      end
      new_node.transform_keys!(&:to_sym)
      self.new(**new_node)
    end
  end

  class Pointer < IndirectType
    def to_s(name = nil)
      str = "*"
      str << "const " if const?
      str << "restrict " if restrict?
      str << "volatile " if volatile?
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
    attr_reader :params
    attr_reader :var_args
    attr_reader :name

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
      new_node.delete("kind")
      if new_node["type"]
        new_node["type"] = Type.from_yaml_ast(new_node["type"])
      end
      if new_node["params"]
        new_node["params"] = new_node["params"].collect { |p| Declaration.from_yaml_ast(p) }
      end
      new_node.transform_keys!(&:to_sym)
      self.new(**new_node)
    end

    def to_s(name = nil, no_types = false)
      str = ""
      if params
        if params.empty?
          str << "void"
        else
          if no_types
            str << params.collect(&:name).join(", ")
          else
            str << params.join(", ")
          end
        end
      end
      str << ", ..." if var_args?
      str = "#{name}(#{str})"
      if type
        type.to_s(str)
      else
        str
      end
    end
  end

  KIND_MAP = {
    "int" => Int,
    "void" => Void,
    "float" => Float,
    "char" => Char,
    "bool" => Bool,
    "complex" => Complex,
    "imaginary" => Imaginary,
    "custom_type" => CustomType,
    "struct" => Struct,
    "union" => Union,
    "enum" => Enum,
    "pointer" => Pointer,
    "array" => Array,
    "function" => Function,
    "declaration" => Declaration
  }

  def self.from_yaml_ast(ast)
    res = {}
    ast.each { |k, v|
      case k
      when "typedefs"
        res[k] = v.collect { |d| Declaration.from_yaml_ast(d) }
      when "functions"
        res[k] = v.collect { |d|
          new_d = {}
          new_d["name"] = d["name"]
          new_d["type"] = { "kind" => "function", "type" => d["type"], "params" => d["params"], "var_args" => d["var_args"] }
          new_d["inline"] = d["inline"]
          new_d["storage"] = d["storage"]
          Declaration.from_yaml_ast(new_d)
        }
      else
        res[k] = v.collect { |s| KIND_MAP[k.chomp("s")].from_yaml_ast(s) }
      end
    }
    res
  end

end

def transitive_closure(types, arr)
  sz = arr.size
  loop do
    arr.concat( types.filter_map { |t|
      t.name if t.type.kind_of?(YAMLCAst::CustomType) && arr.include?(t.type.name)
    } ).uniq!
    break if sz == arr.size
    sz = arr.size
  end
  arr
end

def transitive_closure_map(types, map)
  sz = map.size
  loop do
    types.select { |t|
      t.type.kind_of?(YAMLCAst::CustomType) && map.include?(t.type.name)
    }.each { |t| map[t.name] = map[t.type.name] }
    break if sz == map.size
    sz = map.size
  end
end

def find_types(types, cast_type, arr = nil)
  res = types.select { |t| t.type.kind_of? cast_type }.collect { |t| t.name }
  if arr
    arr.concat res
    res = arr
  end
  transitive_closure(types, res)
end

def find_types_map(types, cast_type, map)
  types.select { |t| t.type.kind_of? cast_type }.each { |t|
    map[t.name] = map[t.type.name]
  }
  transitive_closure_map(types, map)
end

# Those are c types and standard library types.
# We don't keep those part of the AST.
# Some are platform dependent. TODO add platform target.

# signed, size, ffi_type
INT_TYPE_MAP = {
  "char" => [false, 1, "ffi_type_sint8"],
  "unsigned char" => [false, 1, "ffi_type_uint8"],
  "short" => [true, 2, "ffi_type_sint16"],
  "unsigned short" => [false, 2, "ffi_type_uint16"],
  "short int" => [true, 2, "ffi_type_sint16"],
  "unsigned short int" => [false, 2, "ffi_type_uint16"],
  "int" => [true, 4, "ffi_type_sint32"],
  "unsigned int" => [false, 4, "ffi_type_uint32"],
  "long" => [true, 8, "ffi_type_sint64"],
  "unsigned long" => [false, 8, "ffi_type_uint64"],
  "long int" => [true, 8, "ffi_type_sint64"],
  "unsigned long int" => [false, 8, "ffi_type_uint64"],
  "long long" => [true, 8, "ffi_type_sint64"],
  "unsigned long long" => [false, 8, "ffi_type_uint64"],
  "long long int" => [true, 8, "ffi_type_sint64"],
  "unsigned long long int" => [false, 8, "ffi_type_uint64"],
  "int8_t" => [true, 1, "ffi_type_sint8"],
  "uint8_t" => [false, 1, "ffi_type_uint8"],
  "int16_t" => [true, 2, "ffi_type_sint16"],
  "uint16_t" => [false, 2, "ffi_type_uint16"],
  "int32_t" => [true, 4, "ffi_type_sint32"],
  "uint32_t" => [false, 4, "ffi_type_uint32"],
  "int64_t" => [true, 8, "ffi_type_sint64"],
  "uint64_t" => [false, 8, "ffi_type_uint64"],
  "ssize_t" => [true, 8, "ffi_type_pointer"],
  "size_t" => [false, 8, "ffi_type_pointer"],
  "intptr_t" => [true, 8, "ffi_type_pointer"],
  "uintptr_t" => [false, 8, "ffi_type_pointer"],
  "_Bool" => [false, 4, "ffi_type_uint32"],
}

INT_SIGN_MAP = INT_TYPE_MAP.map { |k, v| [k, v[0]] }.to_h
INT_SIZE_MAP = INT_TYPE_MAP.map { |k, v| [k, v[1]] }.to_h

FFI_INT_TYPE_MAP = INT_TYPE_MAP.map { |k, v| [k, v[2]] }.to_h
INT_TYPES = INT_TYPE_MAP.keys

HEX_INT_TYPES = [
  "intptr_t",
  "uintptr_t",
]

FFI_FLOAT_TYPE_MAP = {
  "float" => "ffi_type_float",
  "double" => "ffi_type_double",
}
FLOAT_TYPES = FFI_FLOAT_TYPE_MAP.keys

FFI_TYPE_MAP = {}

OBJECT_TYPES = []
ENUM_TYPES = []
STRUCT_TYPES = []
UNION_TYPES = []
POINTER_TYPES = []

def find_all_types(types)
  objs = types.filter_map { |t|
    t.name if t.type.kind_of?(YAMLCAst::Pointer) && t.type.type.kind_of?(YAMLCAst::Struct)
  }
  OBJECT_TYPES.concat objs
  transitive_closure(types, OBJECT_TYPES)

  find_types(types, YAMLCAst::Int, INT_TYPES)
  find_types(types, YAMLCAst::Float, FLOAT_TYPES)
  find_types(types, YAMLCAst::Enum, ENUM_TYPES)
  find_types(types, YAMLCAst::Struct, STRUCT_TYPES)
  find_types(types, YAMLCAst::Union, UNION_TYPES)
  ptrs = types.filter_map { |t|
    t.name if t.type.kind_of?(YAMLCAst::Pointer) && !t.type.type.kind_of?(YAMLCAst::Struct)
  }
  POINTER_TYPES.concat ptrs
end

STRUCT_MAP = {}

def gen_struct_map(types, structs)
  types.select { |t| t.type.kind_of? YAMLCAst::Struct }.each { |t|
    if t.type.members
      STRUCT_MAP[t.name] = t.type.members
    else
      mapped = structs.find { |str| str.name == t.type.name }
      STRUCT_MAP[t.name] = mapped.members if mapped
    end
  }
  transitive_closure_map(types, STRUCT_MAP)
end

def gen_ffi_type_map(types)
  find_types_map(types, YAMLCAst::Int, INT_SIGN_MAP)
  find_types_map(types, YAMLCAst::Int, INT_SIZE_MAP)
  find_types_map(types, YAMLCAst::Int, FFI_INT_TYPE_MAP)
  find_types_map(types, YAMLCAst::Float, FFI_FLOAT_TYPE_MAP)
  FFI_TYPE_MAP.merge!(FFI_INT_TYPE_MAP, FFI_FLOAT_TYPE_MAP)
  OBJECT_TYPES.each { |o|
    FFI_TYPE_MAP[o] = "ffi_type_pointer"
    INT_SIZE_MAP[o] = 8
    INT_SIGN_MAP[o] = false
  }
  # Debatable
  ENUM_TYPES.each { |e|
    FFI_TYPE_MAP[e] = "ffi_type_sint32"
    INT_SIZE_MAP[e] = 4
    INT_SIGN_MAP[e] = true
  }
  POINTER_TYPES.each { |p|
    FFI_TYPE_MAP[p] = "ffi_type_pointer"
    INT_SIZE_MAP[p] = 8
    INT_SIGN_MAP[p] = false
  }
end
