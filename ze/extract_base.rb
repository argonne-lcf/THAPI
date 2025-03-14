require 'yaml'

def enable_clang_parser?
  ENV.fetch('ENABLE_CLANG_PARSER', "0") == "1"
end

if enable_clang_parser?

  def shared_header
    "#define ZE_MAKE_VERSION( _major, _minor )  (( _major << 16 )|( _minor & 0x0000ffff))
#define ZE_MAJOR_VERSION( _ver )  ( _ver >> 16 )
#define ZE_MINOR_VERSION( _ver )  ( _ver & 0x0000ffff )
#define ZE_BIT( _i )  ( 1 << _i )

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <ze_api.h>
#include <ze_ddi.h>"
  end
else

  require 'cast-to-yaml'

  $parser = C::Parser.new
  $parser.type_names << '__builtin_va_list'
  $cpp = C::Preprocessor.new
  $cpp.macros['__attribute__(a)'] = ''
  $cpp.macros['__restrict'] = 'restrict'
  $cpp.macros['__extension__'] = ''
  $cpp.macros['ZE_APICALL'] = ''
  $cpp.macros['ZE_DLLEXPORT'] = ''
  $cpp.macros['ZE_APIEXPORT'] = ''
  $cpp.macros['__asm__(a)'] = ''
  $cpp.include_path << './modified_include/'
  begin
    preprocessed_sources_libc = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
      #include <stdint.h>
      #include <stddef.h>
    EOF
  rescue StandardError
    C::Preprocessor.command = 'gcc -E'
    preprocessed_sources_libc = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
      #include <stdint.h>
      #include <stddef.h>
    EOF
  end
  $parser.parse(preprocessed_sources_libc)

end
