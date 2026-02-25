require_relative 'opencl_model'
require_relative 'opencl_tracepoints'

en = YAML.load_file(File.join(SRC_DIR, 'supported_enums.yaml'))
en.push({ 'name' => 'cl_bool' })
en.push({ 'name' => 'command execution status', 'trace_name' => 'command_exec_callback_type',
          'type_name' => 'cl_command_execution_status' })

bitfields = {}
enums = {}
objects = CL_OBJECTS + CL_EXT_OBJECTS
int_scalars = CL_INT_SCALARS
float_scalars = CL_FLOAT_SCALARS
lttng_enums = {}
events = {}

res = {
  'enums' => enums,
  'bitfields' => bitfields,
  'objects' => objects,
  'int_scalars' => int_scalars,
  'float_scalars' => float_scalars,
  'type_map' => CL_TYPE_MAP,
  'structures' => CL_STRUCTS,
  'lttng_enums' => lttng_enums,
  'events' => events,
}

en.each do |e|
  bitfield = false
  vals = $requires.select do |r|
    r.comment && r.comment.match(/#{e['name']}(\z| )/)
  end.each do |r|
    bitfield = true if r.comment.match(/ - bitfield/)
  end.collect do |r|
    r.enums
  end.reduce(:+).collect do |v|
    [v, $constants[v]]
  end.to_h
  r = { 'values' => vals }
  r['trace_name'] = e['trace_name'] if e['trace_name']
  r['type_name'] = e['type_name'] if e['type_name']

  if bitfield
    bitfields[e['name']] = r
  else
    enums[e['name']] = r
  end
end

event_lambda = lambda { |c, dir|
  name = "lttng_ust_opencl:#{c.prototype.name}_#{SUFFIXES[dir]}"
  fields = {}
  params = {}
  c.parameters.each do |p|
    param = {}
    params[p.name] = param
    param['type'] = (if p.callback? || p.type == ''
                       'void'
                     else
                       p.type == '*' ? 'void*' : p.type
                     end)
    param['pointer'] = true if p.pointer?
  end
  if dir == 'start'
    c.parameters.select { |p| p.lttng_in_type }.each do |p|
      field = {}
      lttng = p.lttng_in_type
      fname = LTTng.name(*lttng)
      field.merge!(params[fname])
      field['lttng'] = lttng[0]
      fields[fname] = field
    end
    c.meta_parameters.select { |p| p.lttng_in_type }.each do |p|
      meta_field = {}
      lttng = p.lttng_in_type
      fname = LTTng.name(*lttng)
      if fname == 'errcode_ret_val'
        meta_field['type'] = 'cl_errcode'
      elsif fname.match(/_val\z/)
        pname = fname.gsub(/_val\z/, '')
        meta_field['type'] = params[pname]['type']
      else
        meta_field['type'] = params[LTTng.expression(*lttng)]['type']
      end
      if meta_field['type'].match(/\*\z/)
        meta_field['type'] = meta_field['type'].sub(/\*\z/, '')
        meta_field['pointer'] = true
      end
      meta_field['array'] = true if LTTng.array?(*lttng)
      meta_field['string'] = true if LTTng.string?(*lttng)
      meta_field['lttng'] = lttng[0]
      meta_field['length'] = lttng[4] if meta_field['lttng'].match('ctf_array')
      if  meta_field['array'] &&
          !meta_field['pointer'] &&
          CL_STRUCTS.include?(meta_field['type'])
        meta_field.delete('array')
        meta_field['structure'] = true
      end
      fields[fname] = meta_field
    end
  else
    field = {}
    if c.prototype.lttng_return_type
      field['type'] = c.prototype.return_type
      lttng = c.prototype.lttng_return_type
      field['lttng'] = lttng[0]
      fname = LTTng.name(*lttng)
      field['type'] = 'cl_errcode' if fname == 'errcode_ret_val'
      fields[fname] = field
    end
    c.meta_parameters.select { |p| p.lttng_out_type && LTTng.name(*p.lttng_out_type) != '_param_name' }.each do |p|
      meta_field = {}
      lttng = p.lttng_out_type
      fname = LTTng.name(*lttng)
      if fname == 'errcode_ret_val'
        meta_field['type'] = 'cl_errcode'
      elsif fname.match(/_val\z/)
        pname = fname.gsub(/_val\z/, '')
        meta_field['type'] = params[pname]['type']
      else
        begin
          meta_field['type'] = params[LTTng.expression(*lttng)]['type']
        rescue StandardError
          warn name, lttng.inspect
        end
      end
      if meta_field['type'].match(/\*\z/)
        meta_field['type'] = meta_field['type'].gsub(/\*\z/, '')
        meta_field['pointer'] = true
      end
      meta_field['array'] = true if LTTng.array?(*lttng)
      meta_field['string'] = true if LTTng.string?(*lttng)
      meta_field['lttng'] = lttng[0]
      meta_field['length'] = lttng[4] if meta_field['lttng'].match('ctf_array')
      if  meta_field['array'] &&
          !meta_field['pointer'] &&
          CL_STRUCTS.include?(meta_field['type'])
        meta_field.delete('array')
        meta_field['structure'] = true
      end
      fields[fname] = meta_field
    end
  end
  [name, fields]
}

($opencl_commands + $opencl_extension_commands).each do |c|
  %w[start stop].each do |dir|
    name, val = event_lambda.call(c, dir)
    events[name] = val
  end
end

YAML.load_file(File.join(SRC_DIR, 'opencl_wrapper_events.yaml')).each do |namespace, h|
  h['events'].each do |e|
    %w[start stop].each do |dir|
      event = get_fields(e['args'], e[dir.to_s])
      events["#{namespace}:#{e['name']}_#{SUFFIXES[dir]}"] = event
    end
  end
end

YAML.load_file(File.join(SRC_DIR, 'opencl_events.yaml')).each do |namespace, h|
  if h['enums']
    h['enums'].each do |e|
      lttng_enums[e['name']] = {
        values: e['values'].collect { |v| { type: v[0], name: v[1], value: v[2] } },
      }
    end
  end
  h['events'].each do |e|
    event = get_fields(e['args'], e['fields'])
    events["#{namespace}:#{e['name']}"] = event
  end
end

res['suffixes'] = SUFFIXES

puts YAML.dump(res)
