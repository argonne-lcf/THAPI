require 'optparse'
class OptionParserWithDefaultAndValidation < OptionParser

  def initialize()
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
    key = switch.long.first.gsub(/^--(?:\[no-\])?/,'').to_sym
    # Save the default value
    @defaults[key] = @tmp_default unless @tmp_default.nil?
    switch
  end

  def on(*opts, default: nil, allowed:nil, &block)
    # Save the default, will be used in `refine`.
    @tmp_default = default

    # Create a new block where
    #   we append our validation layer before the usr block
    new_block = Proc.new { |a|
      raise(OptionParser::ParseError, "; Wrong argument: #{a} only #{allowed} allowed") unless allowed.nil? || allowed.include?(a) 
      block_given? ? block.yield(a) : a
    }
    return super(*opts, &new_block)
  end
end

