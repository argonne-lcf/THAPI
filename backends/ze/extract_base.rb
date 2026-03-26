require 'yaml'

def enable_clang_parser?
  ENV.fetch('ENABLE_CLANG_PARSER', '0') == '1'
end

if enable_clang_parser?

  def shared_header
    <<~EOF
      #include <stddef.h>
      #include <stdint.h>
      #include <stdbool.h>

      #define THAPI_NO_INCLUDE

      #include <ze_api.h>
      #include <ze_ddi.h>
    EOF
  end
else

  require 'cast-to-yaml'

  constants = %w[
    ZE_MAX_IPC_HANDLE_SIZE
    ZE_MAX_UUID_SIZE
    ZE_MAX_DRIVER_UUID_SIZE
    ZE_MAX_EXTENSION_NAME
    ZE_MAX_DEVICE_UUID_SIZE
    ZE_MAX_DEVICE_NAME
    ZE_SUBGROUPSIZE_COUNT
    ZE_MAX_NATIVE_KERNEL_UUID_SIZE
    ZE_MAX_KERNEL_UUID_SIZE
    ZE_MAX_MODULE_UUID_SIZE
    ZE_MAX_DEVICE_LUID_SIZE_EXT
    ZE_MAX_FABRIC_EDGE_MODEL_EXP_SIZE
    ZES_MAX_EXTENSION_NAME
    ZES_STRING_PROPERTY_SIZE
    ZES_MAX_UUID_SIZE
    ZES_MAX_FABRIC_PORT_MODEL_SIZE
    ZES_MAX_FABRIC_LINK_TYPE_SIZE
    ZES_FAN_TEMP_SPEED_PAIR_COUNT
    ZES_MAX_RAS_ERROR_CATEGORY_COUNT
    ZET_MAX_METRIC_GROUP_NAME
    ZET_MAX_METRIC_GROUP_DESCRIPTION
    ZET_MAX_METRIC_NAME
    ZET_MAX_METRIC_DESCRIPTION
    ZET_MAX_METRIC_COMPONENT
    ZET_MAX_METRIC_RESULT_UNITS
    ZET_MAX_METRIC_EXPORT_DATA_ELEMENT_NAME_EXP
    ZET_MAX_METRIC_EXPORT_DATA_ELEMENT_DESCRIPTION_EXP
    ZET_MAX_PROGRAMMABLE_METRICS_ELEMENT_NAME_EXP
    ZET_MAX_PROGRAMMABLE_METRICS_ELEMENT_DESCRIPTION_EXP
    ZET_MAX_METRIC_PROGRAMMABLE_NAME_EXP
    ZET_MAX_METRIC_PROGRAMMABLE_DESCRIPTION_EXP
    ZET_MAX_METRIC_PROGRAMMABLE_COMPONENT_EXP
    ZET_MAX_METRIC_PROGRAMMABLE_PARAMETER_NAME_EXP
    ZET_MAX_VALUE_INFO_CSTRING_EXP
    ZEL_COMPONENT_STRING_SIZE
  ]

  $parser = C::Parser.new
  $parser.type_names << '__builtin_va_list'
  $cpp = C::Preprocessor.new
  $cpp.macros['__attribute__(a)'] = ''
  $cpp.macros['__restrict'] = 'restrict'
  $cpp.macros['__extension__'] = ''
  $cpp.macros['ZE_APICALL'] = ''
  $cpp.macros['ZE_DLLEXPORT'] = ''
  $cpp.macros['ZE_APIEXPORT'] = ''
  $cpp.macros['__asm__(a)'] = ''
  $cpp.macros['THAPI_NO_INCLUDE'] = ''
  $cpp.macros['ZE_MAKE_VERSION( _major, _minor )'] = 'ZE_MAKE_VERSION( _major, _minor )'
  $cpp.macros['ZE_MAJOR_VERSION( _ver )'] = 'ZE_MAJOR_VERSION( _ver )'
  $cpp.macros['ZE_MINOR_VERSION( _ver )'] = 'ZE_MINOR_VERSION( _ver )'
  $cpp.macros['ZE_BIT( _i )'] = 'ZE_BIT( _i )'
  constants.each do |c|
    $cpp.macros[c] = c
  end
  $cpp.include_path << './modified_include/'
  begin
    preprocessed_sources_libc = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
      #include <stdint.h>
      #include <stddef.h>
    EOF
  rescue StandardError
    C::Preprocessor.command = 'gcc -E'
    preprocessed_sources_libc = $cpp.preprocess(<<~EOF).gsub(/^#.*?$/, '')
      #include <stdint.h>
      #include <stddef.h>
    EOF
  end
  $parser.parse(preprocessed_sources_libc)

end
