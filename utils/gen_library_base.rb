require_relative 'yaml_ast'

def to_ffi_name(name)
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
  when '_Bool'
    return ':bool'
  end
  name.to_sym.inspect
end

module YAMLCAst
  module SUFFIConvertible
    def to_ffi
      unamed_count = 0
      members.map do |m|
        mt = if m.type.is_a?(Array)
               m.type.to_ffi
             elsif m.type.is_a?(Pointer)
               ':pointer'
             elsif m.type.name
               to_ffi_name(m.type.name)
             elsif m.type.is_a?(Struct)
               "(Class::new(#{FFI_STRUCT}) { layout #{gen_layout(m.type.to_ffi)} }.by_value)"
             elsif m.type.is_a?(Union)
               "(Class::new(#{FFI_UNION}) { layout #{gen_layout(m.type.to_ffi)} }.by_value)"
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
    include SUFFIConvertible
  end

  class Union
    include SUFFIConvertible
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
