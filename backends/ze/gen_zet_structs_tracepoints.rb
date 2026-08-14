require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_zet_structs

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include "ze.h.include"
EOF

APIS[:zet].types.select do |t|
  t.type.is_a?(YAMLCAst::Struct) && (struct = APIS[:zet].structs.find do |s|
    t.type.name == s.name
  end) && struct.members.first.name == 'stype'
end.each do |t|
  print_struct_tracepoint(provider, t.name)
end
