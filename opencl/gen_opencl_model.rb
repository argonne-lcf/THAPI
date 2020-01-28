require_relative 'opencl_model'

en = YAML::load_file("supported_enums.yaml")
en.push( { "name" => "cl_bool"} )
en.push( { "name" => "command execution status", "trace_name" => "command_exec_callback_type", "type_name" => "cl_command_execution_status" } )

enums = {}
res = { "enums" => enums }

en.each { |e|
  vals = $requires.select { |r|
    r.comment && r.comment.match(/#{e["name"]}(\z| )/)
  }.collect { |r|
    r.enums
  }.reduce(:+).collect { |v|
   [v, $constants[v]]
  }.to_h
  r = { "values" => vals}
  r["trace_name"] = e["trace_name"] if e["trace_name"]
  r["type_name"] = e["type_name"] if e["type_name"]

  enums[e["name"]] = r
}

puts YAML::dump(res)
