require_relative 'extract_base'

SRC_DIR = ENV['SRC_DIR'] || '.'
export_tables = YAML.load_file(File.join(SRC_DIR, 'cuda_export_tables.yaml'))
src = ''
export_tables.each do |table|
  if table['structures']
    table['structures'].each do |struct|
      src << <<~EOF
          typedef
        #{struct['declaration']}
          #{struct['name']};
      EOF
    end
  end
  table['functions'].each do |function|
    src << <<~EOF
      #{function['declaration']};
    EOF
  end
end

if enable_clang_parser?
  [shared_header, src].join("\n")
  require 'open3'
  yaml, = Open3.capture2('h2yaml -xc -I modified_include/ -', stdin_data: src)

else

  begin
    preprocessed_sources_libc = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
      #include <stdlib.h>
      #include <stdint.h>
      #include <cuda.h>
    EOF
  rescue StandardError
    C::Preprocessor.command = 'gcc -E'
    preprocessed_sources_libc = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
      #include <stdlib.h>
      #include <stdint.h>
      #include <cuda.h>
    EOF
  end

  $parser.parse(preprocessed_sources_libc)
  preprocessed_sources_cuda_api = $cpp.preprocess(src).gsub(/^#.*?$/, '')
  ast = $parser.parse(preprocessed_sources_cuda_api)
  yaml = ast.extract_declarations.to_yaml

end

File.open('cuda_exports_api.yaml', 'w') do |f|
  f.puts yaml
end
