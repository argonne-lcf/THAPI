bats_require_minimum_version 1.5.0

@test "PyTorch: trace contains aten::empty.memory_format" {
  iprof --backends pytorch --analysis-output ./pytorch_out.txt -- \
    python3 ./integration_tests/pytorch_example.py
  grep "aten::empty.memory_format" ./pytorch_out.txt
}
