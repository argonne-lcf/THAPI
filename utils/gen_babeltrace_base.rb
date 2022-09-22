module Babeltrace2Gen

  module BTPrinter
    @@output = ""
    @@indent = 0
    INDENT_INCREMENT = "  "

    def pr(str)
      @@output << INDENT_INCREMENT * @@indent << str << "\n"
    end
    module_function :pr

    def scope(&block)
      pr "{"
      @@indent += 1
      block.call
      @@indent -= 1
      pr "}"
    end
    module_function :scope
  end

  module BTLocator
    attr_reader :parent
    attr_reader :variable
    def root
      @parent ? @parent.root : self
    end

    def stream_class
      @parent ? @parent.stream_class : nil
    end

    def trace_class
      @parent ? @parent.trace_class : nil
    end

    def event_class
      @parent ? @parent.event_class : nil
    end

    def find_field_class_path(path, variable)
      path.scan(/\["(\w+)"\]|\[(\d+)\]/).each { |m|
        # String
        if m.first
          pr "#{variable} = bt_field_class_structure_borrow_member_by_name(#{variable}, \"#{m.first}\");"
        else
          pr "#{variable} = bt_field_class_structure_borrow_member_by_index(#{variable}, #{m.last});"
        end
      }
    end

    def find_field_class(path, variable)
      m = path.match(/\A(PACKET_CONTEXT|EVENT_COMMON_CONTEXT|EVENT_SPECIFIC_CONTEXT|EVENT_PAYLOAD)(.*)/)
      case m[1]
      when "PACKET_CONTEXT"
        pr "#{variable} = #{stream_class.packet_context_field_class.variable};"
      when "EVENT_COMMON_CONTEXT"
        pr "#{variable} = #{stream_class.event_common_context_field_class.variable};"
      when "EVENT_SPECIFIC_CONTEXT"
        pr "#{variable} = #{event_class.specific_context_field_class.varible};"
      when "EVENT_PAYLOAD"
        pr "#{variable} = #{event_class.payload_field_class.variable};"
      else
        raise "invalid path #{path}"
      end
      find_field_class_path(m[2], variable)
    end

  end

  class BTTraceClass
    include BTLocator
    include BTPrinter
    attr_reader :stream_classes, :assigns_automatic_stream_class_id

    def initialize(parent:, stream_classes:, assigns_automatic_stream_class_id: nil)
      raise if parent
      @parent = nil
      @assigns_automatic_stream_class_id = assigns_automatic_stream_class_id
      @stream_classes = stream_classes.collect{ |m|
          raise "Incoherent ID scheme" if (m[:id].nil? != (@assigns_automatic_stream_class_id.nil? || @assigns_automatic_stream_class_id))
          BTStreamClass.from_h(self, m)
      }
    end

    def self.from_h(parent, model)
      self.new(parent: parent, **model)
    end

    def trace_class
      self
    end

    def get_declarator(self_component:, variable:)
        pr "bt_trace_class *#{variable} = bt_trace_class_create(#{self_component});"
        unless @assigns_automatic_stream_class_id.nil?
          auto_sc_id = @assigns_automatic_stream_class_id ? "BT_TRUE": "BT_FALSE"
          pr "bt_trace_class_set_assigns_automatic_stream_class_id(#{variable}, #{auto_sc_id});"
        end

        @stream_classes.each_with_index { |m,i|
          stream_class_name = "#{variable}_sc_#{i}"
          scope {
            pr "bt_stream_class *#{stream_class_name};"
            m.get_declarator(trace_class: variable, variable: stream_class_name)
          }
        }
    end

  end

  class BTStreamClass
    include BTLocator
    include BTPrinter
    attr_reader :packet_context_field_class, :event_common_context_field_class, :event_classes, :id, :name
    def initialize(parent:, name: nil, packet_context_field_class: nil, event_common_context_field_class: nil,
                   event_classes: [], id: nil, assigns_automatic_event_class_id: nil, assigns_automatic_stream_id: nil)
      @parent = parent
      @name = name

      raise "Two packet_context" if packet_context_field_class and packet_context
      # Should put assert to check for struct
      @packet_context_field_class = BTFieldClass.from_h(self, packet_context_field_class) if packet_context_field_class

      # Should put assert to check for struct
      @event_common_context_field_class = BTFieldClass.from_h(self, event_common_context_field_class) if event_common_context_field_class

      @assigns_automatic_event_class_id = assigns_automatic_event_class_id
      @event_classes = event_classes.collect { |ec|
                                              raise "Incorect id scheme" if ec[:id].nil? != (@assigns_automatic_event_class_id.nil? || @assigns_automatic_event_class_id.nil)
                                              BTEventClass.from_h(self, ec) }
      @assigns_automatic_stream_id = assigns_automatic_stream_id
      @id = id

    end

    def self.from_h(parent, model)
      self.new(parent: parent, **model)
    end

    def stream_class
      self
    end

    def get_declarator(trace_class:, variable:)
      if @id
        pr "#{variable} = bt_stream_class_create_with_id(#{trace_class}, #{@id});"
      else
        pr "#{variable} = bt_stream_class_create(#{trace_class});"
      end
      pr "bt_stream_class_set_name(#{variable}, \"#{name}\");" if @name

      if @packet_context_field_class
        var_pc = "#{variable}_pc_fc"
        scope {
          pr "bt_field_class *#{var_pc};"
          @packet_context_field_class.get_declarator(trace_class: trace_class, variable: var_pc)
          pr "bt_stream_class_set_packet_context_field_class(#{variable}, #{var_pc});"
        }
      end

      if @event_common_context_field_class
        var_ecc = "#{variable}_ecc_fc"
        scope {
          pr "bt_field_class *#{var_ecc};"
          @event_common_context_field_class.get_declarator(trace_class: trace_class, variable: var_ecc)
          pr "bt_stream_class_set_event_common_context_field_class(#{variable}, #{var_ecc});"
        }
      end
      # Need to do is avec packet and  event_common_context because it can refer members to those
      unless @assigns_automatic_event_class_id.nil?
        auto_id_event_class = @assigns_automatic_event_class_id ? "BT_TRUE" : "BT_FALSE"
        pr "bt_stream_class_set_assigns_automatic_event_class_id(#{variable}, #{auto_id_event_class});"
      end

      @event_classes.each_with_index { |ec,i|
        var_name = "#{variable}_ec_#{i}"
        scope {
          pr "bt_event_class *#{var_name};"
          ec.get_declarator(trace_class: trace_class, variable: var_name, stream_class: variable)
        }
      }

      unless @assigns_automatic_stream_id.nil?
        auto_id_stream = @assigns_automatic_stream_id ? "BT_TRUE" : "BT_FALSE"
        pr "bt_stream_class_set_assigns_automatic_stream_id(#{variable}, #{auto_id_stream});"
      end
    end
  end

  class BTEventClass
    include BTLocator
    include BTPrinter
    attr_reader :name, :specific_context_field_class, :payload_field_class
    def initialize(parent:, name: nil, specific_context_field_class: nil, payload_field_class: nil, id: nil)
      @parent = parent
      @name = name
      @specific_context_field_class = BTFieldClass.from_h(self, specific_context_field_class) if specific_context_field_class
      @payload_field_class = BTFieldClass.from_h(self, payload_field_class) if payload_field_class

      @id = id
    end

    def self.from_h(parent, model)
      self.new(parent: parent, **model)
    end

    def get_declarator(trace_class:, variable:, stream_class:)
      # Store the variable name for instrocption purpose for LOCATION_PATH
      @variable = variable
      if @id
        pr "#{variable} = bt_event_class_create_with_id(#{stream_class}, #{@id});"
      else
        pr "#{variable} = bt_event_class_create(#{stream_class});"
      end

      pr "bt_event_class_set_name(#{variable}, \"#{@name}\");" if @name

      if @specific_context_field_class
        var_name = "#{variable}_sc_fc"
        scope {
          pr "bt_field_class *#{var_name};"
          @specific_context_field_class.get_declarator(trace_class: trace_class, variable: var_name)
          pr "bt_event_class_set_specific_context_field_class(#{variable}, #{var_name});"
        }
      end

      if @payload_field_class
        var_name = "#{variable}_p_fc"
        scope {
          pr "bt_field_class *#{var_name};"
          @payload_field_class.get_declarator(trace_class: trace_class, variable: var_name)
          pr "bt_event_class_set_payload_field_class(#{variable}, #{var_name});"
        }
      end
    end

    def event_class
      self
    end
  end

  class BTFieldClass
    include BTLocator
    include BTPrinter

    attr_accessor :cast_type
    def initialize(parent:)
      @parent = parent
    end

    def self.from_h(parent, model)
      key = model.delete(:type)
      raise "No type in #{model}" unless key
      raise "No #{key} in FIELD_CLASS_NAME_MAP" unless FIELD_CLASS_NAME_MAP.include?(key)

      cast_type = model.delete(:cast_type)
      fc = FIELD_CLASS_NAME_MAP[key].from_h(parent, model)
      fc.cast_type = cast_type if cast_type
      fc
    end

    def get_declarator(*args, **dict)
      raise NotImplementedError, self.class
    end

  end

  class BTFieldClass::Bool < BTFieldClass
    def self.from_h(parent, model)
      self.new(parent: parent)
    end

    def get_declarator(trace_class:, variable:)
      pr "#{variable} = bt_field_class_bool_create(#{trace_class});"
    end
  end
  BTFieldClassBool = BTFieldClass::Bool

  class BTFieldClass::BitArray < BTFieldClass
    attr_reader :length

    def initialize(parent:, length:)
      @parent = parent
      @length = length
    end

    def self.from_h(parent, model)
      self.new(parent: parent, length: model[:length])
    end

    def get_declarator(trace_class:, variable:)
      pr "#{variable} = bt_field_class_bit_array_create(#{trace_class}, #{@length});"
    end

  end
  BTFieldClassBitArray = BTFieldClass::BitArray

  class BTFieldClass::Integer < BTFieldClass
    attr_reader :field_value_range, :preferred_display_base

    def initialize(parent:, field_value_range: nil, preferred_display_base: nil)
      @parent = parent
      @field_value_range = field_value_range
      @preferred_display_base = preferred_display_base
    end

    def self.from_h(parent, model)
      self.new(parent: parent, **model)
    end

    def get_declarator(trace_class: nil, variable:)
      pr "bt_field_class_integer_set_field_value_range(#{variable}, #{@field_value_range});" if @field_value_range
      pr "bt_field_class_integer_set_preferred_display_base(#{variable}, #{@preferred_display_base});" if @preferred_display_base
    end
  end
  BTFieldClassInteger = BTFieldClass::Integer

  class BTFieldClass::Integer::Unsigned < BTFieldClassInteger
    def get_declarator(trace_class:, variable:)
      pr "#{variable} = bt_field_class_integer_unsigned_create(#{trace_class});"
      super
    end
  end
  BTFieldClass::IntegerUnsigned = BTFieldClass::Integer::Unsigned
  BTFieldClassIntegerUnsigned = BTFieldClass::Integer::Unsigned

  class BTFieldClass::Integer::Signed < BTFieldClassInteger
    def get_declarator(trace_class:, variable:)
      pr "#{variable} = bt_field_class_integer_signed_create(#{trace_class});"
      super
    end
  end
  BTFieldClass::IntegerSigned = BTFieldClass::Integer::Signed
  BTFieldClassIntegerSigned = BTFieldClass::Integer::Signed

  class BTFieldClass::Real < BTFieldClass
    def self.from_h(parent, model)
      self.new(parent: parent)
    end
  end
  BTFieldClassReal = BTFieldClass::Real

  class BTFieldClass::Real::SinglePrecision < BTFieldClassReal
    def get_declarator(trace_class:, variable:)
      pr "#{variable} = bt_field_class_real_single_precision_create(#{trace_class});"
    end
  end
  BTFieldClass::RealSinglePrecision = BTFieldClass::Real::SinglePrecision
  BTFieldClassRealSinglePrecision = BTFieldClass::Real::SinglePrecision

  class BTFieldClass::Real::DoublePrecision < BTFieldClassReal
    def get_declarator(trace_class:, variable:)
      pr "#{variable} = bt_field_class_real_double_precision_create(#{trace_class});"
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

    def initialize(parent:, field_value_range:, preferred_display_base: 10, mappings:)
      @mappings = mappings #TODO init Mapping
      super(parent: parent, field_value_range: field_value_range, preferred_display_base: preferred_display_base)
    end

    def self.from_h(parent, model)
      self.new(parent: parent, **model)
    end
  end
  BTFieldClassEnumerationUnsigned = BTFieldClass::Enumeration::Unsigned
  BTFieldClassEnumerationUnsignedMapping = BTFieldClass::Enumeration::Unsigned::Mapping

  class BTFieldClass::Enumeration::Signed < BTFieldClass::Integer::Signed
    include BTFieldClass::Enumeration
    class Mapping < BTFieldClass::Enumeration::Mapping
    end

    def initialize(parent:, field_value_range:, preferred_display_base: 10, mappings:)
      @mappings = mappings #TODO init Mapping
      super(parent: parent, field_value_range: field_value_range, preferred_display_base: preferred_display_base)
    end

    def self.from_h(parent, model)
      self.new(parent: parent, **model)
    end
  end
  BTFieldClassEnumerationSigned = BTFieldClass::Enumeration::Signed
  BTFieldClassEnumerationSignedMapping = BTFieldClass::Enumeration::Signed::Mapping

  class BTFieldClass::String < BTFieldClass
    def self.from_h(parent, model)
      self.new(parent: parent)
    end

    def get_declarator(trace_class:, variable:)
      pr "#{variable} = bt_field_class_string_create(#{trace_class});"
    end

  end
  BTFieldClassString = BTFieldClass::String

  class BTFieldClass::Array < BTFieldClass
    attr_reader :element_field_class
    def initialize(parent:, element_field_class:)
      @parent = parent
      @element_field_class = BTFieldClass.from_h(self, element_field_class)
    end
  end
  BTFieldClassArray = BTFieldClass::Array

  class BTFieldClass::Array::Static < BTFieldClass::Array
    attr_reader :length

    def initialize(parent:, element_field_class:, length:)
      @length = length
      super(parent: parent, element_field_class: element_field_class)
    end

    def self.from_h(parent, model)
      self.new(parent: parent, element_field_class: model[:field], length: model[:length])
    end

    def get_declarator(trace_class:, variable:)
      element_field_class_variable = "#{variable}_field_class"
      scope {
        pr "bt_field_class *#{element_field_class_variable};"
        @element_field_class.get_declarator(trace_class: trace_class, variable: element_field_class_variable)
        pr "#{variable} = bt_field_class_array_static_create(#{trace_class}, #{element_field_class_variable}, #{@length});"
      }
    end
  end
  BTFieldClassArrayStatic = BTFieldClass::Array::Static

  class BTFieldClass::Array::Dynamic < BTFieldClass::Array
    module WithLengthField
      attr_reader :length_field_path
    end

    def initialize(parent:, element_field_class:, length_field_path: nil)
      super(parent: parent, element_field_class: element_field_class)
      if length_field_path
        self.extend(WithLengthField)
        @length_field_path = length_field_path
      end
    end

    def self.from_h(parent, model)
      self.new(parent: parent, element_field_class: model[:field], length_field_path: model[:length_field_path])
    end

    def get_declarator(trace_class:, variable:)
      element_field_class_variable= "#{variable}_field_class"
      scope {
        pr "bt_field_class *#{element_field_class_variable};"
        @element_field_class.get_declarator(trace_class: trace_class, variable: element_field_class_variable)
        if @length_field_path
          element_field_class_variable_length = "#{element_field_class_variable}_length"
          pr "bt_field_class *#{element_field_class_variable_length};"
          find_field_class(@length_field_path, element_field_class_variable_length)
          pr "#{variable} = bt_field_class_array_dynamic_create(#{trace_class}, #{element_field_class_variable}, #{element_field_class_variable_length});"
        else
          pr "#{variable} = bt_field_class_array_dynamic_create(#{trace_class}, #{element_field_class_variable}, NULL);"
        end
      }
    end

  end
  BTFieldClassArrayDynamic = BTFieldClass::Array::Dynamic
  BTFieldClassArrayDynamicWithLengthField = BTFieldClass::Array::Dynamic::WithLengthField
  class BTFieldClass::Structure < BTFieldClass
    attr_reader :members
    class Member
      include BTLocator
      attr_reader :parent
      attr_reader :name, :field_class

      def initialize(parent:, name:, field_class:)
        @parent = parent
        @name = name
        @field_class = BTFieldClass.from_h(self, field_class)
      end
    end

    def initialize(parent:, members: [])
      @parent = parent
      @members = members.collect { |m| Member.new(parent: self, **m) }
    end

    def [](index)
      case index
      when Integer
        @members[index]
      when String
        @members.find { |m| m.name == index }
      end
    end

    def self.from_h(parent, model)
      self.new(parent: parent, **model)
    end

    def get_declarator(trace_class:, variable:)
        @variable = variable
        pr "#{variable} = bt_field_class_structure_create(#{trace_class});"
        @members.each_with_index { |m, i|
         var_name = "#{variable}_m_#{i}"
          scope {
            pr "bt_field_class *#{var_name};"
            m.field_class.get_declarator(trace_class: trace_class, variable: var_name)
            pr "bt_field_class_structure_append_member(#{variable}, \"#{m.name}\", #{var_name});"
          }
        }
    end
  end
  BTFieldClassStructure = BTFieldClass::Structure
  BTFieldClassStructureMember = BTFieldClass::Structure::Member
  class BTFieldClass::Option < BTFieldClass
    attr_reader :field_class

    def initialize(parent:, field_class:)
      @parent = parent
      @field_class = BTFieldClass.from_h(self, field_class)
    end
  end
  BTFieldClassOption = BTFieldClass::Option

  class BTFieldClass::Option::WithoutSelectorField < BTFieldClass::Option
    def self.from_h(parent, model)
      self.new(parent: parent, field_class: model[:field])
    end
  end
  BTFieldClassOptionWithoutSelectorField = BTFieldClass::Option::WithoutSelectorField

  class BTFieldClass::Option::WithSelectorField < BTFieldClass::Option
    attr_reader :selector_field_path

    def initialize(parent:, field_class:, selector_field_path:)
      @selector_field_path = selector_field_path
      super(parent: parent, field_class: field_class)
    end
  end
  BTFieldClassOptionWithSelectorField = BTFieldClass::Option::WithSelectorField

  class BTFieldClass::Option::WithSelectorField::Bool < BTFieldClass::Option::WithSelectorField
    attr_reader :selector_is_reversed

    def initialize(parent:, field_class:, selector_field_path:, selector_is_reversed: nil)
      @selector_is_reversed = selector_is_reversed
      super(parent: parent, field_class: field_class, selector_field_path: selector_field_path)
    end

    def self.from_h(parent, model)
      self.new(parent: parent, field_class: model[:field], selector_field_path: model[:selector_field_path], selector_is_reversed: [selector_field_path])
    end
  end
  BTFieldClassOptionWithSelectorFieldBool = BTFieldClass::Option::WithSelectorField::Bool

  class BTFieldClass::Option::WithSelectorField::IntegerUnsigned < BTFieldClass::Option::WithSelectorField
    attr_reader :selector_ranges

    def initialize(parent:, field_class:, selector_field_path:, selector_ranges:)
      @selector_ranges = selector_ranges
      super(parent: parent, field_class: field_class, selector_field_path: selector_field_path)
    end

    def self.from_h(parent, model)
      self.new(parent: parent, field_class: model[:field], selector_field_path: model[:selector_field_path], selector_ranges: model[:selector_ranges])
    end
  end
  BTFieldClassOptionWithSelectorFieldIntegerUnsigned = BTFieldClass::Option::WithSelectorField::IntegerUnsigned

  class BTFieldClass::Option::WithSelectorField::IntegerSigned < BTFieldClass::Option::WithSelectorField
    attr_reader :selector_ranges

    def initialize(parent:, field_class:, selector_field_path:, selector_ranges:)
      @selector_ranges = selector_ranges
      super(parent: parent, field_class: field_class, selector_field_path: selector_field_path)
    end

    def self.from_h(parent, model)
      self.new(parent: parent, field_class: model[:field], selector_field_path: model[:selector_field_path], selector_ranges: model[:selector_ranges])
    end
  end
  BTFieldClassOptionWithSelectorFieldIntegerSigned = BTFieldClass::Option::WithSelectorField::IntegerSigned

  class BTFieldClass::Variant < BTFieldClass
    attr_reader :options

    class Option
      attr_reader :name, :field_class
      def initialize(parent:, name:, field_class:)
        @parent = parent
        @name = name
        @field_class = BTFieldClass.from_h(self, field_class)
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
        def initialize(parent:, name:, field_class:, ranges:)
          @ranges = ranges
          super(parent: parent, name: name, field_class: field_class)
        end
      end
    end

    def initialize(parent:, options:, selector_field_class: nil)
      @parent = parent
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
    "integer_unsigned" => BTFieldClass::Integer::Unsigned,
    "integer_signed" => BTFieldClass::Integer::Signed,
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
