require 'yaml'

def enable_clang_parser?
  ENV.fetch('ENABLE_CLANG_PARSER', '0') == '1'
end

def shared_header
  <<~EOF
    #include <stdint.h>
    #include <stddef.h>
  EOF
end

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
    preprocessed_sources_libc = $cpp.preprocess(shared_header).gsub(/^#.*?$/, '')
  rescue StandardError
    C::Preprocessor.command = 'gcc -E'
    preprocessed_sources_libc = $cpp.preprocess(shared_header).gsub(/^#.*?$/, '')
  end
  $parser.parse(preprocessed_sources_libc)

end
