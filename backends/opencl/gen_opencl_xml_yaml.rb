# What the shared code generators need about OpenCL and the headers cannot say,
# read from the Khronos registry (cl.xml.patched):
#
#   functions        the functions traced, in registry order: `core` ones are
#                    found with dlsym, `extension` ones through
#                    clGetExtensionFunctionAddress.
#   declarations     how the registry spells each traced function's C
#                    declaration (CL_API_ENTRY, CL_API_CALL, CL_CALLBACK ...),
#                    which the tracer's wrappers repeat word for word: the
#                    whole declaration, the pointer typedef with `%s` for its
#                    name, the return type, and each parameter with and
#                    without its name.
#   enums, bitfields the value of each constant of the integer types listed in
#                    supported_enums.yaml, grouped as the registry's <require
#                    comment="<type>[ - bitfield]"> blocks group them
#   type_map         each typedef, mapped to the base cl_*/C type it names
#   structures       the struct types
#   int_scalars, float_scalars, objects
#                    the base types typedef chains end in, and the handles
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

# The base types a typedef chain ends in, and the handles (opaque objects), in
# the order opencl_model.yaml lists them. Which types are handles is checked
# against the registry below.
CL_INT_SCALARS = ['unsigned int', 'int', 'uintptr_t', 'intptr_t', 'size_t', 'cl_int', 'cl_uint', 'cl_long', 'cl_ulong',
                  'cl_short', 'cl_ushort', 'cl_char', 'cl_uchar'].freeze
CL_FLOAT_SCALARS = %w[cl_half cl_float cl_double].freeze
CL_OBJECTS = %w[cl_platform_id cl_device_id cl_context cl_command_queue cl_mem cl_program cl_kernel
                cl_event cl_sampler cl_GLsync CLeglImageKHR CLeglDisplayKHR CLeglSyncKHR cl_accelerator_intel
                cl_semaphore_khr].freeze
CL_SCALARS = CL_INT_SCALARS + CL_FLOAT_SCALARS

typedefs = doc.xpath('//types/type').select { |l| l['category'] == 'define' && l.search('type').size > 0 }
type_map = typedefs.to_h { |l| [l.search('name').text, l.search('type').text] }
type_map.reject! { |k, _| (CL_SCALARS + CL_OBJECTS).include?(k) }
type_map.transform_values! do |v|
  v = type_map[v] until CL_SCALARS.include?(v) || v.nil?
  v or raise 'Failed to achieve transitive closure!'
end

constants = doc.xpath('//enums/enum').to_h do |n|
  [n['name'], n['value'] || (n['bitpos'] && "(1 << #{n['bitpos']})")]
end
requires = doc.xpath('//feature/require').to_a + doc.xpath('//extensions/extension/require').to_a
groups = YAML.load_file(File.join(SRC_DIR, 'supported_enums.yaml'))
groups.push({ 'name' => 'cl_bool' })
groups.push({ 'name' => 'command execution status', 'trace_name' => 'command_exec_callback_type',
              'type_name' => 'cl_command_execution_status' })
enums = {}
bitfields = {}
groups.each do |e|
  matching = requires.select { |r| r['comment']&.match(/#{Regexp.escape(e['name'])}(\z| )/) }
  raise "supported_enums.yaml: no <require comment> of cl.xml names #{e['name']}" if matching.empty?

  values = matching.flat_map { |r| r.search('enum').map { |c| c['name'] } }.to_h { |v| [v, constants[v]] }
  group = { 'values' => values }
  group['trace_name'] = e['trace_name'] if e['trace_name']
  group['type_name'] = e['type_name'] if e['type_name']
  (matching.any? { |r| r['comment'].match(/ - bitfield/) } ? bitfields : enums)[e['name']] = group
end

# Whether the registry declares `node` (a <param> or a typedef <type>) as a
# pointer: a `*` before its <name>.
def pointer_before_name?(node)
  node.children.take_while { |c| c.name != 'name' }.any? { |c| c.text.include?('*') }
end

# The registry's text of a node's children, one space between them, with the
# <name> child spelled by the block (or dropped when it returns nil).
def spelled(node)
  node.children.filter_map { |c| c.name == 'name' ? yield(c.text) : c.text }.join(' ').squeeze(' ').strip
end

def param_declaration(param)
  decl = spelled(param) { |name| name }
  type = param.search('type').text
  type += '*' if decl.match?(/\*\*/)
  type += '*' if decl.match?(/\[\]/)
  {
    'name' => param.at_xpath('name')&.text,
    'decl' => decl,
    'type' => spelled(param) { nil },
    'registry_type' => type,
    'pointer' => pointer_before_name?(param),
    'callback' => param.text.include?('CL_CALLBACK'),
  }
end

declarations = (core + extension).to_h do |n|
  proto = commands[n].at_xpath('proto')
  params = commands[n].xpath('param').map { |p| param_declaration(p) }
  decl = spelled(proto) { |name| "CL_API_CALL #{name}" }
  pointer = spelled(proto) { '(CL_API_CALL *%s)' }
  # C spells "no parameters" `(void)`; the registry lists none (clUnloadCompiler).
  list = ->(key) { params.empty? ? 'void' : params.map { |p| p[key] }.join(', ') }
  [n, {
    'decl' => "CL_API_ENTRY #{decl}(#{list.call('decl')})",
    'decl_pointer' => "CL_API_ENTRY #{pointer}(#{list.call('type')})",
    'return_type' => spelled(proto) { nil },
    'params' => params,
  }]
end

# The headers, not the registry, give the shared generators their signatures
# (the registry only renames the parameters, see opencl_model.rb), so every
# traced function needs a header declaration with as many parameters.
api_yaml = YAML.load_file(api_path)
api = api_yaml['functions'].to_h { |f| [f['name'], (f['params'] || []).map { |p| p['name'] }] }
declarations.each do |n, d|
  raise "#{n} is traced but no header declares it" unless api.key?(n)
  next if api[n].size == d['params'].size

  raise "#{n}: parameters #{d['params'].map { |p| p['name'] }} in cl.xml but #{api[n]} in the headers"
end

# The handles are the registry's pointer typedefs (`struct _cl_foo *`,
# `void *`) that the headers declare.
header_types = api_yaml['typedefs'].map { |t| t['name'] }
handles = doc.xpath('//types/type').filter_map do |t|
  name = t.at_xpath('name')&.text
  name if t['category'] == 'define' && header_types.include?(name) && pointer_before_name?(t)
end
raise "CL_OBJECTS must list exactly the handles: #{handles.sort.join(' ')}" if handles.sort != CL_OBJECTS.sort

puts({ 'functions' => { 'core' => core, 'extension' => extension },
       'enums' => enums, 'bitfields' => bitfields, 'type_map' => type_map,
       'structures' => doc.xpath('//types/type').select { |l| l['category'] == 'struct' }.map { |l| l['name'] },
       'int_scalars' => CL_INT_SCALARS, 'float_scalars' => CL_FLOAT_SCALARS, 'objects' => CL_OBJECTS,
       'declarations' => declarations }.to_yaml)
