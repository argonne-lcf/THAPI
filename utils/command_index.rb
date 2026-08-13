# Look commands up by name, so a backend can attach a prologue or an epilogue
# to one it built earlier:
#
#   commands = CommandIndex.new($ze_commands, $zet_commands, ...)
#   commands.add_epilogue('zeCommandListCreate', <<EOF)
#     ...
#   EOF
#
# The code lands on the command object itself rather than in a hash keyed by
# name, so it is reachable from the command and nowhere else. A name that
# matches no command raises: it is always a typo or a function that has since
# been renamed, which otherwise attaches the code to nothing at all.
#
# This lives apart from utils/command.rb because opencl builds its commands from
# the Khronos XML with a Command class of its own, and so cannot require that
# file, but indexes them just the same.
class CommandIndex
  def initialize(*command_lists)
    @by_name = {}
    command_lists.flatten.each do |c|
      raise "#{c.name} appears in two command lists" if @by_name.key?(c.name)

      @by_name[c.name] = c
    end
  end

  def [](name)
    @by_name.fetch(name) { raise "Unknown method: #{name}!" }
  end

  def add_prologue(name, code)
    self[name].add_prologue(code)
  end

  def add_epilogue(name, code)
    self[name].add_epilogue(code)
  end
end
