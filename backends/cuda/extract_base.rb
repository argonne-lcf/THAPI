require 'yaml'

def enable_clang_parser?
  ENV.fetch('ENABLE_CLANG_PARSER', '0') == '1'
end

if enable_clang_parser?

  def shared_header
    <<~EOF
      #include <stdlib.h>
      #include <stdint.h>
      #define __CUDA_API_VERSION_INTERNAL=1
      #define CU_COMPUTE_ACCELERATED_TARGET_BASE   0x10000
      #define CU_TENSOR_MAP_NUM_QWORDS 16
    EOF
  end

else

  require 'cast-to-yaml'

  $parser = C::Parser.new
  $parser.type_names << '__builtin_va_list'
  $cpp = C::Preprocessor.new
  $cpp.macros['__attribute__(a)'] = ''
  $cpp.macros['__restrict'] = 'restrict'
  $cpp.macros['__inline'] = 'inline'
  $cpp.macros['__extension__'] = ''
  $cpp.macros['__asm__(a)'] = ''
  $cpp.macros['_Alignas(v)'] = ''
  $cpp.macros['__CUDA_API_VERSION_INTERNAL'] = '1'
  $cpp.include_path << './modified_include/'
end
