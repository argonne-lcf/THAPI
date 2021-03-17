require 'babeltrace2'
require 'yaml'
require 'find'
require 'set'

path = ARGV[0]
trace_location = Find.find(path).select { |f|
  File.basename(f) == "metadata" }.collect { |f|
  File.dirname(f)
}.uniq

graph = BT2::BTGraph.new
ctf_fs = BT2::BTPlugin.find("ctf").get_source_component_class_by_name("fs")
utils_muxer = BT2::BTPlugin.find("utils").get_filter_component_class_by_name("muxer")
printed = false

comp1 = graph.add(ctf_fs, "trace", params: {"inputs" => trace_location})
comp2 = graph.add(utils_muxer, "mux")
comp3 = graph.add_simple_sink("print", lambda { |iterator, _|
  mess = iterator.next_messages
  mess.each { |m|
    if !printed
      puts YAML.dump(m.stream.get_class.trace_class.to_h)
      printed = true
    end
  }
})
comp1.output_ports.each_with_index { |op, i|
  ip = comp2.input_port(i)
  graph.connect_ports(op, ip)
}
graph.connect_ports(comp2.output_port(0), comp3.input_port(0))
graph.run
