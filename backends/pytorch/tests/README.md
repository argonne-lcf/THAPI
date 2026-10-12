# PyTorch Backend Tests

| Test | Feature | Description |
|------|---------|-------------|
| **basic_op_name** | Basic tracing | Validates tracing of a simple PyTorch operation with empty overload name. |
| **basic_op_overload_name** | Overload name handling | Validates that operations with overload names create qualified names (e.g., `aten::add.Tensor`). |
| **nested_ops_simple** | LIFO stack (AABB) | Validates LIFO stack correctly matches nested operations in AABB order (outer→inner→inner exit→outer exit). |
| **nested_ops_interleaved** | LIFO stack error handling (ABAB) | Validates error flag is set when exit events arrive in non-LIFO order (ABAB pattern), testing stack mismatch detection. |
| **deeply_nested_ops** | Stack depth handling | Validates LIFO stack correctly handles three levels of nesting (forward→linear→matmul). |
| **exit_without_entry** | Stack underflow | Validates error flag is set when an exit event arrives with no matching entry on the stack. |
| **multithreaded_ops** | Per-thread isolation | Validates each thread maintains its own independent LIFO stack without cross-thread interference. |
