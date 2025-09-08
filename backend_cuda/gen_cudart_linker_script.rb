require_relative 'cudart_model'

puts <<EOF
#{ARGV[0]} {
  global:
    #{$cudart_commands.collect(&:name).join(";\n    ")};
  local:
    *;
};
EOF
