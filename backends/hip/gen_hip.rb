require_relative 'hip_model'
require_relative '../../utils/gen_tracer_base'

puts <<~EOF
  #include <pthread.h>
  #include <sys/mman.h>
  #include <string.h>
  #include "hip_tracepoints.h"
EOF

COMMANDS.each do |c|
  puts "#define #{HIP_POINTER_NAMES[c]} #{c.pointer_name}"
end

COMMANDS.each do |c|
  puts <<~EOF

    #{c.decl_pointer(c.pointer_type_name)};
    static #{c.pointer_type_name} #{HIP_POINTER_NAMES[c]} = (void *) 0x0;
  EOF
end

puts <<~EOF

  static void find_hip_symbols(void * handle, int verbose) {
EOF

COMMANDS.each do |c|
  puts <<EOF

  #{HIP_POINTER_NAMES[c]} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");
  if (!#{HIP_POINTER_NAMES[c]} && verbose)
	fprintf(stderr, "Missing symbol #{c.name}!\\n");
EOF
end

puts <<~EOF
  }

EOF

puts File.read(File.join(SRC_DIR, 'tracer_hip_helpers.include.c'))

normal_wrapper = lambda { |c, provider|
  print_wrapper(c, init: ('_init_tracer();' if c.init?)) { print_traced_body(c, provider, HIP_POINTER_NAMES) }
}

COMMANDS.each do |c|
  normal_wrapper.call(c, :lttng_ust_hip)
end
