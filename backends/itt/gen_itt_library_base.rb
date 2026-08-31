require_relative 'itt_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

# ITT spells its types __itt_domain_t: a leading-underscore prefix the shared
# rule does not strip, and names that may be snake_case or camelCase.
NAMING = NamingContext.new(
  module_name: 'ITT',
  api_files: ['itt_api.yaml'],
  namespace_pattern: /\A(__itt[dt]?)_/,
  strict: true,
  upcase_namespace: true,
  class_namer: lambda { |naming, name|
    mod = naming.name_space(name).gsub(/\A_+/, '')
    mod += '::' unless mod.empty? || mod.end_with?('::')

    base = name.to_s
      .sub(/_t\z/i, '')
      .sub(/\A_+/, '')
      .sub(/\A__?itt[dt]?_/i, '')
    base = if base.include?('_')
             base.split('_').map(&:capitalize).join
           elsif base[0] =~ /[a-z]/
             base[0].upcase + base[1..]
           else
             base
           end
    base = "ITT#{base}" unless base =~ /\A[A-Z]/
    mod + base
  }
)
