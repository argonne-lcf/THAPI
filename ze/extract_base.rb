require 'cast-to-yaml'
require 'yaml'


$parser = C::Parser::new
$parser.type_names << '__builtin_va_list'
$cpp = C::Preprocessor::new
$cpp.macros['__attribute__(a)'] = ''
$cpp.macros['__restrict'] = 'restrict'
$cpp.macros['__extension__'] = ''
$cpp.macros['__zecall'] = ''
$cpp.macros['__zedllexport'] = ''
$cpp.macros['__ze_api_export'] = ''
$cpp.macros['__asm__(a)'] = ''
$cpp.macros['ZE_ENABLE_OCL_INTEROP'] = '1'
$cpp.include_path << './modified_include/core'
$cpp.include_path << './modified_include/experimental'
$cpp.include_path << './modified_include/tools'



preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdint.h>
#include <stddef.h>
typedef struct _cl_platform_id *    cl_platform_id;
typedef struct _cl_device_id *      cl_device_id;
typedef struct _cl_context *        cl_context;
typedef struct _cl_command_queue *  cl_command_queue;
typedef struct _cl_mem *            cl_mem;
typedef struct _cl_program *        cl_program;
typedef struct _cl_kernel *         cl_kernel;
typedef struct _cl_event *          cl_event;
typedef struct _cl_sampler *        cl_sampler;
EOF
$parser.parse(preprocessed_sources_libc)
