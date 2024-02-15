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
    @defaults[switch.switch_name.to_sym] = @tmp_default unless @tmp_default.nil?
    switch
  end

  def on(*opts, default: nil, allowed: nil, &block)
    # Save the default, will be used in `refine`.
    @tmp_default = default
    opts << "Values allowed: #{allowed}" unless allowed.nil?
    # Default can containt Array (or Set).
    # We cannot use the [a].flatten trick.
    # So we use the respond_to join
    opts << "Default: #{default.respond_to?(:join) ? default.join(',') : default}" unless default.nil?
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
