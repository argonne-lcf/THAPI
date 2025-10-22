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
  yaml, status = Open3.capture2("h2yaml --compat-cast-to-yaml -Wc,-xc -Wc,-I#{SRC_DIR} -Wc,-Imodified_include/ -", stdin_data: header)
  exit(1) unless status.success?

else
  preprocessed_sources_hip_api = $cpp.preprocess(hip_header).gsub(/^#.*?$/, '')
  ast = $parser.parse(preprocessed_sources_hip_api)
  yaml = ast.extract_declarations.to_yaml
end

File.open('hip_api.yaml', 'w') do |f|
  f.puts yaml
end
