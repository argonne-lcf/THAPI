require_relative 'extract_base'

zex_header = <<~EOF
  #include <zex_api.h>
EOF

if enable_clang_parser?
  header = [shared_header, zex_header].join("\n")
  puts header
  require 'open3'
  yaml, = Open3.capture2('h2yaml -xc -I modified_include/ --filter-header zex -', stdin_data: header)
else

  preprocessed_sources_ze_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #include <ze_api.h>
  EOF

  preprocessed_sources_zex_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #define _ZE_API_H
    #{zex_header}
  EOF

  $parser.parse(preprocessed_sources_ze_api)
  ast = $parser.parse(preprocessed_sources_zex_api)
  yaml = ast.extract_declarations.to_yaml

end

File.open('zex_api.yaml', 'w') do |f|
  f.puts yaml
end
