# HIP Backend

## `hip_missing_apis.h`

`hip_missing_apis.h` is not part of the official header.

Declarations for functions and types exported by `libamdhip64.so` but not declared in any public HIP header:

- `hipGraphDebugDotPrint`: Exports a `hipGraph_t` as a DOT file. The `hipGraphDebugDotFlags` enum (used by the `flag` parameter) is declared in `hip_runtime_api.h`, but the function itself is not.
- Runtime registration functions: `__hipRegisterFatBinary`, `__hipRegisterFunction`, `__hipRegisterVar`, etc.
- `hipGetCmdName`, `activity_domain_t`, `hipRegisterTracerCallback`
