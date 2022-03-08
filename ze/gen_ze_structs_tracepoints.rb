require_relative 'ze_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_ze_structs

puts <<EOF
#include "ze.h.include"
EOF

$ze_api["typedefs"].select { |t|
  t.type.kind_of?(YAMLCAst::Struct) && (struct = $ze_api["structs"].find { |s| t.type.name == s.name }) && struct.members.first.name == "stype"
}.each { |t|
  $struct_tracepoint_lambda.call(provider, t.name)
}
