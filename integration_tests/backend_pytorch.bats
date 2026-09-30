bats_require_minimum_version 1.5.0

@test "PyTorch: tally contains aten::empty.memory_format" {
  iprof --backends pytorch --analysis-output ./pytorch_out.txt -- \
    python3 ./integration_tests/pytorch_example.py
  grep "aten::empty.memory_format" ./pytorch_out.txt
}

@test "PyTorch: trace (-t) contains pretty-printed op_entry/op_exit events" {
  iprof --backends pytorch -t --analysis-output ./pytorch_trace_out.txt -- \
    python3 ./integration_tests/pytorch_example.py
  grep "lttng_ust_pytorch:op_entry" ./pytorch_trace_out.txt
  grep "lttng_ust_pytorch:op_exit" ./pytorch_trace_out.txt
  grep 'name: "aten::empty' ./pytorch_trace_out.txt
  grep 'overload_name: "memory_format' ./pytorch_trace_out.txt
}
