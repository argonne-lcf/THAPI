require 'cast-to-yaml'
require 'yaml'

SRC_DIR = ENV["SRC_DIR"]

$parser = C::Parser::new
$parser.type_names << '__builtin_va_list'
$cpp = C::Preprocessor::new
$cpp.macros['__attribute__(a)'] = ''
$cpp.macros['__restrict'] = 'restrict'
$cpp.macros['__inline'] = 'inline'
$cpp.macros['__extension__'] = ''
$cpp.macros['__asm__(a)'] = ''
$cpp.macros['__HIP_PLATFORM_AMD__'] = ''
$cpp.include_path << './modified_include/'
$cpp.include_path << "#{SRC_DIR}/"
