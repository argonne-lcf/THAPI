require 'cast-to-yaml'
require 'yaml'


$parser = C::Parser::new
$parser.type_names << '__builtin_va_list'
$cpp = C::Preprocessor::new
$cpp.macros['__attribute__(a)'] = ''
$cpp.macros['__restrict'] = 'restrict'
$cpp.macros['__inline'] = 'inline'
$cpp.macros['__extension__'] = ''
$cpp.macros['__asm__(a)'] = ''
$cpp.macros['__CUDA_API_VERSION_INTERNAL'] = '1'
$cpp.macros['__USE_ISOC99'] = '1'
$cpp.include_path << './modified_include/'
