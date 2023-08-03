require 'yaml'
require 'set'

data = nil
apis = Hash.new { |h, k| h[k] = Hash.new { |h2, k2| h2[k2] = [] } }

ARGV.each { |path|
  File.open(path) { |f|
    f.read.scan(/CUDAAPI\s+\*PFN_(\w+)_v(\d+)(_ptsz|_ptds)?/).each { |api, version, suffix|
      apis[api][suffix].push version.to_i
    }
  }
}

puts YAML::dump(apis)
