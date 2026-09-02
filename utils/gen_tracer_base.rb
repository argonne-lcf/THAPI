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

# `pointer_names` maps a command to the symbol holding the address of the real
# function, which is the only part of this that is per-backend. A backend whose
# body does more than this -- ze walks pNext chains between these steps --
# writes its own rather than growing a flag here.
def print_traced_body(c, provider, pointer_names)
  call_args = tracepoint_call_args(c)
  print_tracepoint_locals(c)
  print_tracepoint_call(provider, c, :start, call_args)

  c.prologues.each { |p| puts p }

  print_traced_call(c, pointer_names[c])

  c.tracepoint_parameters.each { |p| puts p.init if p.after? }
  c.epilogues.each { |e| puts e }

  call_args.push '_retval' if c.has_return_type?
  print_tracepoint_call(provider, c, :stop, call_args)
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
