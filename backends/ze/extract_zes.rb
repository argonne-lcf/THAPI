require_relative 'extract_base'

zes_header = <<EOF
    #include <zes_api.h>
    #include <zes_ddi.h>
    #include <zes_ddi_ver.h>
EOF

if enable_clang_parser?
  header = [shared_header, zes_header].join("\n")
  require 'open3'
  yaml, status = Open3.capture2('h2yaml -Wc,-xc -Wc,-Imodified_include/ --filter-header zes -', stdin_data: header)
  exit(1) unless status.success?
else

  require_relative 'extract_base'

  preprocessed_sources_ze_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #include <ze_api.h>
    #include <ze_ddi.h>
  EOF

  preprocessed_sources_zes_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #define _ZE_API_H
    #{zes_header}
  EOF

  $parser.parse(preprocessed_sources_ze_api)
  ast = $parser.parse(preprocessed_sources_zes_api)
  yaml = ast.extract_declarations.to_yaml

end

File.open('zes_api.yaml', 'w') do |f|
  f.puts yaml
end
