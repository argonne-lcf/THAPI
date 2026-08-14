# A backend's commands, built once and published as one constant:
#
#   COMMANDS = CommandIndex.new(lttng_ust_ze: [...], lttng_ust_zet: [...])
#   COMMANDS.add_epilogue('zeCommandListCreate', <<EOF)
#     ...
#   EOF
#
# Commands are built in groups -- one per LTTng provider for most backends, core
# versus extension for opencl -- and both shapes are wanted downstream, so both
# are served from here rather than from a second constant. `each` walks every
# command, for the sweeps that attach code to whichever ones match a shape;
# `groups` hands the groups back, for the generators that emit one file per
# provider.
#
# The code attached by name lands on the command object itself rather than in a
# hash keyed by name, so it is reachable from the command and nowhere else. A
# name that matches no command raises: it is always a typo or a function that
# has since been renamed, which otherwise attaches the code to nothing at all.
#
# This lives apart from utils/command.rb because opencl builds its commands from
# the Khronos XML with a Command class of its own, and so cannot require that
# file, but indexes them just the same.
class CommandIndex
  include Enumerable

  attr_reader :groups

  def initialize(groups)
    @groups = groups.freeze
    @by_name = {}
    @groups.each_value do |commands|
      commands.each do |c|
        raise "#{c.name} appears in two command lists" if @by_name.key?(c.name)

        @by_name[c.name] = c
      end
    end
  end

  # Iterating yields every command across the groups, in the order they were
  # given, so the index is also the handle for the sweeps that attach code to
  # whichever commands match a shape.
  def each(&)
    @by_name.each_value(&)
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
