require_relative 'extract_base'

omp_header = <<~EOF
  #include <omp-tools.h>
EOF

if enable_clang_parser?
  require 'open3'
  header = [shared_header, omp_header].join("\n")
  yaml, status = Open3.capture2('h2yaml --compat-cast-to-yaml -Wc,-xc -Wc,-Imodified_include/ -', stdin_data: header)
  exit(1) unless status.success?

else
  preprocessed_sources_omp_api = $cpp.preprocess(omp_header).gsub(/^#.*?$/, '')
  ast = $parser.parse(preprocessed_sources_omp_api)
  yaml = ast.extract_declarations.to_yaml
end

File.open('ompt_api.yaml', 'w') do |f|
  f.puts yaml
end
