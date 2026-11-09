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
#   meta_parameters  every function's meta-parameter rows: the ones OpenCL's
#                    naming conventions imply (event_wait_list, errcode_ret,
#                    param_value ...), then the hand-written ones of
#                    opencl_meta_parameters.yaml, then the queried param_name
#                    of the clGet*Info functions.
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
ABSENT_FUNCTIONS = /\A(?:#{(ICD_AND_LAYER_FUNCTIONS + SUPPORTED_EXTENSIONS).join('|')})\z/
SUPPORTED_EXTENSION_FUNCTIONS = /#{SUPPORTED_EXTENSIONS.join('|')}/

doc = Nokogiri::XML(File.read(xml_path))
commands = doc.xpath('//commands/command').to_h { |c| [c.at_xpath('proto/name').text, c] }

# Functions only experimental extensions require: the headers declare them only
# with CL_ENABLE_BETA_EXTENSIONS, and their signatures change between revisions.
experimental, final = doc.xpath('//extensions/extension').partition do |e|
  e['experimental'] == 'true' || e['provisional'] == 'true'
end
required = ->(nodes) { nodes.flat_map { |n| n.xpath('require/command').map { |c| c['name'] } } }
experimental_functions = required[experimental] - required[final] - required[doc.xpath('//feature')]

core = commands.keys.reject do |n|
  n.match(VENDOR_EXT) || n.match(ABSENT_FUNCTIONS) || n.match(WINDOWS) || experimental_functions.include?(n)
end
extension = commands.keys.grep(SUPPORTED_EXTENSION_FUNCTIONS)

# The base types a typedef chain ends in, and the handles (opaque objects).
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
  values = matching.flat_map { |r| r.search('enum').map { |c| c['name'] } }.to_h { |v| [v, constants[v]] }
  group = { 'values' => values }
  group['trace_name'] = e['trace_name'] if e['trace_name']
  group['type_name'] = e['type_name'] if e['type_name']
  (matching.any? { |r| r['comment'].match(/ - bitfield/) } ? bitfields : enums)[e['name']] = group
end

# The registry's text of a node's children, one space between them, with the
# <name> child spelled by the block (or dropped when it returns nil). A whole
# declaration keeps the registry's trailing space (`clUnloadCompiler(void )`).
def spelled(node, strip: true)
  text = node.children.filter_map { |c| c.name == 'name' ? yield(c.text) : c.text }.join(' ').squeeze(' ')
  strip ? text.strip : text
end

def param_declaration(param)
  decl = spelled(param, strip: false) { |name| name }
  type = param.search('type').text
  type += '*' if decl.match?(/\*\*/)
  type += '*' if decl.match?(/\[\]/)
  {
    'name' => param.at_xpath('name')&.text,
    'decl' => decl,
    'type' => spelled(param) { nil },
    'registry_type' => type,
    'pointer' => param.children.take_while { |c| c.name != 'name' }.any? { |c| c.text.include?('*') },
    'callback' => param.text.include?('CL_CALLBACK'),
  }
end

declarations = (core + extension).to_h do |n|
  proto = commands[n].at_xpath('proto')
  params = commands[n].xpath('param').map { |p| param_declaration(p) }
  decl = spelled(proto, strip: false) { |name| "CL_API_CALL #{name}" }
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
api = YAML.load_file(api_path)['functions'].to_h { |f| [f['name'], (f['params'] || []).map { |p| p['name'] }] }
declarations.each do |n, d|
  raise "#{n} is traced but no header declares it" unless api.key?(n)

  xml_names = d['params'].filter_map { |p| p['name'] }
  raise "#{n}: parameters #{xml_names.inspect} in cl.xml but #{api[n].inspect} in the headers" if api[n].size != xml_names.size
end

# The meta-parameters OpenCL's naming conventions imply, in the order the
# tracepoints list them.
def implied_rows(name, params)
  has = ->(n, pointer: false) { params.any? { |p| p['name'] == n && (!pointer || p['pointer'] || p['callback']) } }
  rows = []
  rows << %w[InArray event_wait_list num_events_in_wait_list] if has.call('event_wait_list')
  rows << ['OutScalar', 'errcode_ret', { 'nocheck' => true }] if has.call('errcode_ret', pointer: true)
  if has.call('param_value_size_ret', pointer: true)
    rows << ['OutScalar', 'param_value_size_ret', { 'nocheck' => true }]
  end
  rows << %w[OutArray param_value param_value_size] if has.call('param_value') && name != 'clSetKernelExecInfo'
  rows << %w[OutScalar event] if has.call('event', pointer: true)
  rows
end

hand = YAML.load_file(File.join(SRC_DIR, 'opencl_meta_parameters.yaml'))['meta_parameters']
unknown = hand.keys - declarations.keys
raise "opencl_meta_parameters.yaml names untraced functions: #{unknown.join(', ')}" unless unknown.empty?

meta_parameters = {}
declarations.each do |n, d|
  rows = implied_rows(n, d['params']) + hand.fetch(n, [])
  if n.match(/clGet(\w*?)Info/) && (pn = d['params'].find { |p| p['name'] == 'param_name' })
    rows << ['OutLTTng', 'param_name', 'ctf_integer_hex', pn['registry_type'], '_param_name', 'param_name']
  end
  meta_parameters[n] = rows unless rows.empty?
end

puts({ 'functions' => { 'core' => core, 'extension' => extension },
       'enums' => enums, 'bitfields' => bitfields, 'type_map' => type_map,
       'structures' => doc.xpath('//types/type').select { |l| l['category'] == 'struct' }.map { |l| l['name'] },
       'int_scalars' => CL_INT_SCALARS, 'float_scalars' => CL_FLOAT_SCALARS, 'objects' => CL_OBJECTS,
       'declarations' => declarations,
       'meta_parameters' => meta_parameters }.to_yaml)
