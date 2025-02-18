require_relative './yaml_ast'

module YAMLCAst
  class Struct
    def to_ffi
      unamed_count = 0
      res = []
      members.each { |m|
        mt = case m.type
        when Array
          m.type.to_ffi
        when Pointer
          ":pointer"
        else
          if !m.type.name
            print_lambda = lambda { |m|
              s = "#{m[0]}, "
              if m[1].kind_of?(::Array)
                s << "[ #{m[1][0]}, #{m[1][1]} ]"
              else
                s << "#{m[1]}"
              end
              s
            }
            case m.type
            when Struct
              membs = m.type.to_ffi
              "(Class::new(#{FFI_STRUCT}) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
            when Union
              membs = m.type.to_ffi
              "(Class::new(#{FFI_UNION}) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
            else
              raise "Error type unknown!"
            end
          else
            to_ffi_name(m.type.name)
          end
        end
        res.push [m.name ? m.name.to_sym.inspect : ":_unamed_#{unamed_count}", mt]
      }
      res
    end
  end

  class Union
    def to_ffi
      unamed_count = 0
      res = []
      members.each { |m|
        mt = case m.type
        when Array
          m.type.to_ffi
        when Pointer
          ":pointer"
        else
          if !m.type.name
            print_lambda = lambda { |m|
              s = "#{m[0]}, "
              if m[1].kind_of?(::Array)
                s << "[ #{m[1][0]}, #{m[1][1]} ]"
              else
                s << "#{m[1]}"
              end
              s
            }
            case m.type
            when Struct
              membs = m.type.to_ffi
              "(Class::new(#{FFI_STRUCT}) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
            when Union
              membs = m.type.to_ffi
              "(Class::new(#{FFI_UNION}) { layout #{membs.collect(&print_lambda).join(", ")} }.by_value)"
            else
              raise "Error type unknown!"
            end
          else
            to_ffi_name(m.type.name)
          end
        end
        res.push [m.name ? m.name.to_sym.inspect : ":_unamed_#{unamed_count}", mt]
      }
      res
    end
  end

  class Array
    def to_ffi
      t = case type
      when Pointer
        ":pointer"
      else
       to_ffi_name(type.name)
      end
      [ t, length ]
    end
  end

  class Function
    def to_ffi
      if type.respond_to?(:name)
        t = to_ffi_name(type.name)
      elsif type.kind_of?(Pointer)
        t = ":pointer"
      else
        raise "unknown return type: #{type}"
      end
      p = if params
        params.collect { |par|
          if par.type.kind_of?(Pointer)
            if par.type.type.respond_to?(:name) &&
              $all_struct_names.include?(par.type.type.name)
              "#{to_class_name(par.type.type.name)}.ptr"
            else
              ":pointer"
            end
          else
            to_ffi_name(par.type.name)
          end
        }
      else
        []
      end
      [t, p]
    end
  end

end
