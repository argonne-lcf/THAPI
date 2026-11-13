#!/usr/bin/env bash
# Usage: utils_spec_update/extension_versions.sh [<include/CL dir>]
#
# For each extension that adds host functions (per cl.xml) and that an OpenCL
# platform or device on this node reports, prints the revision the runtime
# reports (clinfo) next to the one the headers declare
# (CL_<EXT>_EXTENSION_VERSION). Default headers: the vendored include/CL.
# Run it on every system you care about (a GPU compute node, ...): each driver
# reports its own extensions, per device; the loader does not know them.
set -euo pipefail
srcdir=$(dirname "$0")/..
inc=${1:-$srcdir/include/CL}

with_fns=$(perl -0ne 'print "$1\n" while /<extension name="([^"]+)"[^>]*(?<!\/)>((?:(?!<\/?extension[ >]).)*<command name=)/gs' "$srcdir"/cl.xml)

clinfo | grep -oE 'cl_[a-z]+_[A-Za-z0-9_]+ +0x[0-9a-f]+ \([0-9.]+\)' | sort -u | grep -wFf <(echo "$with_fns") |
  while read -r ext _ ver; do
    h=$(grep -h -A1 "define ${ext^^}_EXTENSION_VERSION" "$inc"/*.h | grep -oE '[0-9]+, *[0-9]+, *[0-9]+' | tr -d ' ' | tr , . || true)
    echo "$ext runtime ${ver//[()]/} headers ${h:-none}"
  done | column -t
