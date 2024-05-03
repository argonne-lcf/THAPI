require_relative 'extract_base.rb'

preprocessed_sources_ze_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <ze_api.h>
#include <ze_ddi.h>
EOF

preprocessed_sources_zes_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#define _ZE_API_H
#include <zes_api.h>
#include <zes_ddi.h>
#include <zes_ddi_ver.h>
EOF

$parser.parse(preprocessed_sources_ze_api)
ast = $parser.parse(preprocessed_sources_zes_api)
File::open("zes_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
