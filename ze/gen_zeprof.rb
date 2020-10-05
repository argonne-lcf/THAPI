require 'erb'
require_relative 'gen_ze_library_base.rb'

meta_parameter_lambda = lambda { |m, dir|
  if dir == :start
    lttng = m.lttng_in_type
  else
    lttng = m.lttng_out_type
  end
  name = lttng.name
  t = m.command[m.name].type.type

  case m
  when ScalarMetaParameter
    if lttng.length
      ["size_t _#{name}_length", "#{t} *#{name}"]
    else
      "#{t} #{name}"
    end
  when ArrayMetaParameter, InString, OutString
    if lttng.macro.to_s == "ctf_string"
      "#{t} *#{name}"
    else
      ["size_t _#{name}_length", "#{t} *#{name}"]
    end
  end   
}

class DBT_event

   def initialize(namespace, name, fields, suffix=nil)
       @namespace = namespace
       @name = name
       @fields = fields
       @suffix = suffix

   end 

   def name
       @name
   end   
   def suffix
       @suffix
   end
    
   def namespace
      @namespace
   end

   def uuid
     "#{@namespace}_#{@name}_#{@suffix}"
   end

   def lltng
     if @suffix
        "#{@namespace}:#{@name}_#{@suffix}"
     else
        "#{@namespace}:#{@name}"
     end
   end

   def fields
       @fields
   end

   def include?(p1)
       @fields.any? { |f| f.include? p1}
   end
end

gen_event_callback_name_fields = lambda { |provider, c, dir| 
  fields = ["const bt_event *bt_evt",
            "const bt_clock_snapshot *bt_clock"]
  if dir == :start
    fields += (c.parameters+c.tracepoint_parameters).collect { |p|
      p.to_s
    }
    fields += c.meta_parameters.select { |m| m.kind_of?(In) }.collect { |m|
      meta_parameter_lambda.call(m, :start)
    }
  else
    fields.push "ze_result_t #{RESULT_NAME}"
    fields += c.meta_parameters.select { |m| m.kind_of?(Out) }.collect { |m|
      meta_parameter_lambda.call(m, :stop)
    }
  end
  DBT_event.new(provider, c.name, fields, dir)
}

gen_extra_event_callback =lambda { |provider, event|
    fields = ["const bt_event *bt_evt",
              "const bt_clock_snapshot *bt_clock"]
    args = event["args"].collect { |arg| arg.reverse }.to_h
    event["fields"].each { |field|
      lttng = LTTng::TracepointField::new(*field)
      name = lttng.name
      type = lttng.type
      if name.match(/_val\z/)
        n = name.sub(/_val\z/,"")
        fields.push "size_t _#{name}_length"
        type = args[n] if args[n]
      else
        type = args[name] if args[name]
      end
      fields.push "#{type} #{name}"
    }
    DBT_event.new(provider,event["name"], fields)
}

def write_file_via_template(file, testing = false)
    template = File.read(File.join(SRC_DIR,"#{file}.erb"))
    template_rendered = ERB.new(template).result(binding).gsub(/^\s*$\n/,'')
    
    if testing
        File.write("testing_#{file}", template_rendered)
    else
        File.write("#{file}", template_rendered)
    end
end

provider = :lttng_ust_ze
$dbt_events = $ze_commands.map{ |c| gen_event_callback_name_fields.call(provider, c, :start) } + 
              $ze_commands.map{ |c| gen_event_callback_name_fields.call(provider, c, :stop) }

provider = :lttng_ust_zet
$dbt_events += $zet_commands.map{ |c| gen_event_callback_name_fields.call(provider, c, :start) } +
               $zet_commands.map{ |c| gen_event_callback_name_fields.call(provider, c, :stop) }

provider = :lttng_ust_zes
$dbt_events += $zes_commands.map{ |c| gen_event_callback_name_fields.call(provider, c, :start) } +
               $zes_commands.map{ |c| gen_event_callback_name_fields.call(provider, c, :stop) }

ze_events = YAML::load_file(File.join(SRC_DIR,"ze_events.yaml"))
$dbt_events += ze_events.map{ |provider,es|
    es['events'].map{ |event|
         gen_extra_event_callback.call(provider, event)
    }
}.flatten

write_file_via_template("zeprof_callbacks.cpp")
write_file_via_template("zeprof_callbacks.h")
write_file_via_template("zeprof.c")

