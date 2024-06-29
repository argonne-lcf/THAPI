# gtensor integration tests

Allows testing cuda, hip, and ze backends with single source file.

# Usage

```
GTENSOR_DEVICE=cuda bats ./integration_tests/gtensor.bats
```
Or "hip" or "sycl"

Note that THAPI hip backend does not yet do device profiling, modifications will need to be
made for tests to pass on hip.
