require_relative 'extract_base'

mpi_header = <<~EOF
  #include <mpi.h>
EOF

if enable_clang_parser?
  header = [shared_header, mpi_header].join("\n")
  require 'open3'
  yaml, = Open3.capture2('h2yaml -xc -I modified_include/ -', stdin_data: header)
else

  preprocessed_sources_mpi_api = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
    #{mpi_header}
  EOF

  ast = $parser.parse(preprocessed_sources_mpi_api)
  yaml = ast.extract_declarations.to_yaml

end

File.open('mpi_api.yaml', 'w') do |f|
  f.puts yaml
end
