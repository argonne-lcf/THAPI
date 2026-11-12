#!/usr/bin/env bash
# Usage: utils_spec_update/check_missing_apis.sh <builddir>/backends/opencl/tracer_opencl.c
#
# Lists the functions THAPI traces (one `<fn>_ptr_t` typedef each in the
# generated tracer_opencl.c) that no header of include/CL/ declares, and
# diffs them against the ones opencl_missing_apis.h declares. No output and
# exit 0 means opencl_missing_apis.h is exactly what is needed.
set -euo pipefail
srcdir=$(dirname "$0")/..
protos() { grep -ho '\<cl[A-Za-z0-9]*[[:space:]]*(' "$@" | tr -d ' (' | sort -u; }

traced=$(sed -n 's/.*(CL_API_CALL \*\(cl[A-Za-z0-9]*\)_ptr_t).*/\1/p' "$1" | sort -u)
diff -u --label opencl_missing_apis.h --label 'traced, not in include/CL' \
  <(protos "$srcdir"/opencl_missing_apis.h) \
  <(comm -23 <(echo "$traced") <(protos "$srcdir"/include/CL/*.h))
