require_relative 'extract_base.rb'

preprocessed_sources_omp_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <omp-tools.h>
EOF

ast = $parser.parse(preprocessed_sources_omp_api)
File::open("ompt_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
