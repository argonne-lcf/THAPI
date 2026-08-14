require_relative 'ze_model'
require_relative '../../utils/gen_probe_base'

provider = :lttng_ust_zex_structs

puts <<~EOF
  #include "lttng/tracepoint_gen.h"
  #include "ze.h.include"
EOF

APIS[:zex].types.select do |t|
  t.type.is_a?(YAMLCAst::Struct) && (struct = APIS[:zex].structs.find do |s|
    t.type.name == s.name
  end) && struct.members.first.name == 'stype'
end.each do |t|
  print_struct_tracepoint(provider, t.name)
end
