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

