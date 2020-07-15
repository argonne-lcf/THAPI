require_relative 'extract_base.rb'

preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdlib.h>
#include <stdint.h>
EOF
$parser.parse(preprocessed_sources_libc)

preprocessed_sources_cuda_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <cuda.h>
EOF

ast = $parser.parse(preprocessed_sources_cuda_api)

File::open("cuda_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
