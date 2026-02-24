require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_ze_structs

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include "ze.h.include"
EOF

$ze_api['typedefs'].select do |t|
  t.type.is_a?(YAMLCAst::Struct) && (struct = $ze_api['structs'].find do |s|
    t.type.name == s.name
  end) && struct.members.first.name == 'stype'
end.each do |t|
  $struct_tracepoint_lambda.call(provider, t.name)
end
