require_relative 'opencl_model'

en = YAML::load_file("supported_enums.yaml")
en.push( { "name" => "cl_bool"} )
en.push( { "name" => "command execution status", "trace_name" => "command_exec_callback_type", "type_name" => "cl_command_execution_status" } )

enums = {}
events = {}
res = { "enums" => enums, "events" => events }

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


event_lambda = lambda { |c, dir|
  name = "lttng_ust_opencl:#{c.prototype.name}_#{dir}"
  fields = []
  meta_fields = []
  val = {"fields" => fields, "meta_fields" => meta_fields}
  if dir == :start
    c.parameters.select { |p| p.lttng_in_type }.each { |p|
      field = {}
      field["type"] = (p.type == '' ? "void" : p.type)
      field["pointer"] = p.pointer?
      field["lttng"] = p.lttng_in_type
      fields.push field
    }
    c.meta_parameters.select { |p| p.lttng_in_type }.each { |p|
      meta_field = {}
      meta_field["lttng"] = p.lttng_in_type
      meta_fields.push meta_field
    }
  else
    field = {}
    field["type"] = c.prototype.return_type
    field["lttng"] = c.prototype.lttng_return_type
    fields.push field
    c.meta_parameters.select { |p| p.lttng_out_type }.each { |p|
      meta_field = {}
      meta_field["lttng"] = p.lttng_out_type
      meta_fields.push meta_field
    }
  end
  [name, val]
}

($opencl_commands+$opencl_extension_commands).each { |c|
  [:start, :stop].each { |dir|
    name, val = event_lambda.call(c, dir)
    events[name] = val
  }
}

puts YAML::dump(res)
