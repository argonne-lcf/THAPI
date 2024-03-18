#!/usr/bin/env ruby

# Ugly hack to remove ":packet_context_field_class:" key and all it's sub
# keys from yaml input. Relies on ':event*' keys being next in the file.

def remove_packet_fields(inpath, outpath)
  in_packet_fields = false
  outfile = File.open(outpath, "w")
  File.readlines(inpath).each do |line|
    if line.strip == ":packet_context_field_class:"
      in_packet_fields = true
    elsif in_packet_fields
      if line.strip.start_with?(":event")
        in_packet_fields = false
        outfile.write(line)
      end
    else
      outfile.write(line)
    end
  end
  outfile.close
end

if __FILE__ == $PROGRAM_NAME
  remove_packet_fields(ARGV[0], ARGV[1])
end
