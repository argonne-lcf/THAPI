#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

command -v metababel >/dev/null
command -v protoc >/dev/null || {
    echo "Missing protoc: load your protobuf development environment."
    exit 1
}
pkg-config --exists babeltrace2 protobuf || {
    echo "Missing babeltrace2 or protobuf development package."
    exit 1
}

generated=build/btx_sink_timeline
objects=build/timeline-objects
mkdir -p "$objects" build/timeline-proto build/timeline-plugins

# Generate the sink using the SAME MetaBabel as the libfabric filter.
metababel \
    -u ../../xprof/btx_interval_model.yaml \
    -t SINK \
    -o "$generated" \
    -p btx_timeline \
    -c timeline \
    --params ../../xprof/btx_timeline_params.yaml

# Generate Perfetto protobuf serialization code.
protoc \
    -I../../xprof \
    --cpp_out=build/timeline-proto \
    ../../xprof/perfetto_pruned.proto

includes=(
    -I../../utils
    -I../../utils/include
    "-I$generated"
    -Ibuild/timeline-proto
)

objects_to_link=()

for source in "$generated"/metababel/*.c "$generated"/btx_main.c; do
    object="$objects/$(basename "${source%.c}").o"
    cc -fPIC "${includes[@]}" \
        $(pkg-config --cflags babeltrace2) \
        -c "$source" -o "$object"
    objects_to_link+=("$object")
done

c++ -std=c++17 -fPIC "${includes[@]}" \
    $(pkg-config --cflags babeltrace2 protobuf) \
    -c ../../xprof/btx_timeline.cpp \
    -o "$objects/timeline.o"

c++ -std=c++17 -fPIC "${includes[@]}" \
    $(pkg-config --cflags protobuf) \
    -c build/timeline-proto/perfetto_pruned.pb.cc \
    -o "$objects/protobuf.o"

c++ -shared \
    "${objects_to_link[@]}" \
    "$objects/timeline.o" "$objects/protobuf.o" \
    -Wl,-z,defs \
    -o build/timeline-plugins/libXTimeline.so \
    $(pkg-config --libs babeltrace2 protobuf)

echo "Built local THAPI timeline plugin."
