require 'babeltrace2'
require 'yaml'
require 'optparse'

OptionParser.new do |opts|
  opts.banner = 'Usage: dust.rb [options] trace_file'
  opts.on('-f', '--file FILENAME', 'Configuration file Name') do |path|
    $dust_schema = YAML.load_file(path)
  end
end.parse!

def get_comment_class(type, plugin, name)
  if plugin != 'ruby'
    bt_plugins = BT2::BTPlugin.find(plugin)
    bt_plugins.send("get_#{type}_component_class_by_name", name)
  else
    require_relative 'bt_plugins/comparator'
    require_relative 'bt_plugins/dust'
    Object.const_get(name.capitalize).new.create_component_class
  end
end

# TODO: Create only the components who are connected
def create_bt_components(graph, compopents)
  compopents.map do |component|
    type, plugin, name = component[:plugin].split('.')
    bt_component_class = get_comment_class(type, plugin, name)

    bt_component = graph.add(bt_component_class,
                             component[:name],
                             params: component.fetch(:params, {}))
    [component[:name], bt_component]
  end.to_h
end

def connect_ports(graph, bt_components, connections)
  starting_port = Hash.new(0)
  connections.each do |connection|
    component_in, component_out = connection.split(':')
    bt_component = bt_components[component_in]
    sp = starting_port[component_out]

    bt_component.output_ports
                .each.with_index(sp) do |op, i|
      ip = bt_components[component_out].input_port(i)
      graph.connect_ports(op, ip)
    end
    starting_port[component_out] += bt_component.output_port_count
  end
end

graph = BT2::BTGraph.new
bt_components = create_bt_components(graph, $dust_schema[:graph_plugins][:components])
connect_ports(graph, bt_components, $dust_schema[:graph_plugins][:connections])
graph.run
