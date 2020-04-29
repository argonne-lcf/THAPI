require_relative 'gen_ze_library_base.rb'


puts <<EOF
require_relative 'ze_library.rb'
require_relative 'ze_refinements.rb'
require_relative 'ze_bindings_base.rb'

module ZE

EOF

$all_funcs.each { |f|
  type, params = f.type.to_ffi
  puts <<EOF
  attach_function #{to_ffi_name(f.name)},
                  [ #{params.join(",\n"+" "*20)} ],
                  #{type}

EOF
}

puts <<EOF
  ZE.init

  at_exit {
    ZE::ZE_OBJECTS_MUTEX.synchronize {
      ZE::ZE_OBJECTS.to_a.reverse.each do |h, d|
        result = method(d).call(h)
        ZE.error_check(result)
      end
      ZE::ZE_OBJECTS.clear
    }
  }

end
EOF
