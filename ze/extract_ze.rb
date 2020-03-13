require_relative 'extract_base.rb'

preprocessed_sources_ze_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <ze_api.h>
#include <ze_ddi.h>
EOF

ast = $parser.parse(preprocessed_sources_ze_api)
File::open("ze_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
