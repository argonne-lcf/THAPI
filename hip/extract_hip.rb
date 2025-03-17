require_relative 'extract_base'

hip_header = <<~EOF
  #include <hip/hip_runtime_api.h>
  #include <hip/hiprtc.h>
  #include <hip/hip_runtime_load_api.h>
  #include <hip/hip_ext.h>
  #include "hip_cpp_symbols.h"
  #include "hip_missing_apis.h"
EOF

if enable_clang_parser?
  header = [shared_header, hip_header].join("\n")
  require 'open3'
  Open3.capture2('h2yaml -xc -I modified_include/ -', stdin_data: header)
else
  preprocessed_sources_hip_api = $cpp.preprocess(hip_header).gsub(/^#.*?$/, '')
  ast = $parser.parse(preprocessed_sources_hip_api)
  ast.extract_declarations.to_yaml
end

File.open('hip_api.yaml', 'w') do |f|
  f.puts ast.extract_declarations.to_yaml
end
