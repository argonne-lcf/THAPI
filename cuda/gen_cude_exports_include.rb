require 'yaml'
SRC_DIR = ENV["SRC_DIR"]
export_tables = YAML::load_file(File.join(SRC_DIR,"cuda_export_tables.yaml"))

puts <<EOF
#ifndef _CUDA_EXPORTS_H_INCLUDE
#define _CUDA_EXPORTS_H_INCLUDE

EOF

export_tables.each { |table|
  if table["structures"]
    table["structures"].each { |struct|
      puts <<EOF
typedef #{struct["declaration"].chomp} #{struct["name"]};

EOF
    }
  end
}

puts <<EOF
#endif
EOF
