require_relative 'extract_base'

omp_header = <<~EOF
  #include <omp-tools.h>
EOF

if enable_clang_parser?
  require 'open3'
  yaml, = Open3.capture2('h2yaml -xc -I modified_include/ -', stdin_data: omp_header)
else
  preprocessed_sources_omp_api = $cpp.preprocess(omp_header).gsub(/^#.*?$/, '')
  ast = $parser.parse(preprocessed_sources_omp_api)
  yaml = ast.extract_declarations.to_yaml
end

File.open('ompt_api.yaml', 'w') do |f|
  f.puts yaml
end
