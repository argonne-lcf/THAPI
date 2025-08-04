require 'pp'
pp Dir["./api-v*"].sort_by { |name|
  Gem::Version.new(name.split("-v").last)
}.reject { |name|
  Gem::Version.new(name.split("-v").last) < Gem::Version.new("1.0")
}.map { |name|
  [name, Set.new(File.read(name).lines.map(&:chomp))]
}.reduce([{}, Set.new]) { |(hash, old), (name, set)|
  added = set - old
  removed = old - set
  $stderr.puts "Warning: removed symbols in #{name}: #{removed}" if !removed.empty?
  if !added.empty?
    hash[name] = added
    [hash, set]
  else
    [hash, old]
  end
}.first
