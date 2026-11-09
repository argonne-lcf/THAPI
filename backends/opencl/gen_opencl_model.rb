# opencl_model.yaml: what the babeltrace model (gen_babeltrace_cl_model.rb) and
# the pretty-printer (babeltrace_opencl_lib.rb) read about OpenCL. The registry
# facts come from opencl_xml.yaml, the events from the traced commands and the
# hand-written event files.
require_relative 'opencl_model'

CL_EVENT_SUFFIXES = SUFFIXES.transform_keys(&:to_s)

res = OPENCL_XML.slice('enums', 'bitfields', 'objects', 'int_scalars', 'float_scalars', 'type_map', 'structures')
                .merge('lttng_enums' => {}, 'events' => {})
events = res['events']

def array_field?(f)
  f.macro.to_s.match?(/array|sequence|blob/)
end

def string_field?(f)
  f.macro.to_s.match?('string')
end

# A meta-parameter's field, described by the parameter it reads.
meta_field = lambda do |params, f|
  fname = f.name.to_s
  field = {}
  field['type'] = if fname == 'errcode_ret_val'
                    'cl_errcode'
                  elsif fname.match(/_val\z/)
                    params[fname.gsub(/_val\z/, '')]['type']
                  else
                    params[f.expression]['type']
                  end
  if field['type'].match(/\*\z/)
    field['type'] = field['type'].sub(/\*\z/, '')
    field['pointer'] = true
  end
  field['array'] = true if array_field?(f)
  field['string'] = true if string_field?(f)
  field['lttng'] = f.macro.to_s
  field['length'] = f.length if f.macro.to_s.match('ctf_array')
  if field['array'] && !field['pointer'] && res['structures'].include?(field['type'])
    field.delete('array')
    field['structure'] = true
  end
  [fname, field]
end

# A hand-written event's field ([macro, *args]), described by the argument it
# reads.
declared_field = lambda do |args, tuple|
  f = LTTng::TracepointField.new(*tuple)
  name = f.name.to_s
  type = name.match(/_val\z/) ? args[name.delete_suffix('_val')] : args[name] || args[f.expression]
  field = {}
  if type.match(/\*\z/)
    type = type.sub(/\*\z/, '').strip
    field['pointer'] = true
  end
  field = { 'type' => type }.merge(field)
  if array_field?(f) || string_field?(f)
    field.delete('pointer')
    field['array'] = true if array_field?(f)
    field['string'] = true if string_field?(f)
  end
  field['enum_type'] = f.enum_name if f.macro == :ctf_enum
  field['lttng'] = tuple[0]
  [name, field]
end

declared_fields = lambda do |args, tuples|
  args_h = args.to_h(&:reverse)
  tuples.to_a.to_h { |t| declared_field.call(args_h, t) }
end

COMMANDS.each do |c|
  params = c.declaration('params').to_h do |p|
    type = p['callback'] || p['registry_type'] == '' ? 'void' : p['registry_type']
    type = 'void*' if type == '*'
    param = { 'type' => type }
    param['pointer'] = true if p['pointer']
    [p['name'], param]
  end
  start = {}
  c.parameters.to_a.each do |p|
    f = p.lttng_type(c.type_classes)
    start[f.name.to_s] = params[f.name.to_s].merge('lttng' => f.macro.to_s)
  end
  c.meta_parameters.filter_map(&:lttng_in_type).each { |f| start.store(*meta_field.call(params, f)) }
  events["lttng_ust_opencl:#{c.name}_#{START}"] = start

  stop = {}
  if c.has_return_type? && (r = c.type.lttng_type(c.type_classes))
    type = c.result_field == 'errcode_ret_val' ? 'cl_errcode' : c.declaration('return_type')
    stop[c.result_field] = { 'type' => type, 'lttng' => r.macro.to_s }
  end
  outputs = c.meta_parameters.filter_map(&:lttng_out_type).reject { |f| f.name.to_s == '_param_name' }
  outputs.each { |f| stop.store(*meta_field.call(params, f)) }
  events["lttng_ust_opencl:#{c.name}_#{STOP}"] = stop
end

yaml_load_file_cached(File.join(SRC_DIR, 'opencl_wrapper_events.yaml')).each do |namespace, h|
  h['events'].each do |e|
    CL_EVENT_SUFFIXES.each do |dir, suffix|
      events["#{namespace}:#{e['name']}_#{suffix}"] = declared_fields.call(e['args'], e[dir])
    end
  end
end

yaml_load_file_cached(File.join(SRC_DIR, 'opencl_events.yaml')).each do |namespace, h|
  h['enums'].to_a.each do |e|
    res['lttng_enums'][e['name']] = { values: e['values'].collect { |v| { type: v[0], name: v[1], value: v[2] } } }
  end
  h['events'].each do |e|
    events["#{namespace}:#{e['name']}"] = declared_fields.call(e['args'], e['fields'])
  end
end

res['suffixes'] = CL_EVENT_SUFFIXES

puts YAML.dump(res)
