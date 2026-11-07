# Libfabric tracing backend

This experimental backend uses the upstream libfabric hook framework to
record LTTng events for:

- `fi_send`
- `fi_recv`
- `fi_cq_read`

A Babeltrace filter pairs entry and exit events into THAPI host intervals.
THAPI's timeline exporter converts these intervals into a Perfetto trace.

The backend currently uses standalone build scripts. Integration into the
normal THAPI build and `iprof` launcher is not complete.

The bundled upstream sources are under `upstream/libfabric`.
The tested source version is libfabric 2.3.1.

The build embeds the upstream hook framework into the tracing plugin and
links against system libfabric. It does not replace system libfabric.

The commands below were tested with the CXI provider.
Other providers and libfabric versions require validation.


## Environment setup

The repository provides `backends/libfabric/tools/build_efficios_deps.sh`
to install the tracing dependencies needed by this workflow. 

### One-time dependency installation
If `~/efficios_dep/env.sh` does not exist, install the dependencies:

```bash
bash backends/libfabric/tools/build_efficios_deps.sh
```

If you already have compatible dependencies, you can use that environment
instead. 

### Activate the environment in every terminal

On Aurora, run these commands in this order in each new terminal, including
both terminals used for the ping-pong test:

```bash
module load thapi
source ~/efficios_dep/env.sh
```

Then, from the THAPI repository root:

```bash
cd backends/libfabric
```


Run the commands below from `backends/libfabric`.
## 1. Generate the tracepoints and build the hook

Run this block in Bash. It stops if any step fails.

```bash
(
    set -euo pipefail

    mkdir -p build/lttng

    h2yaml \
        --compat-cast-to-yaml \
        --include-builtin-headers \
        "-Wc,-I$PWD/upstream/libfabric/include" \
        extract/libfabric_api.h \
        > libfabric_api.yaml

    ruby gen_libfabric_tracepoints.rb \
        > build/libfabric_tracepoints.tp

    lttng-gen-tp build/libfabric_tracepoints.tp \
        -o build/libfabric_tracepoints.h \
        -o build/libfabric_tracepoints.c

    ruby ../../utils/tracepoint_gen.rb 25 \
        > build/lttng/tracepoint_gen.h

    bash build_hook_framework.sh
)
```

Output:

```text
build/libthapi-fi.so
```

The `25`: it generates macros
supporting up to 25 typed arguments per event.

## 2. Generate and build the interval filter

```bash
(
    set -euo pipefail

    ruby gen_babeltrace_libfabric_model.rb \
        > build/btx_libfabric_model.yaml

    metababel \
        -u build/btx_libfabric_model.yaml \
        -d ../../xprof/btx_interval_model.yaml \
        --matching btx_libfabric_matching.yaml \
        -t FILTER \
        -o build/btx_filter_libfabric \
        -p libfabricinterval \
        -c interval \
        -i rdma/fi_endpoint.h

    bash build_interval_filter.sh
)
```

Output:

```text
build/libLibfabricInterval.so
```

Verify plugin discovery:

```bash
babeltrace2 --plugin-path="$PWD/build" list-plugins
```

The output should include:

```text
filter.libfabricinterval.interval
```

## 3. Build a compatible local timeline exporter

```bash
bash build_local_timeline.sh
```

Output:

```text
build/timeline-plugins/libXTimeline.so
```

This helper rebuilds THAPI's existing timeline exporter using the same
MetaBabel/Babeltrace toolchain as the interval filter.



## 4. Record a CXI ping-pong test

Use two terminals on the same node, under the same user account.
In both terminals, change to `backends/libfabric`.

Check that the selected domain is available:

```bash
fi_info -p cxi -d cxi0 -t FI_EP_RDM
```

### Terminal 1: start recording and launch the server

```bash
session_name="thapi-libfabric-$(date +%Y%m%d-%H%M%S)"
trace_dir="$PWD/build/$session_name"

lttng create "$session_name" --output="$trace_dir"

lttng enable-event --userspace --session="$session_name" \
    'lttng_ust_libfabric:*'

lttng add-context --userspace --session="$session_name" \
    -t vpid -t vtid

lttng start "$session_name"

FI_PROVIDER_PATH="+$PWD/build/libthapi-fi.so" \
FI_HOOK=thapi \
fi_pingpong -p cxi -d cxi0 -e rdm -m msg \
    -I 3 -S 256 -c
```

The server waits for the client.

### Terminal 2: launch the client

```bash
FI_PROVIDER_PATH="+$PWD/build/libthapi-fi.so" \
FI_HOOK=thapi \
fi_pingpong -p cxi -d cxi0 -e rdm -m msg \
    -I 3 -S 256 -c 127.0.0.1
```

`127.0.0.1` is the same-node control connection address. The selected
libfabric provider is still CXI.

The 256-byte size exceeded the tested CXI inject threshold of 192 bytes.
Smaller messages can use `fi_inject`, which this backend does not yet trace.

### Terminal 1: stop recording

After both applications finish:

```bash
lttng stop "$session_name" 
lttng destroy "$session_name"
```
Keep using this terminal for the following commands so that `$trace_dir`
retains its value.

## 5. Inspect the recording

Raw events:

```bash
babeltrace2 "$trace_dir" > "${trace_dir}.txt"
head -n 20 "${trace_dir}.txt"
```

Expected event names:

```text
lttng_ust_libfabric:fi_send_entry
lttng_ust_libfabric:fi_send_exit
lttng_ust_libfabric:fi_recv_entry
lttng_ust_libfabric:fi_recv_exit
lttng_ust_libfabric:fi_cq_read_entry
lttng_ust_libfabric:fi_cq_read_exit
```

Converted THAPI intervals:

```bash
babeltrace2 --plugin-path="$PWD/build" \
    "$trace_dir" \
    --component=filter.libfabricinterval.interval \
    > "${trace_dir}.intervals.txt"

head -n 20 "${trace_dir}.intervals.txt"
```

Expected output contains `lttng:host` events with function names, start
timestamps, durations in nanoseconds, and an error flag.

## 6. Generate the Perfetto timeline

```bash
BABELTRACE_PLUGIN_PATH= babeltrace2 \
    --plugin-path="$PWD/build/timeline-plugins:$PWD/build" \
    "$trace_dir" \
    --component=filter.libfabricinterval.interval \
    --component=sink.btx_timeline.timeline \
    --params="output_path=\"${trace_dir}.pftrace\",offset=+0"

ls -lh "${trace_dir}.pftrace"
```

The `+0` is intentional: Babeltrace's parameter syntax uses a `+` prefix
for unsigned integers, and the timeline sink requires an unsigned offset.

Download the `.pftrace` file and open it using **Open trace file** at:

https://ui.perfetto.dev
