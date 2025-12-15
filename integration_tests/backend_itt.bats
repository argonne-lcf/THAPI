bats_require_minimum_version 1.5.0

@test "ITT (C): trace contains __itt_task_begin events" {
  gcc ./integration_tests/itt_example.c -I${ITTAPI_ROOT}/include -L${ITTAPI_ROOT}/lib64 -L${ITTAPI_ROOT}/lib -littnotify -ldl -o itt_example
  iprof --backends itt --analysis-output ./itt_out_c.txt -- ./itt_example
  grep "Example.Domain:Task 2" ./itt_out_c.txt
  grep "Example.Domain:Task 1" ./itt_out_c.txt
}

@test "ITT (Python, context manager): trace contains ITT task events" {
  iprof --backends itt --analysis-output ./itt_out_py_context-manager.txt -- python3 ./integration_tests/itt_example_context-manager.py
  grep "Example.Domain:Task 2" ./itt_out_py_context-manager.txt
  grep "Example.Domain:Task 1" ./itt_out_py_context-manager.txt
}

@test "ITT (Python, C-style): trace contains ITT task events" {
  iprof --backends itt --analysis-output ./itt_out_py_c-style.txt -- python3 ./integration_tests/itt_example_c-style.py
  grep "Example.Domain:Task 2" ./itt_out_py_c-style.txt
  grep "Example.Domain:Task 1" ./itt_out_py_c-style.txt
}

