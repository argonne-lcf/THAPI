require_relative 'ze_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_zex_structs

puts <<EOF
#include "lttng/tracepoint_gen.h"
#include "ze.h.include"
EOF

$zex_api["typedefs"].select { |t|
  t.type.kind_of?(YAMLCAst::Struct) && (struct = $zex_api["structs"].find { |s| t.type.name == s.name }) && struct.members.first.name == "stype"
}.each { |t|
  $struct_tracepoint_lambda.call(provider, t.name)
}
