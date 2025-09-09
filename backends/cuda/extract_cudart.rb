require_relative 'extract_base'

cudart_header = <<~EOF
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

if enable_clang_parser?
  header = [shared_header, cudart_header].join("\n")
  require 'open3'
  yaml, status = Open3.capture2('h2yaml -xc -I modified_include/ -', stdin_data: header)
  exit(1) unless status.success?
else

  begin
    preprocessed_sources_libc = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
      #include <stdint.h>
    EOF
  rescue StandardError
    C::Preprocessor.command = 'gcc -E'
    preprocessed_sources_libc = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
      #include <stdint.h>
    EOF
  end
  $parser.parse(preprocessed_sources_libc)

  preprocessed_sources_cudart_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #{cudart_header}
  EOF

  ast = $parser.parse(preprocessed_sources_cudart_api)
  yaml = ast.extract_declarations.to_yaml

end

File.open('cudart_api.yaml', 'w') do |f|
  f.puts yaml
end
