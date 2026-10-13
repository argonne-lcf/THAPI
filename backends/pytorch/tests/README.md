# PyTorch Backend Tests

| Test | Feature | Description |
|------|---------|-------------|
| **basic_op_name-AA** | Basic tracing | Validates tracing of a simple PyTorch operation with empty overload name. |
| **basic_op_overload_name-AA** | Overload name handling | Validates that operations with overload names create qualified names (e.g., `aten::add.Tensor`). |
| **nested_ops_simple-ABBA** | LIFO stack (ABBA) | Validates LIFO stack correctly matches nested operations in ABBA order (outer→inner→inner exit→outer exit). |
| **nested_ops_interleaved-ABAB** | Non LIFO stack error handling (ABAB) | Validates error flag is set when an exit name does not match the top of the stack. |
| **deeply_nested_ops-ABCCBA** | Stack depth handling | Validates LIFO stack correctly handles three levels of nesting (forward→linear→matmul). |
| **exit_without_entry-A** | Stack underflow | Validates error flag is set when an exit event arrives with no matching entry on the stack. |
| **multithreaded_ops-AABB** | Per-thread isolation | Validates each thread maintains its own independent LIFO stack without cross-thread interference. |
| **reentrant_op-AAAA** | Reentrancy | Validates the LIFO stack correctly handles the same op re-entering itself (aten::norm.Scalar calling aten::norm.Scalar internally). |
