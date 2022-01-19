require_relative 'extract_base.rb'

SRC_DIR = ENV['SRC_DIR'] || '.'

begin
  preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdlib.h>
#include <stdint.h>
#include <cuda.h>
EOF
rescue
  C::Preprocessor.command = "gcc -E"
  preprocessed_sources_libc = $cpp.preprocess(<<EOF).gsub(/^#.*?$/, '')
#include <stdlib.h>
#include <stdint.h>
#include <cuda.h>
EOF
end

$parser.parse(preprocessed_sources_libc)

export_tables = YAML::load(File::read(File.join(SRC_DIR, "cuda_export_tables.yaml")))
src = ""
export_tables.each { |table|
  if table["structures"]
    table["structures"].each { |struct|
      src << <<EOF
  typedef
#{struct["declaration"]}
  #{struct["name"]};
EOF
    }
  end
  table["functions"].each { |function|
    src << <<EOF
#{function["declaration"]};
EOF
  }
}

preprocessed_sources_cuda_api = $cpp.preprocess(src).gsub(/^#.*?$/, '')

ast = $parser.parse(preprocessed_sources_cuda_api)

File::open("cuda_exports_api.yaml", "w") { |f|
  f.puts ast.extract_declarations.to_yaml
}
