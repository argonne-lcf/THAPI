require 'cast-to-yaml'
require 'yaml'


$parser = C::Parser::new
$parser.type_names << '__builtin_va_list'
$cpp = C::Preprocessor::new
$cpp.macros['__attribute__(a)'] = ''
$cpp.macros['__restrict'] = 'restrict'
$cpp.macros['__extension__'] = ''
$cpp.macros['__zecall'] = ''
$cpp.macros['__asm__(a)'] = ''
$cpp.include_path << './include/core'
$cpp.include_path << './include/experimental'
$cpp.include_path << './include/tools'



preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdint.h>
#include <string.h>
EOF
$parser.parse(preprocessed_sources_libc)
