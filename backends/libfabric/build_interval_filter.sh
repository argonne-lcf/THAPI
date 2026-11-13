#!/usr/bin/env bash
set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

generated=build/btx_filter_libfabric
objects=build/interval-objects
mkdir -p "$objects"

includes=(
    -I../../utils
    -I../../utils/include
    -Iupstream/libfabric/include
    "-I$generated"
)

# Compile MetaBabel's generated C implementation.
for source in "$generated"/metababel/*.c "$generated"/btx_main.c; do
    cc -fPIC \
        "${includes[@]}" \
        $(pkg-config --cflags babeltrace2) \
        -c "$source" \
        -o "$objects/$(basename "${source%.c}").o"
done

# Compile our entry/exit pairing callbacks.
c++ -std=c++17 -fPIC \
    "${includes[@]}" \
    $(pkg-config --cflags babeltrace2) \
    -c btx_libfabricinterval_callbacks.cpp \
    -o "$objects/callbacks.o"

# Link the Babeltrace plugin.
c++ -shared \
    "$objects"/*.o \
    -Wl,-z,defs \
    -o build/libLibfabricInterval.so \
    $(pkg-config --libs babeltrace2)

echo "Built build/libLibfabricInterval.so"