require_relative 'extract_base.rb'

begin
  preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <string.h>
#include <stdint.h>
#include <stddef.h>
EOF
rescue
  C::Preprocessor.command = "gcc -E"
  preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <string.h>
#include <stdint.h>
#include <stddef.h>
EOF
end

$parser.parse(preprocessed_sources_libc)

  preprocessed_sources_hip_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <hip/hip_runtime_api.h>
#include <hip/hiprtc.h>
#include <hip/hip_runtime_load_api.h>
#include <hip/hip_ext.h>
#include "hip_cpp_symbols.h"
#include "hip_missing_apis.h"
EOF
begin
  ast = $parser.parse(preprocessed_sources_hip_api)
rescue
  puts preprocessed_sources_hip_api
  raise
end

File::open("hip_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
