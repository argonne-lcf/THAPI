require_relative '../../utils/yaml_ast'
SRC_DIR = ENV.fetch('SRC_DIR', nil)
export_tables = yaml_load_file_cached(File.join(SRC_DIR, 'cuda_export_tables.yaml'))

puts <<~EOF
  #ifndef _CUDA_EXPORTS_H_INCLUDE
  #define _CUDA_EXPORTS_H_INCLUDE

EOF

export_tables.each do |table|
  next unless table['structures']

  table['structures'].each do |struct|
    puts <<~EOF
      typedef #{struct['declaration'].chomp} #{struct['name']};

    EOF
  end
end

puts <<~EOF
  #endif
EOF
