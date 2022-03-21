require 'erb'
require 'yaml'

SRC_DIR = ENV['SRC_DIR'] || '.'

OMP_MODEL_ENUMS = YAML.load_file('ompt_api.yaml')["enums"]

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

  def enum
    @enums ||= @fields.filter_map{ |f|
        next if (f[:cast_type] == "ompt_scope_endpoint_t")
        OMP_MODEL_ENUMS.lazy.filter_map { |e| [f,e] if e["name"] == f[:cast_type] }.first
    }
    raise if @enums.size > 1
    @enums.first
  end

  def have_enum?
    !(enum.nil?)
  end

  def enum_t
    enum[0][:cast_type]
  end

  def enum_name
    enum[0][:name]
  end

  def enum_fields
    enum[1]["members"].map { |f| f["name"] }
  end

  def name_striped
    @name_unsanitized[/:(.*?)\z/, 1]
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

omp_babeltrace_model = YAML.load_file('omp_babeltrace_model.yaml')
$dbt_events = omp_babeltrace_model[:event_classes].map { |klass|
  DBT_event.new(klass)
}

template = File.read(File.join(SRC_DIR, "ompinterval_callbacks.cpp.erb"))
puts ERB.new(template).result(binding).gsub(/^\s*$\n/, '')
