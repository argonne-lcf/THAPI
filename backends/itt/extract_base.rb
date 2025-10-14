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

unless enable_clang_parser?
  require 'cast-to-yaml'

  $parser = C::Parser.new
  $cpp = C::Preprocessor.new
  $cpp.macros['__attribute__(a)'] = ''
  $cpp.include_path << './modified_include/'
  begin
    preprocessed_sources_libc = $cpp.preprocess(shared_header).gsub(/^#.*?$/, '')
  rescue StandardError
    C::Preprocessor.command = 'gcc -E'
    preprocessed_sources_libc = $cpp.preprocess(shared_header).gsub(/^#.*?$/, '')
  end
  $parser.parse(preprocessed_sources_libc)

end
