require 'yaml'
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
puts src
