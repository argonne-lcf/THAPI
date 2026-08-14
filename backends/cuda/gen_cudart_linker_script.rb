require_relative 'cudart_model'

puts <<~EOF
  #{ARGV[0]} {
    global:
      #{COMMANDS.collect(&:name).join(";\n    ")};
    local:
      *;
  };
EOF
