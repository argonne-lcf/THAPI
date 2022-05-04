require_relative 'ze_model'
require_relative 'gen_probe_base.rb'

provider = :lttng_ust_zel_structs

puts <<EOF
#include "lttng/tracepoint_gen.h"
#include "ze.h.include"
EOF

$zel_api["typedefs"].select { |t|
  t.type.kind_of?(YAMLCAst::Struct) && (struct = $zel_api["structs"].find { |s| t.type.name == s.name }) && struct.members.first.name == "stype"
}.each { |t|
  $struct_tracepoint_lambda.call(provider, t.name)
}
