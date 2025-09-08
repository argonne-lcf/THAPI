require 'yaml'
require 'set'
apis = Hash.new { |h, k| h[k] = Hash.new { |h2, k2| h2[k2] = [] } }

ARGV.each do |path|
  File.open(path) do |f|
    f.read.scan(/CUDAAPI\s+\*PFN_(\w+)_v(\d+)(_ptsz|_ptds)?/).each do |api, version, suffix|
      apis[api][suffix].push version.to_i
    end
  end
end

apis.each { |_api, suffixes| suffixes.each { |_suffix, versions| versions.sort!.reverse! } }

puts YAML.dump(apis)
