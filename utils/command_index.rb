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

  # The symbol each command's real function pointer is stored in.
  #
  # Five backends spell it by upper-snake-casing the pointer name, which is
  # what `naming` does by default. The two exceptions are properties of their
  # API, stated where they hold: MPI already spells its functions in snake_case
  # so upcasing is enough, and the commands reached through libffi rather than
  # dlsym (ze's zex, opencl's extensions) keep the header's own spelling.
  #
  # `pointer_name_of` reads the name off a command, because opencl's Command
  # class keeps it on a prototype rather than on the command itself.
  def pointer_names(naming: method(:upper_snake_case), pointer_name_of: :pointer_name.to_proc, &exception)
    to_h do |c|
      name = pointer_name_of.call(c)
      [c, exception&.call(c, name) || naming.call(name)]
    end
  end

  def add_prologue(name, code)
    self[name].add_prologue(code)
  end

  def add_epilogue(name, code)
    self[name].add_epilogue(code)
  end
end

# The macro that lets hand-written tracer C call a traced function through the
# same symbol the generated code uses.
#
# Takes any list of commands, because a backend defines these for the subset it
# reaches by dlsym rather than always for the whole index.
#
# `pointer_name_of` reads the name off a command, because opencl's Command
# class keeps it on a prototype rather than on the command itself.
def print_pointer_defines(commands, pointer_names, pointer_name_of: :pointer_name.to_proc)
  commands.each { |c| puts "#define #{pointer_names[c]} #{pointer_name_of.call(c)}" }
end
