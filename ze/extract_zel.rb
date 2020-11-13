require_relative 'extract_base.rb'

preprocessed_sources_ze_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <ze_api.h>
#include <ze_ddi.h>
EOF

preprocessed_sources_zel_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#define _ZE_API_H
#include <layers/zel_tracing_api.h>
#include <layers/zel_tracing_ddi.h>
EOF

$parser.parse(preprocessed_sources_ze_api)
ast = $parser.parse(preprocessed_sources_zel_api)
File::open("zel_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
