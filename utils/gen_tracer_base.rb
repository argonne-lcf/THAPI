# The interposed C wrapper THAPI puts in front of a traced function: fire an
# entry tracepoint, call the real function, fire an exit tracepoint. Six
# backends generate one, and these are the parts they generate the same way.

require_relative 'gen_probe_base'

# The traced function's own arguments, as the tracepoint macro must receive
# them. An array parameter decays to a pointer, and a function pointer goes
# through intptr_t because the macro stores it as an address.
def tracepoint_call_args(c)
  return [] unless c.parameters

  c.parameters.collect do |p|
    if p.type.is_a?(YAMLCAst::Pointer) && p.type.type.is_a?(YAMLCAst::Function)
      '(void *)(intptr_t)' + p.name
    elsif p.type.to_s.match(/\[.*\]/)
      "(#{p.type.to_s.gsub(/\[.*\]/, '*')}) #{p.name}"
    else
      p.name
    end
  end
end

# A tracepoint parameter is a value the tracer computes itself rather than one
# the caller passed. An `after?` one describes the result, so it cannot be
# initialised until the call has returned.
def print_tracepoint_locals(c)
  c.tracepoint_parameters.each { |p| puts "  #{p.type} #{p.name};" }
  c.tracepoint_parameters.each { |p| puts p.init unless p.after? }
end

# The entry event leaves out the `after?` parameters: they describe the result,
# which does not exist yet.
def print_tracepoint_call(provider, c, dir, call_args)
  name = tracepoint_event_name(c, dir)
  locals = c.tracepoint_parameters
  locals = locals.reject(&:after?) unless dir == :stop
  puts "  tracepoint(#{provider}, #{name}, #{(call_args + locals.collect(&:name)).join(', ')});"
end

# The return value goes in `_retval` because both the exit tracepoint and the
# wrapper's own `return` read it.
#
# `declare_retval` is false when the caller has already declared `_retval`
# itself -- ze's ProcAddrTable getters do, because their epilogue rewrites the
# table the call just filled in.
def print_traced_call(c, target, declare_retval: true)
  args = (c.parameters || []).collect(&:name).join(', ')
  if c.has_return_type?
    puts "  #{c.type} _retval;" if declare_retval
    puts "  _retval = #{target}(#{args});"
  else
    puts "  #{target}(#{args});"
  end
end

# The body of an interposed wrapper: fire the entry event, run the prologues,
# call the real function, run the epilogues, fire the exit event. Every backend
# generates this shape; the arguments below are where they differ.
#
# `pointer_names` maps a command to the symbol holding the address of the real
# function.
#
# `epilogues` says whether a command's epilogue runs before the exit tracepoint
# or after it. Before is what five backends want: an epilogue that rewrites the
# result must run while the tracepoint can still see the rewrite. mpi's single
# epilogue instead fires a second tracepoint of its own, which belongs after
# the exit event rather than between it and the call.
#
# `after_entry` and `after_exit` are extra text to emit just inside each
# tracepoint, for a backend with something to say about the call's own
# arguments: ze walks the pNext chain of an extension struct it was handed,
# which is only meaningful before the call for an input and after it for an
# output.
#
# `declare_retval` is false for a command whose prologue has already declared
# `_retval` -- ze's ProcAddrTable getters do, because their prologue rewrites
# the table the call is about to fill in.
def print_traced_body(c, provider, pointer_names, epilogues: :before_exit,
                      after_entry: nil, after_exit: nil, declare_retval: true)
  call_args = tracepoint_call_args(c)
  print_tracepoint_locals(c)
  print_tracepoint_call(provider, c, :start, call_args)
  after_entry&.call(c)

  c.prologues.each { |p| puts p }

  print_traced_call(c, pointer_names[c], declare_retval: declare_retval)

  c.tracepoint_parameters.each { |p| puts p.init if p.after? }
  c.epilogues.each { |e| puts e } if epilogues == :before_exit

  call_args.push '_retval' if c.has_return_type?
  print_tracepoint_call(provider, c, :stop, call_args)
  after_exit&.call(c)

  c.epilogues.each { |e| puts e } if epilogues == :after_exit
end

# The function-pointer table the dlsym lookups fill in: for each traced
# function, the pointer typedef and the variable holding the real function.
#
# `initializer` is what the variable starts as. It defaults to a null the
# lookup overwrites; cuda instead starts each pointer at a trampoline that
# initializes the tracer on first call, so it passes one per command.
#
# `before` and `after` are extra text to emit around the declaration, for a
# backend whose table entry is more than the pointer itself: cuda wraps each
# one in the two stubs that pointer can name.
#
# `blank` says which side of an entry its separating blank line falls on. Every
# backend separates entries the same way; mpi's happens to lead with the
# declaration rather than the blank, which is visible here rather than as a
# loop of its own.
def print_pointer_table(commands, pointer_names, initializer: ->(_c) { '(void *) 0x0' },
                        before: nil, after: nil, blank: :before)
  commands.each do |c|
    puts if blank == :before
    puts before.call(c) if before
    puts "#{c.decl_pointer(c.pointer_type_name)};"
    puts "static #{c.pointer_type_name} #{pointer_names[c]} = #{initializer.call(c)};"
    puts after.call(c) if after
    puts if blank == :after
  end
end

# The dlsym lookups that fill in a backend's function-pointer table: one per
# traced function, each reporting a symbol the loaded library does not export.
#
# `prefix` is what the diagnostic is tagged with, and `indent` how far its
# fprintf is indented -- both are as each backend's generated tracer already
# spells them.
#
# `fallback` names a stub to install when the symbol is missing, and changes
# the shape of the check: cuda points every unresolved symbol at a wrapper that
# reports CUDA_ERROR_NOT_SUPPORTED, so its lookup cannot leave a null behind
# and the verbose report moves inside. The backends that pass nothing leave the
# pointer null and let the caller check it.
def print_dlsym_lookups(commands, pointer_names, prefix: '', indent: '    ', fallback: nil)
  commands.each do |c|
    ptr = pointer_names[c]
    report = %(fprintf(stderr, "#{prefix}Missing symbol #{c.name}!\\n");)
    puts
    puts %(  #{ptr} = (#{c.pointer_type_name})(intptr_t)dlsym(handle, "#{c.name}");)
    if fallback
      puts "  if (!#{ptr}) {"
      puts "    #{ptr} = &#{fallback.call(c)};"
      puts '    if (verbose)'
      puts "      #{report}"
      puts '  }'
    else
      puts "  if (!#{ptr} && verbose)"
      puts "#{indent}#{report}"
    end
  end
end

# `storage` is 'static ' for a wrapper that is not itself the exported symbol.
# `init` is the tracer's one-time setup call, for a backend that needs one
# before the traced function can run.
def print_wrapper(c, storage: nil, init: nil)
  puts "#{storage}#{c.decl} {"
  puts "  #{init}" if init

  yield

  puts '  return _retval;' if c.has_return_type?
  puts '}'
  puts ''
end
