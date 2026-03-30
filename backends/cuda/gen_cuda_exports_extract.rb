require 'yaml'

SRC_DIR = ENV['SRC_DIR'] || '.'
export_tables = YAML.load_file(File.join(SRC_DIR, 'cuda_export_tables.yaml'))

puts <<~EOF
  #include <stdlib.h>
  #include <stdint.h>
  #include <limits.h>
  #include <stddef.h>
  #define __CUDA_API_VERSION_INTERNAL=1
  #define THAPI_NO_INCLUDE
  #include <cuda.h>
EOF

export_tables.each do |table|
  if table['structures']
    table['structures'].each do |struct|
      puts <<~EOF
          typedef
        #{struct['declaration']}
          #{struct['name']};
      EOF
    end
  end
  table['functions'].each do |function|
    puts <<~EOF
      #{function['declaration']};
    EOF
  end
end
