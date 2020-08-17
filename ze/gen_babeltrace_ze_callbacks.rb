require_relative 'gen_ze_library_base.rb'
puts <<EOF
#include <ze_api.h>
#include <ze_ddi.h>
#include <zet_api.h>
#include <zet_ddi.h>
#include <zes_api.h>
#include <zes_ddi.h>
#include <babeltrace2/babeltrace.h>

EOF


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
  when ArrayMetaParameter
      ["size_t _#{name}_length", "#{t} *#{name}"]
  when InString
      ["size_t _#{name}_length", "#{t} *#{name}"]
  when OutString
      ["size_t _#{name}_length", "#{t} *#{name}"]
  end
}

gen_event_callback = lambda { |provider, c, dir|
  puts <<EOF
typedef void (#{provider}_#{c.name}_#{dir})(
EOF
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
  fields.flatten
  puts <<EOF
  #{fields.join(",\n  ")}
EOF
  puts <<EOF
);

EOF

}


provider = :lttng_ust_ze
$ze_commands.each { |c|
  gen_event_callback.call(provider, c, :start)
  gen_event_callback.call(provider, c, :stop)
}

provider = :lttng_ust_zet
$zet_commands.each { |c|
  gen_event_callback.call(provider, c, :start)
  gen_event_callback.call(provider, c, :stop)
}

provider = :lttng_ust_zes
$zes_commands.each { |c|
  gen_event_callback.call(provider, c, :start)
  gen_event_callback.call(provider, c, :stop)
}

