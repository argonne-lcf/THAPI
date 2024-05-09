require_relative 'extract_base.rb'

preprocessed_sources_ze_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <ze_api.h>
EOF

preprocessed_sources_zex_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#define _ZE_API_H
#include <zex_api.h>
EOF

$parser.parse(preprocessed_sources_ze_api)
ast = $parser.parse(preprocessed_sources_zex_api)
File::open("zex_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
