require 'yaml'

def enable_clang_parser?
  ENV.fetch('ENABLE_CLANG_PARSER', '0') == '1'
end

if enable_clang_parser?

  def shared_header
    "#include <stdint.h>
#include <stddef.h>"
  end
else

  require 'cast-to-yaml'

  $parser = C::Parser.new
  $parser.type_names << '__builtin_va_list'
  $cpp = C::Preprocessor.new
  $cpp.macros['__attribute__(a)'] = ''
  $cpp.macros['__restrict'] = 'restrict'
  $cpp.macros['__extension__'] = ''
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
