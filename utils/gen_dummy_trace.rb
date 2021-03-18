require 'yaml'
model_path = ARGV[0]
d = YAML.load_file(model_path)
schema_name = d[:name]
events = d[:event_classes].collect{ |f| {name: f[:name], stream: 'stream0'} }
puts YAML.dump({
  stream_classes: [
    { name: 'stream_class0', schemas: [ schema_name ], clock_snapshot_value: true }
  ],
  streams: [ {name: 'stream0', class: 'stream_class0' } ],
  events: events
})
