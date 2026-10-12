# PyTorch Backend Tests

| Test | Feature | Description |
|------|---------|-------------|
| **basic_op_name-Aa** | Basic tracing | Validates tracing of a simple PyTorch operation with empty overload name. |
| **basic_op_overload_name-Aa** | Overload name handling | Validates that operations with overload names create qualified names (e.g., `aten::add.Tensor`). |
| **nested_ops_simple-ABba** | LIFO stack (ABBA) | Validates LIFO stack correctly matches nested operations in ABBA order (outer→inner→inner exit→outer exit). |
| **nested_ops_interleaved-ABab** | LIFO stack error handling (ABAB) | Validates error flag is set when exit events arrive in non-LIFO order (ABAB pattern), testing stack mismatch detection. |
| **deeply_nested_ops-ABCcba** | Stack depth handling | Validates LIFO stack correctly handles three levels of nesting (forward→linear→matmul). |
| **exit_without_entry-a** | Stack underflow | Validates error flag is set when an exit event arrives with no matching entry on the stack. |
| **multithreaded_ops-AaBb** | Per-thread isolation | Validates each thread maintains its own independent LIFO stack without cross-thread interference. |
