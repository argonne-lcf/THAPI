# Backends

A backend is one traced API: `cuda`, `hip`, `itt`, `mpi`, `omp`, `opencl`, `ze`.
Each has its own directory, but they share the generators in `utils/`. This file
holds what is true for all of them. What is true for one lives in that backend's
own README.

## `meta_parameters_struct`: how to read a struct's bytes

`<backend>_meta_parameters.yaml` can carry a `meta_parameters_struct` section.
It says how to print the byte-array members of a struct:

```yaml
meta_parameters_struct:
  ze_uuid_t:
    - [ uuid_reversed, id ]
  ze_kernel_uuid_t:
    - [ uuid_reversed, kid ]
    - [ uuid_reversed, mid ]
  ze_ipc_mem_handle_t:
    - [ blob, data ]
```

Each row is `[ renderer, member ]`. A member with no row prints the way the FFI
base class prints it, which is what the fixed-width strings want.

### Why this must be declared

A byte array cannot say from its shape what it holds. `uint8_t x[16]` is a UUID
in `ze_uuid_t` and opaque bytes in `ze_device_luid_ext_t`. `char x[16]` is a
UUID in `CUuuid` and opaque bytes in `ze_ipc_mem_handle_t`. Headers also spell
the same array three ways -- `char`, `unsigned char` and `uint8_t` -- and cuda
uses all three. So neither the C type nor the class name can decide, and the
header's answer is written down instead.

### The renderers

| Renderer | Prints | Used by |
| --- | --- | --- |
| `blob` | every byte escaped, stopping at none | opaque handles |
| `uuid` | dashed hex, first byte first | cuda, hip |
| `uuid_reversed` | dashed hex, last byte first | ze, zes |

`blob` exists because a blob is not a C string: read as text it would stop at
the first NUL and lose the rest. The two UUID renderers differ in byte order
alone, because cuda and hip print a UUID first byte first and ze prints it last
byte first. Both keep the canonical dashes (after bytes 4, 6, 8 and 10) that the
array is long enough to reach, so 16 bytes read 8-4-4-4-12 and an 8-byte LUID
degrades to `17161514-1312-1110` rather than running off the end.

The renderers live in `RENDERERS` in `utils/gen_library_base.rb`. Only the ones
a backend's rows ask for are emitted, into one `Rendering` module per backend.

### Mistakes the build refuses

A row is written by hand against a header that keeps moving, so the generator
raises rather than let a wrong row do nothing:

- a struct name that matches no struct;
- a member name that matches no member of that struct;
- a member that is not a byte array, which every renderer needs;
- a renderer name that is not in `RENDERERS`;
- the same member rendered twice, or the same struct declared twice.
