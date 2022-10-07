require 'erb'
require 'yaml'
require 'set'

SRC_DIR = ENV['SRC_DIR'] || '.'
START = 'entry'
STOP = 'exit'

class DBT_event
  attr_reader :fields, :name_unsanitized

  def initialize(klass)
    #Should be named lltng
    @name_unsanitized = klass[:name]
    @fields = klass[:payload]
  end

  def fields_name
    @fields.map { |f| f[:name] }
  end

  #uuid
  def name
    @name_unsanitized.gsub(':', '_')
  end

  def start?
    @name_unsanitized.end_with?(START)
  end
 
  def stop?
    @name_unsanitized.end_with?(STOP)
  end

  def name_prefix
    @name_unsanitized.gsub(/_?(?:#{START}|#{STOP})?$/, '')   
  end 

  def name_striped
    # #{namespace}:#{foo}_#{START} -> #{foo}
    # #{namespace}:#{foo} -> #{foo}
    @name_unsanitized[/:(.*?)_?(?:#{START}|#{STOP})?$/, 1]
  end

  def callback_signature
    decls = []
    @fields.each do |f|
      decls.push ['size_t', "_#{f[:name]}_length"] if f[:class] == 'array_static'
      decls.push [f[:cast_type], f[:name]]
    end
    (['const bt_event *bt_evt', 'const bt_clock_snapshot *bt_clock'] + decls.map { |t, n| "#{t} #{n}" }).join(",\n    ")
  end
end

ze_babeltrace_model = YAML.load_file('ze_babeltrace_model.yaml')

$dbt_events = ze_babeltrace_model[:event_classes].map { |klass|
  DBT_event.new(klass)
}

$dbt_events_who_signal = $dbt_events.filter_map { |dbt_event| dbt_event.name_striped if dbt_event.start? and dbt_event.fields_name.include?('hSignalEvent') }
$profiling_apis = Set.new

template = File.read(File.join(SRC_DIR, "zeinterval_callbacks.cpp.erb"))

t = ERB.new(template).result(binding)

if ARGV[0] == "callbacks"
  puts t.gsub(/^\s*$\n/, '')
elsif ARGV[0] == "apis"
  puts $profiling_apis.to_a.join(',')
else
  raise "Invalid argument #{ARGV[0]}"
end
