#!/usr/bin/env ruby

# Fix timestamps in a babeltrace_thapi -c log file

def fix_timestamps(inpath, outpath)
  outfile = File.open(outpath, "w")
  line_num = 0
  File.readlines(inpath).each do |line|
    _, tail = line.split(" - ", 2)
    outfile.write("12:00:00.%03d000000" % line_num)
    outfile.write(" - ")
    outfile.write(tail)
    line_num += 1
  end
  outfile.close
end

if __FILE__ == $PROGRAM_NAME
  fix_timestamps(ARGV[0], ARGV[1])
end
