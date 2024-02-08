require 'optparse'
class OptionParserWithDefaultAndValidation < OptionParser
  def initialize
    # Dictionary to save the default values passed by `on`
    @defaults = {}
    super
  end

  def parse!(argv = default_argv, into: nil)
    # Merge the default value with the `into` dict
    into.merge!(@defaults) unless into.nil?
    # This will populate the `into` dict (overwriting our default if needed)
    super
  end

  def define(*opts, &block)
    switch = super
    # Parse the long name to get the key / name of the option
    #   Should be able to get it with some instrospection, no idea how
    # At least `define` already did the split of long, short, and arguments of opts
    key = switch.long.first.gsub(/^--(?:\[no-\])?/, '').to_sym
    # Save the default value
    @defaults[key] = @tmp_default unless @tmp_default.nil?
    switch
  end

  def on(*opts, default: nil, allowed: nil, &block)
    # Save the default, will be used in `refine`.
    @tmp_default = default
    opts << "Values allowed: #{allowed}" unless allowed.nil?
    # Default can containt Array (or Set).
    # We cannot use the [a].flatten trick.
    # So we use the respond_to one
    #   irb(main):028:0> default = Set[-1,2]
    #   => #<Set: {-1, 2}>
    #   irb(main):029:0> [default].flatten.join(',')
    #   => "#<Set: {-1, 2}>"
    #   irb(main):030:0> default.respond_to?(:flatten)? default.flatten.join(',') : default
    #   => "-1,2"
    opts << "Default: #{default.respond_to?(:flatten) ? default.flatten.join(',') : default}" unless default.nil?
    # Create a new block where
    #   we append our validation layer before the usr block
    new_block = proc do |a|
      unless allowed.nil? || allowed.include?(a)
        raise(OptionParser::ParseError,
              "; Wrong argument: #{a} only #{allowed} allowed")
      end

      block_given? ? block.yield(a) : a
    end
    super(*opts, &new_block)
  end
end
