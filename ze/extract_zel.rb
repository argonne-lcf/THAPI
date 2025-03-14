require_relative 'extract_base'

if ENV['ENABLE_CLANG_PARSER']
  header = "
#{shared_header}

#include <layers/zel_tracing_api.h>
#include <layers/zel_tracing_ddi.h>
#include <layers/zel_tracing_ddi_ver.h>
#include <layers/zel_tracing_register_cb.h>
#include <loader/ze_loader.h>"

  require 'open3'
  yaml, = Open3.capture2('h2yaml -xc -I modified_include/ --filter-header "zel|ze_loader" -', stdin_data: header)

else

  require_relative 'extract_base'

  preprocessed_sources_ze_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #include <ze_api.h>
    #include <ze_ddi.h>
  EOF

  preprocessed_sources_zel_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #define _ZE_API_H
    #include <layers/zel_tracing_api.h>
    #include <layers/zel_tracing_ddi.h>
    #include <layers/zel_tracing_ddi_ver.h>
    #include <layers/zel_tracing_register_cb.h>
    #include <loader/ze_loader.h>
  EOF

  $parser.parse(preprocessed_sources_ze_api)
  ast = $parser.parse(preprocessed_sources_zel_api)
  yaml = ast.extract_declarations.to_yaml

end

File.open('zel_api.yaml', 'w') do |f|
  f.puts yaml
end
