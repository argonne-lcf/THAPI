if ENV['ENABLE_CLANG_PARSER']
  header = "
#define ZE_MAKE_VERSION( _major, _minor )  (( _major << 16 )|( _minor & 0x0000ffff))
#define ZE_MAJOR_VERSION( _ver )  ( _ver >> 16 )
#define ZE_MINOR_VERSION( _ver )  ( _ver & 0x0000ffff )
#define ZE_BIT( _i )  ( 1 << _i )

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <ze_api.h>
#include <ze_ddi.h>
#include <ze_ddi_ver.h>
#include <loader/ze_loader_api.h>"

  require 'open3'

  o, = Open3.capture2('h2yaml -xc -I modified_include/ -', stdin_data: header)

  File.open('ze_api.yaml', 'w') do |f|
    f.puts o
  end

else

  require_relative 'extract_base'

  preprocessed_sources_ze_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #include <ze_api.h>
    #include <ze_ddi.h>
    #include <ze_ddi_ver.h>
    #include <loader/ze_loader_api.h>
  EOF

  ast = $parser.parse(preprocessed_sources_ze_api)
  File.open('ze_api.yaml', 'w') do |f|
    f.puts ast.extract_declarations.to_yaml
  end

end
