require_relative 'hip_model'

puts <<~EOF
  #include <pthread.h>
  #include <sys/mman.h>
  #include <string.h>
  #include "hip_tracepoints.h"
EOF

$hip_commands.each do |c|
  puts "#define #{HIP_POINTER_NAMES[c]} #{c.pointer_name}"
end

$hip_commands.each do |c|
  puts <<~EOF

    #{c.decl_pointer(c.pointer_type_name)};
    static #{c.pointer_type_name} #{HIP_POINTER_NAMES[c]} = (void *) 0x0;
  EOF
end

puts <<~EOF

  static void find_hip_symbols(void * handle, int verbose) {
EOF

$hip_commands.each do |c|
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

common_block = lambda { |c, provider|
  if c.parameters
    params = c.parameters.collect(&:name)
    tp_params = c.parameters.collect do |p|
      if p.type.is_a?(YAMLCAst::Pointer) && p.type.type.is_a?(YAMLCAst::Function)
        '(void *)(intptr_t)' + p.name
      else
        p.name
      end
    end
  else
    params = []
    tp_params = []
  end
  c.tracepoint_parameters.each do |p|
    puts "  #{p.type} #{p.name};"
  end
  c.tracepoint_parameters.each do |p|
    puts p.init unless p.after?
  end
  tracepoint_params = c.tracepoint_parameters.filter_map { |p| p.name unless p.after? }
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{START}, #{(tp_params + tracepoint_params).join(', ')});
EOF
  tracepoint_params = c.tracepoint_parameters.collect(&:name)

  c.prologues.each do |p|
    puts p
  end

  if c.has_return_type?
    puts <<EOF
  #{c.type} _retval;
  _retval = #{HIP_POINTER_NAMES[c]}(#{params.join(', ')});
EOF
  else
    puts "  #{HIP_POINTER_NAMES[c]}(#{params.join(', ')});"
  end
  c.tracepoint_parameters.each do |p|
    puts p.init if p.after?
  end
  c.epilogues.each do |e|
    puts e
  end
  tp_params.push '_retval' if c.has_return_type?
  puts <<EOF
  tracepoint(#{provider}, #{c.name}_#{STOP}, #{(tp_params + tracepoint_params).join(', ')});
EOF
}

normal_wrapper = lambda { |c, provider|
  puts <<~EOF
    #{c.decl} {
  EOF
  if c.init?
    puts <<EOF
  _init_tracer();
EOF
  end
  common_block.call(c, provider)
  if c.has_return_type?
    puts <<EOF
  return _retval;
EOF
  end
  puts <<~EOF
    }

  EOF
}

$hip_commands.each do |c|
  normal_wrapper.call(c, :lttng_ust_hip)
end
