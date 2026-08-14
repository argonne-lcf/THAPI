require_relative 'ompt_model'
require_relative '../../utils/gen_probe_base'
require_relative '../../utils/gen_library_base'

def to_class_name(name)
  mod = to_name_space(name)
  n = name.gsub(/_t\z/, '').gsub(/\Aomp[dt]?_/, '').split('_').collect(&:capitalize).join
  mod << n
end

def to_scoped_class_name(name)
  "OMP::#{to_class_name(name)}"
end

def to_name_space(name)
  name.match(/\A(omp[dt]?)_/)[1].upcase
end

FFI_STRUCT = 'FFI::OMPTStruct'
FFI_UNION = 'FFI::OMPTUnion'
