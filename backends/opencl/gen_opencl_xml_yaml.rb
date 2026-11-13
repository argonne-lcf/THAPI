# What the shared code generators need about OpenCL and the headers cannot say,
# read from the Khronos registry (cl.xml.patched):
#
#   functions        the functions traced, in registry order: `core` ones are
#                    found with dlsym, `extension` ones through
#                    clGetExtensionFunctionAddress.
#   enums, bitfields the value of each constant of the integer types listed in
#                    supported_enums.yaml (`type_name` when the registry groups
#                    them under another name), grouped as the registry's
#                    <require comment="<type>[ - bitfield]"> blocks group them
#
# Usage: gen_opencl_xml_yaml.rb cl.xml.patched opencl_api.yaml > opencl_xml.yaml
require 'nokogiri'
require 'yaml'

SRC_DIR = ENV.fetch('SRC_DIR', '.')
xml_path, api_path = ARGV

WINDOWS = /D3D|DX9/
VENDOR_EXT = /QCOM$|INTEL$|ARM$|APPLE$|IMG$|OCLICD$|PoCL$/

# Traced through clGetExtensionFunctionAddress(ForPlatform), per platform: the
# ICD loader exports no extension function, and the platform-less lookup at load
# can return NULL even where the platform provides it.
SUPPORTED_EXTENSIONS = YAML.load_file(File.join(SRC_DIR, 'supported_extensions.yaml'))

# Not looked up when the tracer loads. The clIcd*KHR are called by the loader on
# the drivers (cl_khr_icd), and the layer functions are the loader's.
ICD_AND_LAYER_FUNCTIONS = %w[
  clIcdGetPlatformIDsKHR clIcdGetFunctionAddressForPlatformKHR clIcdSetPlatformDispatchDataKHR
  clGetLayerInfo clInitLayer
].freeze

doc = Nokogiri::XML(File.read(xml_path))
commands = doc.xpath('//commands/command').to_h { |c| [c.at_xpath('proto/name').text, c] }

# Functions only experimental extensions require: the headers declare them only
# with CL_ENABLE_BETA_EXTENSIONS, and their signatures change between revisions.
experimental, final = doc.xpath('//extensions/extension').partition do |e|
  e['experimental'] == 'true' || e['provisional'] == 'true'
end
required = ->(nodes) { nodes.flat_map { |n| n.xpath('require/command').map { |c| c['name'] } } }
experimental_functions = required[experimental] - required[final] - required[doc.xpath('//feature')]

not_core = ICD_AND_LAYER_FUNCTIONS + SUPPORTED_EXTENSIONS + experimental_functions
core = commands.keys.reject { |n| n.match?(VENDOR_EXT) || n.match?(WINDOWS) || not_core.include?(n) }
extension = commands.keys & SUPPORTED_EXTENSIONS
unknown = SUPPORTED_EXTENSIONS - commands.keys
raise "supported_extensions.yaml names functions cl.xml does not: #{unknown.join(', ')}" unless unknown.empty?

# The constants of each type of supported_enums.yaml: the registry gives a
# type's constants in the <require> blocks whose comment names the type.
constants = doc.xpath('//enums/enum').to_h do |n|
  [n['name'], n['value'] || (n['bitpos'] && "(1 << #{n['bitpos']})")]
end
requires = doc.xpath('//feature/require').to_a + doc.xpath('//extensions/extension/require').to_a
enums = {}
bitfields = {}
YAML.load_file(File.join(SRC_DIR, 'supported_enums.yaml')).each do |e|
  matching = requires.select { |r| r['comment']&.match(/#{Regexp.escape(e['name'])}(\z| )/) }
  raise "supported_enums.yaml: no <require comment> of cl.xml names #{e['name']}" if matching.empty?

  kind = matching.any? { |r| r['comment'].match(/ - bitfield/) } ? bitfields : enums
  kind[e['type_name'] || e['name']] = matching.flat_map { |r| r.search('enum').map { |c| c['name'] } }
                                              .to_h { |v| [v, constants[v]] }
end

# The headers give the shared generators their signatures, so every traced
# function needs a header declaration.
undeclared = core + extension - YAML.load_file(api_path)['functions'].map { |f| f['name'] }
raise "traced but declared by no header: #{undeclared.join(', ')}" unless undeclared.empty?

puts({ 'functions' => { 'core' => core, 'extension' => extension },
       'enums' => enums, 'bitfields' => bitfields }.to_yaml)
