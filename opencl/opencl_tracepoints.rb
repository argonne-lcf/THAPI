def print_tracepoint(namespace, tp, dir)
  puts <<EOF
TRACEPOINT_EVENT(
  #{namespace},
  #{tp["name"]}_#{dir},
  TP_ARGS(
EOF
  print "    "
  puts tp["args"].collect { |a| a.join(", ") }.join(",\n    ")
  puts <<EOF
  ),
  TP_FIELDS(
EOF
  if tp[dir]
    print "    "
    puts tp[dir].collect { |(f, *args)| "#{f}(#{args.join(", ")})" }.join("\n    ")
  end
  puts <<EOF
  )
)

EOF
end
