require_relative 'extract_base.rb'

begin
  preprocessed_sources_cudart_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <cuda_runtime_api.h>
EOF
rescue
  C::Preprocessor.command = "gcc -E"
  preprocessed_sources_cudart_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <cuda_runtime_api.h>
EOF
end

ast = $parser.parse(preprocessed_sources_cudart_api)

File::open("cudart_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
