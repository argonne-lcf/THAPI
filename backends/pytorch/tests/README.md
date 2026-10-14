# PyTorch Backend Tests

| Test | Feature | Description |
|------|---------|-------------|
| **basic_op_name-AA** | Basic tracing | Validates tracing of a simple PyTorch operation with empty overload name. |
| **basic_op_overload_name-AA** | Overload name handling | Validates that operations with overload names create qualified names (e.g., `aten::add.Tensor`). |
| **nested_ops_simple-ABBA** | LIFO stack (ABBA) | Validates LIFO stack correctly matches nested operations in ABBA order (outer→inner→inner exit→outer exit). |
| **deeply_nested_ops-ABCCBA** | Stack depth handling | Validates LIFO stack correctly handles three levels of nesting (forward→linear→matmul). |
| **multithreaded_ops-AABB** | Per-thread isolation | Validates each thread maintains its own independent LIFO stack without cross-thread interference. |
| **reentrant_op-AAAA** | Reentrancy | Validates the LIFO stack correctly handles the same op re-entering itself (aten::norm.Scalar calling aten::norm.Scalar internally). |
