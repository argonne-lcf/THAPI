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
end
EOF
