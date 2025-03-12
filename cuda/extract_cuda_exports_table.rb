require 'yaml'
SRC_DIR = ENV['SRC_DIR'] || '.'

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
puts src
