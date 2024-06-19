require 'cast-to-yaml'
require 'yaml'


$parser = C::Parser::new
$parser.type_names << '__builtin_va_list'
$cpp = C::Preprocessor::new
$cpp.macros['__attribute__(a)'] = ''
$cpp.macros['__restrict'] = 'restrict'
$cpp.macros['__extension__'] = ''
$cpp.macros['__asm__(a)'] = ''
$cpp.include_path << './modified_include/'
begin
  preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdint.h>
#include <stddef.h>
EOF
rescue
  C::Preprocessor.command = "gcc -E"
  preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdint.h>
#include <stddef.h>
EOF
end
$parser.parse(preprocessed_sources_libc)

