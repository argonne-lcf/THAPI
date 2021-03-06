require_relative 'gen_ze_library_base.rb'

METHOD_PREFIX = [:put, :get, :write, :read, :put_array_of, :get_array_of, :read_array_of, :write_array_of]

def print_acessor(orig, add)
  METHOD_PREFIX.each { |meth|
    puts <<EOF
      alias_method :#{meth}_#{add}, :#{meth}_#{orig}
EOF
  }
end

puts <<EOF
module ZE
  module ZERefinements

    refine FFI::Pointer do
EOF

ze_bool = $all_types.find { |t| t.name == "ze_bool_t" }
print_acessor(ze_bool.type.name.gsub(/_t\z/,""), ze_bool.name)

$all_types.each { |t|
  if $objects.include?(t.name)
    print_acessor(:pointer, t.name)
  end
}
puts <<EOF
      if FFI.find_type(:size_t).size == 8
EOF
print_acessor(:uint64, :size_t)
puts <<EOF
      else
EOF
print_acessor(:uint32, :size_t)
puts <<EOF
      end
    end

  end
end
EOF
