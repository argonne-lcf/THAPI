arg_count = ARGV[0].to_i

puts <<EOF

#undef LTTNG_STAP_PROBEV
#define LTTNG_STAP_PROBEV(...)
#undef LTTNG_UST_STAP_PROBEV
#define LTTNG_UST_STAP_PROBEV(...)

EOF

['', 'LTTNG_UST_'].each { |prefix|
  puts <<EOF
/*
 * TP_ARGS takes tuples of type, argument separated by a comma.
 * It can take up to #{arg_count} tuples (which means that less than #{arg_count} tuples is
 * fine too).
 * Each tuple is also separated by a comma.
 */
#undef #{prefix}__TP_COMBINE_TOKENS
#define #{prefix}__TP_COMBINE_TOKENS(_tokena, _tokenb)				\
		_tokena##_tokenb
#undef #{prefix}_TP_COMBINE_TOKENS
#define #{prefix}_TP_COMBINE_TOKENS(_tokena, _tokenb)				\
		__TP_COMBINE_TOKENS(_tokena, _tokenb)
#undef #{prefix}__TP_COMBINE_TOKENS3
#define #{prefix}__TP_COMBINE_TOKENS3(_tokena, _tokenb, _tokenc)			\
		_tokena##_tokenb##_tokenc
#undef #{prefix}_TP_COMBINE_TOKENS3
#define #{prefix}_TP_COMBINE_TOKENS3(_tokena, _tokenb, _tokenc)			\
		__TP_COMBINE_TOKENS3(_tokena, _tokenb, _tokenc)
#undef #{prefix}__TP_COMBINE_TOKENS4
#define #{prefix}__TP_COMBINE_TOKENS4(_tokena, _tokenb, _tokenc, _tokend)	\
		_tokena##_tokenb##_tokenc##_tokend
#undef #{prefix}_TP_COMBINE_TOKENS4
#define #{prefix}_TP_COMBINE_TOKENS4(_tokena, _tokenb, _tokenc, _tokend)		\
		__TP_COMBINE_TOKENS4(_tokena, _tokenb, _tokenc, _tokend)

/*
 * _TP_EXVAR* extract the var names.
 * _TP_EXVAR1 and _TP_EXDATA_VAR1 are needed for -std=c99.
 */
#undef #{prefix}_TP_EXVAR0
#define #{prefix}_TP_EXVAR0()
#undef #{prefix}_TP_EXVAR1
#define #{prefix}_TP_EXVAR1(p_0_0)
EOF

  arg_list = lambda { |c|
    c.times.collect { |j|
      "p_#{j}_0,p_#{j}_1"
    }.join(",")
  }

  second_arg_list = lambda { |c|
    c.times.collect { |j|
      "p_#{j}_1"
    }.join(",")
  }

  tuple_arg_list = lambda { |c|
    c.times.collect { |j|
      "p_#{j}_0 p_#{j}_1"
    }.join(",")
  }

  arg_count.times { |i|
    c = (i+1)
    puts "#undef #{prefix}_TP_EXVAR#{c*2}"
    puts "#define #{prefix}_TP_EXVAR#{c*2}(#{arg_list.call(c)}) #{second_arg_list.call(c)}"
  }

  puts <<EOF
#undef #{prefix}_TP_EXDATA_VAR0
#define #{prefix}_TP_EXDATA_VAR0() __tp_data
#undef #{prefix}_TP_EXDATA_VAR1
#define #{prefix}_TP_EXDATA_VAR1(p_0_0) __tp_data
EOF

  arg_count.times { |i|
    c = (i+1)
    puts "#undef #{prefix}_TP_EXDATA_VAR#{c*2}"
    puts "#define #{prefix}_TP_EXDATA_VAR#{c*2}(#{arg_list.call(c)}) __tp_data,#{second_arg_list.call(c)}"
  }

  puts <<EOF

/*
 * _TP_EXPROTO* extract tuples of type, var.
 * _TP_EXPROTO1 and _TP_EXDATA_PROTO1 are needed for -std=c99.
 */
#undef #{prefix}_TP_EXPROTO0
#define #{prefix}_TP_EXPROTO0() void
#undef #{prefix}_TP_EXPROTO1
#define #{prefix}_TP_EXPROTO1(p_0_0) void
EOF

  arg_count.times { |i|
    c = (i+1)
    puts "#undef #{prefix}_TP_EXPROTO#{c*2}"
    puts "#define #{prefix}_TP_EXPROTO#{c*2}(#{arg_list.call(c)}) #{tuple_arg_list.call(c)}"
  }

  puts <<EOF

#undef #{prefix}_TP_EXDATA_PROTO0
#define #{prefix}_TP_EXDATA_PROTO0() void *__tp_data
#undef #{prefix}_TP_EXDATA_PROTO1
#define #{prefix}_TP_EXDATA_PROTO1(p_0_0) void *__tp_data
EOF

  arg_count.times { |i|
    c = (i+1)
    puts "#undef #{prefix}_TP_EXDATA_PROTO#{c*2}"
    puts "#define #{prefix}_TP_EXDATA_PROTO#{c*2}(#{arg_list.call(c)}) void *__tp_data,#{tuple_arg_list.call(c)}"
  }

  puts <<EOF
/* Preprocessor trick to count arguments. Inspired from sdt.h. */
#undef #{prefix}_TP_NARGS
#define #{prefix}_TP_NARGS(...)                  __TP_NARGS(__VA_ARGS__, #{(arg_count*2+1).times.collect(&:to_s).reverse.join(",")})
#undef #{prefix}__TP_NARGS
#define #{prefix}__TP_NARGS(#{(arg_count*2+1).times.collect{|i| "_"+i.to_s}.join(",")}, N, ...)	N
#undef #{prefix}_TP_PROTO_N
#define #{prefix}_TP_PROTO_N(N, ...)		_TP_PARAMS(_TP_COMBINE_TOKENS(_TP_EXPROTO, N)(__VA_ARGS__))
#undef #{prefix}_TP_VAR_N
#define #{prefix}_TP_VAR_N(N, ...)		_TP_PARAMS(_TP_COMBINE_TOKENS(_TP_EXVAR, N)(__VA_ARGS__))
#undef #{prefix}_TP_DATA_PROTO_N
#define #{prefix}_TP_DATA_PROTO_N(N, ...)	_TP_PARAMS(_TP_COMBINE_TOKENS(_TP_EXDATA_PROTO, N)(__VA_ARGS__))
#undef #{prefix}_TP_DATA_VAR_N
#define #{prefix}_TP_DATA_VAR_N(N, ...)		_TP_PARAMS(_TP_COMBINE_TOKENS(_TP_EXDATA_VAR, N)(__VA_ARGS__))
#undef #{prefix}_TP_ARGS_PROTO
#define #{prefix}_TP_ARGS_PROTO(...)		_TP_PROTO_N(_TP_NARGS(0, ##__VA_ARGS__), ##__VA_ARGS__)
#undef #{prefix}_TP_ARGS_VAR
#define #{prefix}_TP_ARGS_VAR(...)		_TP_VAR_N(_TP_NARGS(0, ##__VA_ARGS__), ##__VA_ARGS__)
#undef #{prefix}_TP_ARGS_DATA_PROTO
#define #{prefix}_TP_ARGS_DATA_PROTO(...)	_TP_DATA_PROTO_N(_TP_NARGS(0, ##__VA_ARGS__), ##__VA_ARGS__)
#undef #{prefix}_TP_ARGS_DATA_VAR
#define #{prefix}_TP_ARGS_DATA_VAR(...)		_TP_DATA_VAR_N(_TP_NARGS(0, ##__VA_ARGS__), ##__VA_ARGS__)
#undef #{prefix}_TP_PARAMS
#define #{prefix}_TP_PARAMS(...)			__VA_ARGS__

EOF
}
