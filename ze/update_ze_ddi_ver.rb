require 'yaml'
require 'erb'

# ERB template for versioned DDI table structures
# trim_mode '-' removes trailing newlines
template = ERB.new <<~EOF, trim_mode: '-'
  #ifndef _ZE_DDI_VER_H
  #define _ZE_DDI_VER_H
  #if defined(__cplusplus)
  #pragma once
  #endif
  #include "ze_ddi.h"

  #if defined(__cplusplus)
  extern "C" {
  #endif

  <% dditables.values.each do |groups| -%>
  ///////////////////////////////////////////////////////////////////////////////
  /// <%= groups["versions"] %>
  <% groups["strucs"].each do |dditable| -%>
  typedef struct <%= dditable['name'] %>
  {
  <% dditable['members'].each do |member| -%>
      <%= member["type"]["name"].ljust(59) %> <%= member["name"] %>;
  <% end -%>
  } <%= dditable["name"][1..] %>;

  <% end -%>
  <% end -%>

  #if defined(__cplusplus)
  } // extern "C"
  #endif

  #endif // _ZE_DDI_VER_H
EOF


# Parse existing DDI versioned structures from YAML file
# Groups structures by their base name (stem) and tracks version numbers
# Example:
#   { "ze_device_dditable_t": # Stem
#     versions: [1.2, 1.3]
#     structs: [ze_device_dditable_t_1_2, ...]
#   }
#   ...

dditables = YAML.load(`h2yaml ./include/ze_ddi_ver.h -Iinclude/`)['structs']
# Add meta info used for the group by
                .filter_map do |struct|
  ms = /(.*_dditable_t)_(\d+)_(\d+)/.match(struct['name'])
  next unless ms

  struct['base_name'], *version = ms.captures
  struct['version'] = version.join('.').to_f # "1_3" -> 1.3
  struct
end
  # Groupy by base_name
  .group_by { |dditable| dditable['base_name'] }.to_h
  .transform_values do |dditables|
     { 'versions' => dditables.map { |y| y['version'] },
       'strucs' => dditables }
end


# Official current header
ze_ddi_yaml = YAML.load(`h2yaml ./include/ze_ddi.h -Iinclude/`)

# Extract the current API version from the official header DDI YAML file
# Looks for the latest version in the _ze_api_version_t enum
current_ze_version = ze_ddi_yaml['enums'].find do |enum|
  enum['name'] == '_ze_api_version_t'
end['members'].filter_map do |member|
  match = /_(\d+)_(\d+)/.match(member['name'])
  match && match.captures
end.last


# Find all the new members of `ze_.*_dditable` (not present in `ze_ddi_ver`),
# put them in their own versioned structure in `dditables`
ze_ddi_yaml['structs'].each do |struct|

  name = struct['name']
  next unless  /_ze_.*_dditable_t/.match?(name)
  # Initialize entry for completely new DDI table type
  dditables[name] = { 'versions' => [], 'strucs' => [] } unless dditables.include?(name)

  # Find members that are new (not present in any existing version)
  new_members = struct['members'].reject do |member|
    dditables[name]['strucs'].any? { |old_dditable| old_dditable['members'].include?(member) }
  end
  next if new_members.empty?

  # Create new versioned structure for the new members
  # Structure name format: #{base_name}_major_minor (e.g., ze_device_dditable_t_1_3)
  struct['name'] = ([name] + current_ze_version).join('_')
  struct['members'] = new_members
  dditables[name]['strucs'].append(struct)
  # Version format: major.minor as float (e.g., 1.3)
  dditables[name]['versions'].append(current_ze_version.join(',').to_f)
end

# Generate header file
puts template.result(binding)
