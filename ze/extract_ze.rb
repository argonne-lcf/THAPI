require_relative 'extract_base'

if ENV['ENABLE_CLANG_PARSER']
  header = "
#{shared_header}
#include <ze_api.h>
#include <ze_ddi.h>
#include <ze_ddi_ver.h>
#include <loader/ze_loader_api.h>"

  require 'open3'

  yaml, = Open3.capture2('h2yaml -xc -I modified_include/ -', stdin_data: header)

else

  preprocessed_sources_ze_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #include <ze_api.h>
    #include <ze_ddi.h>
    #include <ze_ddi_ver.h>
    #include <loader/ze_loader_api.h>
  EOF

  $parser.parse(preprocessed_sources_ze_api)
  ast = $parser.parse(preprocessed_sources_zex_api)
  yaml = ast.extract_declarations.to_yaml

end

File.open('ze_api.yaml', 'w') do |f|
  f.puts yaml
end
