# pretty_print

C++ pretty printers for THAPI traces (`babeltrace_thapi trace`, `iprof -t`).
They print the same text as the Ruby printer (`sink.text.rubypretty`).

## Pieces

- `gen_pretty.rb` generates the printers of one backend from YAML only:
  - `--model`: the babeltrace (metababel) event model, `btx_<backend>_model.yaml`.
    It gives the events, their fields, and the C type of each field (`cast_type`).
  - the API models, `<backend>_api.yaml` (positional arguments). h2yaml writes them from
    the C headers. They give the members of each struct, the enumerators of each enum,
    and the typedefs.
  - `--rules`: `backends/<backend>/pretty_rules.yaml`, what neither file says (below).

  It writes:
  - `--printers` `btx_<backend>pretty.hpp`: a printer `pv(o, value)` for each C struct and
    enum the events carry;
  - `--formatters` `btx_<backend>pretty_fmt.hpp`: an `fmt::formatter` for each of them, for
    code that wants `fmt::format("{}", value)` (the plugin does not use it);
  - `--callbacks` `btx_<backend>pretty_callbacks.cpp`: the callbacks of a metababel FILTER,
    `filter.<backend>pretty.pretty`, which turns each event into one `thapi_pretty:line`
    event with the same timestamp and forwards every other event.
  - `--all-types` also writes a printer for every struct and union of the API.

  The generated C++ prints most values by their C type; the compiler gives the sizes, the
  member offsets and the enumerator values. The YAML gives only what the C type does not:
  the order of the members, the enumerator names, and which integer types hold flags.
- `thapi_pretty.hpp`: the primitives that reproduce the Ruby output byte for byte
  (`Float#to_s`, `String#inspect` of binary strings, `"0x%016x"`, `Time#strftime`).
- `pretty.am`: the make rules every backend includes (see the comment in it).
- `xprof/btx_pretty_print_callbacks.cpp`: `sink.thapipretty.text`, a metababel SINK that
  prints the lines, and every other event the way the Ruby printer prints an event it has
  no printer for (metababel `on_upstream`).

## Rules

```yaml
special:            # structs printed in a special way
  uuid_reversed: [ze_device_uuid_t, ...]   # { id: <16 bytes back to front> }
  uuid: [CUuuid]                           # { id: <16 bytes in order> }
  kernel_uuid: [ze_kernel_uuid_t]          # { kid: ..., mid: ... }
  handle: [ze_ipc_mem_handle_t, ...]       # { data: "\x00\x01..." }
  typed_value: [zet_typed_value_t]         # the member of the union `type` selects
version: [ze_api_version_t, ...]           # enums printed "major.minor"
drop_enumerators: [CU_COREDUMP_LIGHTWEIGHT_FLAGS]  # unknown to the Ruby printer
pointers: [CUdeviceptr]                    # integer typedefs printed as pointers
```

Flags follow THAPI's convention: a typedef of an enum named `*flag_t`, and the
`*_flags_t` spelling of a `*_flag_t` typedef.

## In the graph

`babeltrace_thapi trace` picks the components after the source:

- every selected backend with a Ruby printer has a pretty filter: the filters, then
  `sink.thapipretty.text`;
- some do: the filters, then the Ruby sink, which prints their lines as they are and
  the other events with its Ruby printers;
- none does: the Ruby sink.

`THAPI_PRETTY=ruby` or `THAPI_PRETTY=cxx` forces the Ruby sink or `sink.thapipretty.text`.
