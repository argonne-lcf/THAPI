require_relative 'extract_base'

itt_header = <<~EOF
  #include "include/ittnotify.h"
EOF

yaml = nil

if enable_clang_parser?
  header = [shared_header, itt_header].join("\n")
  require 'open3'
  yaml, status = Open3.capture2('h2yaml -c -Wc,-xc -', stdin_data: header)
  exit(1) unless status.success?
else
  preprocessed_sources_itt_api = $cpp.preprocess(itt_header).gsub(/^#.*?$/, '')
  ast = $parser.parse(preprocessed_sources_itt_api)
  yaml = ast.extract_declarations.to_yaml
end

File.open('itt_api.yaml', 'w') do |f|
  f.puts yaml
end
