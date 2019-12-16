require_relative 'extract_base.rb'

preprocessed_sources_ze_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <ze_api.h>
#include <ze_ddi.h>
EOF

preprocessed_sources_zex_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#define _ZE_API_H
#include <zex_api.h>
#include <zex_ddi.h>
EOF

preprocessed_sources_zet_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#define _ZE_API_H
#define _ZEX_API_H
#include <zet_api.h>
#include <zet_ddi.h>
EOF

$parser.parse(preprocessed_sources_ze_api)
$parser.parse(preprocessed_sources_zex_api)
ast = $parser.parse(preprocessed_sources_zet_api)
File::open("zet_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
