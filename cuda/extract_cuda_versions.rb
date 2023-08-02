require 'yaml'
require 'set'

data = nil
apis = Hash.new { |h, k| h[k] = Hash.new { |h2, k2| h2[k2] = [] } }

File.open("modified_include/cudaTypedefs.h") { |f|
  data = f.read.scan(/CUDAAPI\s+\*PFN_(\w+)_v(\d+)(_ptsz|_ptds)?/)
}

data.each { |api, version, suffix|
  apis[api][suffix].push version.to_i
}

File.open("modified_include/cudaVDPAUTypedefs.h") { |f|
  data = f.read.scan(/CUDAAPI\s+\*PFN_(\w+)_v(\d+)(_ptsz|_ptds)?/)
}

data.each { |api, version, suffix|
  apis[api][suffix].push version.to_i
}

puts YAML::dump(apis)
