#!/usr/bin/env ruby

# Script to help set timestamps in manually edited THAPI text pretty
# format files (i.e. the babeltrace_thapi -c format).

def set_timestamps(inpath, outpath)
  outfile = File.open(outpath, "w")
  File.readlines(inpath).each_with_index do |line, line_num|
    _, tail = line.split(" - ", 2)
    outfile.write("12:00:00.%03d000000" % line_num)
    outfile.write(" - ")
    outfile.write(tail)
  end
  outfile.close
end

if __FILE__ == $PROGRAM_NAME
  set_timestamps(ARGV[0], ARGV[1])
end
