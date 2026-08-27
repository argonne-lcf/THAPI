# A backend's commands, indexed by name and kept in the groups they were built
# in -- one per LTTng provider for most backends, core versus extension for
# opencl -- because generators want both shapes.
#
# Looking up a name that matches no command raises: it is always a typo or a
# renamed function, which would otherwise attach its code to nothing at all.
#
# This lives apart from utils/command.rb because opencl builds its commands
# from the Khronos XML with a Command class of its own, and so cannot require
# that file, but indexes them just the same.
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

  def each(&block)
    @by_name.each_value(&block)
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
