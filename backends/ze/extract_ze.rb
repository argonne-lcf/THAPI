require_relative 'extract_base'

ze_header = <<~EOF
  #include <ze_api.h>
  #include <ze_ddi.h>
  #include <ze_ddi_ver.h>
  #include <loader/ze_loader.h>
EOF

if enable_clang_parser?
  header = [shared_header, ze_header].join("\n")
  require 'open3'
  yaml, status = Open3.capture2('h2yaml --compat-cast-to-yaml -Wc,-xc -Wc,-Imodified_include/ -', stdin_data: header)
  exit(1) unless status.success?

else
  preprocessed_sources_ze_api = $cpp.preprocess(ze_header).gsub(/^#.*?$/, '')
  ast = $parser.parse(preprocessed_sources_ze_api)
  yaml = ast.extract_declarations.to_yaml
end

File.open('ze_api.yaml', 'w') do |f|
  f.puts yaml
end
