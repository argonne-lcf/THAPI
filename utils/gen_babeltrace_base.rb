module Babeltrace2Gen

  class BTFieldClass
    def self.from_h(model)
      FIELD_CLASS_NAME_MAP[model[:class]].from_h(model)
    end

    def get_declarator
      raise NotImplementedError
    end
  end

  class BTFieldClass::Bool < BTFieldClass
    def self.from_h(model)
      self.new
    end

    def get_declarator(trace_class:, variable:, indent: 0)
      [ "#{' '*indent}#{variable} = bt_field_class_bool_create(#{trace_class});" ]
    end
  end
  BTFieldClassBool = BTFieldClass::Bool

  class BTFieldClass::BitArray < BTFieldClass
    attr_reader :length

    def intialize(length:)
      @length = length
    end

    def self.from_h(model)
      self.new(length: model[:length])
    end

    def get_declarator(trace_class:, variable:, indent: 0)
      [ "#{' '*indent}#{variable} = bt_field_class_bit_array_create(#{trace_class}, #{@length});" ]
    end

  end
  BTFieldClassBitArray = BTFieldClass::BitArray

  class BTFieldClass::Integer < BTFieldClass
    attr_reader :field_value_range, :preferred_display_base

    def initialize(field_value_range: 64, preferred_display_base: 10)
      @field_value_range = field_value_range
      @preferred_display_base = preferred_display_base
    end

    def self.from_h(model)
      self.new(**model[:class_properties])
    end

    def get_declarator(trace_class: nil, variable:, indent: 0)
      indent_str = ' '*indent
      a = []
      a << "#{indent_str}bt_field_class_integer_set_field_value_range(#{variable}, #{@field_value_range});" if @field_value_range != 64
      a << "#{indent_str}bt_field_class_integer_set_preferred_display_base(#{variable}, #{@preferred_display_base});" if @preferred_display_base != 10
    end
  end
  BTFieldClassInteger = BTFieldClass::Integer

  class BTFieldClass::Integer::Unsigned < BTFieldClassInteger
    def get_declarator(trace_class:, variable:, indent: 0)
      [ "#{' '*indent}#{variable} = bt_field_class_integer_unsigned_create(#{trace_class});" ] + super
    end
  end
  BTFieldClass::IntegerUnsigned = BTFieldClass::Integer::Unsigned
  BTFieldClassIntegerUnsigned = BTFieldClass::Integer::Unsigned

  class BTFieldClass::Integer::Signed < BTFieldClassInteger
    def get_declarator(trace_class:, variable:, indent: 0)
      [ "#{' '*indent}#{variable} = bt_field_class_integer_signed_create(#{trace_class});" ] + super
    end
  end
  BTFieldClass::IntegerSigned = BTFieldClass::Integer::Signed
  BTFieldClassIntegerSigned = BTFieldClass::Integer::Signed

  class BTFieldClass::Real < BTFieldClass
    def self.from_h(model)
      self.new
    end
  end
  BTFieldClassReal = BTFieldClass::Real

  class BTFieldClass::Real::SinglePrecision < BTFieldClassReal
    def get_declarator(trace_class:, variable:, indent: 0)
      [ "#{' '*indent}#{variable} = bt_field_class_real_single_precision_create(#{trace_class});" ] 
    end
  end
  BTFieldClass::RealSinglePrecision = BTFieldClass::Real::SinglePrecision
  BTFieldClassRealSinglePrecision = BTFieldClass::Real::SinglePrecision

  class BTFieldClass::Real::DoublePrecision < BTFieldClassReal
    def get_declarator(trace_class:, variable:, indent: 0)
      [ "#{' '*indent}#{variable} = bt_field_class_real_double_precision_create(#{trace_class});" ]
    end
  end
  BTFieldClass::RealDoublePrecision = BTFieldClass::Real::DoublePrecision
  BTFieldClassRealDoublePrecision = BTFieldClass::Real::DoublePrecision

  module BTFieldClass::Enumeration
    attr_reader :mappings
    class Mapping
    end
  end
  BTFieldClassEnumeration = BTFieldClass::Enumeration
  BTFieldClassEnumerationMapping = BTFieldClass::Enumeration::Mapping

  class BTFieldClass::Enumeration::Unsigned < BTFieldClass::Integer::Unsigned
    include BTFieldClass::Enumeration
    class Mapping < BTFieldClass::Enumeration::Mapping
    end

    def initialize(field_value_range:, preferred_display_base: 10, mappings:)
      @mappings = mappings #TODO init Mapping
      super(field_value_range: field_value_range, preferred_display_base: preferred_display_base)
    end

    def self.from_h(model)
      self.new(**model)
    end
  end
  BTFieldClassEnumerationUnsigned = BTFieldClass::Enumeration::Unsigned
  BTFieldClassEnumerationUnsignedMapping = BTFieldClass::Enumeration::Unsigned::Mapping

  class BTFieldClass::Enumeration::Signed < BTFieldClass::Integer::Signed
    include BTFieldClass::Enumeration
    class Mapping < BTFieldClass::Enumeration::Mapping
    end

    def initialize(field_value_range:, preferred_display_base: 10, mappings:)
      @mappings = mappings #TODO init Mapping
      super(field_value_range: field_value_range, preferred_display_base: preferred_display_base)
    end

    def self.from_h(model)
      self.new(**model)
    end
  end
  BTFieldClassEnumerationSigned = BTFieldClass::Enumeration::Signed
  BTFieldClassEnumerationSignedMapping = BTFieldClass::Enumeration::Signed::Mapping

  class BTFieldClass::String < BTFieldClass
    def self.from_h(model)
      self.new
    end
  end
  BTFieldClassString = BTFieldClass::String

  class BTFieldClass::Array < BTFieldClass
    attr_reader :element_field_class
    def initialize(element_field_class:)
      @element_field_class = BTFieldClass.from_h(element_field_class)
    end
  end
  BTFieldClassArray = BTFieldClass::Array

  class BTFieldClass::Array::Static < BTFieldClass::Array
    attr_reader :length

    def initialize(element_field_class:, length:)
      @length = length
      super(element_field_class: element_field_class)
    end

    def self.from_h(model)
      self.new(element_field_class: model[:field], length: model[:length])
    end

    def get_declarator(trace_class:, variable:, indent: 0)
      a = ["#{' '*(indent)}{" ]
      indent += 2

      indent_str = ' '*(indent);
      element_field_class_variable= "#{variable}_field_class";
      a << "#{indent_str}bt_field_class *#{element_field_class_variable};"
      a += @element_field_class.get_declarator(trace_class: trace_class, variable: element_field_class_variable, indent: indent)
      a << "#{indent_str}#{variable} = bt_field_class_array_static_create(#{trace_class}, #{element_field_class_variable}, #{@length});"

      indent -= 2
      a << "#{' '*(indent)}}"
    end
  end
  BTFieldClassArrayStatic = BTFieldClass::Array::Static

  class BTFieldClass::Array::Dynamic < BTFieldClass::Array
    module WithLengthField
      attr_reader :length_field_path
    end

    def initialize(element_field_class:, length_field_path: nil)
      super(element_field_class: element_field_class)
      if length_field_path
        self.extend(WithLengthField)
        @length_field_path = length_field_path
      end
    end

    def self.from_h(model)
      self.new(element_field_class: model[:field], length_field_path: model[:length_field_path])
    end
  end
  BTFieldClassArrayDynamic = BTFieldClass::Array::Dynamic
  BTFieldClassArrayDynamicWithLengthField = BTFieldClass::Array::Dynamic::WithLengthField

  class BTFieldClass::Structure < BTFieldClass
    attr_reader :members

    class Member
      attr_reader :name, :field_class

      def initialize(name:, field_class:)
        @name = name
        @field_class = BTFieldClass.from_h(field_class)
      end
    end
    
    def initialize(members:)
      @members = members.collect { |m| Member.new(name: m[:name], field_class: m) }
    end

    def self.from_h(model)
      self.new(members: model[:members])
    end
  end
  BTFieldClassStructure = BTFieldClass::Structure
  BTFieldClassStructureMember = BTFieldClass::Structure::Member

  class BTFieldClass::Option < BTFieldClass
    attr_reader :field_class

    def initialize(field_class:)
      @field_class = BTFieldClass.from_h(field_class)
    end
  end
  BTFieldClassOption = BTFieldClass::Option

  class BTFieldClass::Option::WithoutSelectorField < BTFieldClass::Option
    def self.from_h(model)
      self.new(field_class: model[:field])
    end
  end
  BTFieldClassOptionWithoutSelectorField = BTFieldClass::Option::WithoutSelectorField

  class BTFieldClass::Option::WithSelectorField < BTFieldClass::Option
    attr_reader :selector_field_path

    def initialize(field_class:, selector_field_path:)
      @selector_field_path = selector_field_path
      super(field_class: field_class)
    end
  end
  BTFieldClassOptionWithSelectorField = BTFieldClass::Option::WithSelectorField

  class BTFieldClass::Option::WithSelectorField::Bool < BTFieldClass::Option::WithSelectorField
    attr_reader :selector_is_reversed

    def initialize(field_class:, selector_field_path:, selector_is_reversed: nil)
      @selector_is_reversed = selector_is_reversed
      super(field_class: field_class, selector_field_path: selector_field_path)
    end

    def self.from_h(model)
      self.new(field_class: model[:field], selector_field_path: model[:selector_field_path], selector_is_reversed: [selector_field_path])
    end
  end
  BTFieldClassOptionWithSelectorFieldBool = BTFieldClass::Option::WithSelectorField::Bool

  class BTFieldClass::Option::WithSelectorField::IntegerUnsigned < BTFieldClass::Option::WithSelectorField
    attr_reader :selector_ranges

    def initialize(field_class:, selector_field_path:, selector_ranges:)
      @selector_ranges = selector_ranges
      super(field_class: field_class, selector_field_path: selector_field_path)
    end

    def self.from_h(model)
      self.new(field_class: model[:field], selector_field_path: model[:selector_field_path], selector_ranges: model[:selector_ranges])
    end
  end
  BTFieldClassOptionWithSelectorFieldIntegerUnsigned = BTFieldClass::Option::WithSelectorField::IntegerUnsigned

  class BTFieldClass::Option::WithSelectorField::IntegerSigned < BTFieldClass::Option::WithSelectorField
    attr_reader :selector_ranges

    def initialize(field_class:, selector_field_path:, selector_ranges:)
      @selector_ranges = selector_ranges
      super(field_class: field_class, selector_field_path: selector_field_path)
    end

    def self.from_h(model)
      self.new(field_class: model[:field], selector_field_path: model[:selector_field_path], selector_ranges: model[:selector_ranges])
    end
  end
  BTFieldClassOptionWithSelectorFieldIntegerSigned = BTFieldClass::Option::WithSelectorField::IntegerSigned

  class BTFieldClass::Variant < BTFieldClass
    attr_reader :options

    class Option
      attr_reader :name, :field_class
      def initialize(name:, field_class:)
        @name = name
        @field_class = BTFieldClass.from_h(field_class)
      end
    end

  end
  BTFieldClassVariant = BTFieldClass::Variant
  BTFieldClassVariantOption = BTFieldClass::Variant::Option

  class BTFieldClass::Variant
    module WithoutSelectorField
    end
    module WithSelectorField
      attr_reader :selector_field_class
      class Option < BTFieldClassVariantOption
        attr_reader :ranges
        def initialize(name:, field_class:, ranges:)
          @ranges = ranges
          super(name: name, field_class: field_class)
        end
      end
    end

    def initialize(options:, selector_field_class: nil)
      if selector_field_class
        self.extend(WithSelectorField)
        @selector_field_class = selector_field_class
        @options = options.collect { |o|
          BTFieldClass::Variant::WithSelectorField::Option.new(name: o[:name], field_class: o[:field], range: o[:range])
        }
      else
        self.extend(WithoutSelectorField)
        @options = options.collect { |o|
          BTFieldClass::Variant::Option.new(name: o[:name], field_class: o[:field])
        }
      end
    end
  end
  BTFieldClassVariantWithoutSelectorField =
    BTFieldClass::Variant::WithoutSelectorField
  BTFieldClassVariantWithSelectorField =
    BTFieldClass::Variant::WithSelectorField
  BTFieldClassVariantWithSelectorFieldOption =
    BTFieldClass::Variant::WithSelectorField::Option

  FIELD_CLASS_NAME_MAP = {
    "bool" => BTFieldClass::Bool,
    "bit_array" => BTFieldClass::BitArray,
    "unsigned" => BTFieldClass::Integer::Unsigned,
    "signed" => BTFieldClass::Integer::Signed,
    "single" => BTFieldClass::Real::SinglePrecision,
    "double" => BTFieldClass::Real::DoublePrecision,
    "enumeration_unsigned" => BTFieldClass::Enumeration::Unsigned,
    "enumeration_signed" => BTFieldClass::Enumeration::Signed,
    "string" => BTFieldClass::String,
    "array_static" => BTFieldClass::Array::Static,
    "array_dynamic" => BTFieldClass::Array::Dynamic,
    "structure" => BTFieldClass::Structure,
    "option_without_selector_field" => BTFieldClass::Option::WithoutSelectorField,
    "option_with_selector_field_bool" => BTFieldClass::Option::WithSelectorField::Bool,
    "option_with_selector_field_unsigned" => BTFieldClass::Option::WithSelectorField::IntegerUnsigned,
    "option_with_selector_field_signed" => BTFieldClass::Option::WithSelectorField::IntegerSigned,
    "variant" => BTFieldClass::Variant
  }

end
