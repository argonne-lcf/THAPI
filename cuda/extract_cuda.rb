require_relative 'extract_base.rb'

begin
  preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdlib.h>
#include <stdint.h>
EOF
rescue
  C::Preprocessor.command = "gcc -E"
  preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdlib.h>
#include <stdint.h>
EOF
end

$parser.parse(preprocessed_sources_libc)

  preprocessed_sources_cuda_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <cuda.h>
typedef int32_t VdpStatus;
typedef uint32_t VdpFuncId;
typedef uint32_t VdpDevice;
typedef VdpStatus  VdpGetProcAddress(
     VdpDevice device,
     VdpFuncId function_id,
     void * *  function_pointer
);
typedef uint32_t VdpVideoSurface;
typedef uint32_t VdpOutputSurface;
#include <cudaVDPAU.h>
#include <cudaProfiler.h>
EOF
ast = $parser.parse(preprocessed_sources_cuda_api)

File::open("cuda_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
