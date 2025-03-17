require_relative 'extract_base'

zet_header = <<~EOF
  #include <zet_api.h>
  #include <zet_ddi.h>
  #include <zet_ddi_ver.h>
EOF

if enable_clang_parser?
  header = [shared_header, zet_header].join("\n")
  require 'open3'
  yaml, = Open3.capture2('h2yaml -xc -I modified_include/ --filter-header zet -', stdin_data: header)

else

  preprocessed_sources_ze_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #include <ze_api.h>
    #include <ze_ddi.h>
  EOF

  preprocessed_sources_zet_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #define _ZE_API_H
    #{zet_header}
  EOF

  $parser.parse(preprocessed_sources_ze_api)
  ast = $parser.parse(preprocessed_sources_zet_api)
  yaml = ast.extract_declarations.to_yaml

end

File.open('zet_api.yaml', 'w') do |f|
  f.puts yaml
end
