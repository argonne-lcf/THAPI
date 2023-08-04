require_relative 'extract_base.rb'

begin
  preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdint.h>
EOF
rescue
  C::Preprocessor.command = "gcc -E"
  preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdint.h>
EOF
end

$parser.parse(preprocessed_sources_libc)

begin
  preprocessed_sources_cudart_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <cuda_runtime_api.h>
#include <__cudart.h>
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
#include <cuda_vdpau_interop.h>
#include <cuda_profiler_api.h>
EOF
rescue
  C::Preprocessor.command = "gcc -E"
  preprocessed_sources_cudart_api = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <cuda_runtime_api.h>
#include <__cudart.h>
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
#include <cuda_vdpau_interop.h>
#include <cuda_profiler_api.h>
EOF
end

ast = $parser.parse(preprocessed_sources_cudart_api)

File::open("cudart_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
