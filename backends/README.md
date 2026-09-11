# Backends

## `meta_parameters_struct` and `meta_parameters_function`

`<backend>_meta_parameters.yaml` can carry two more sections of the same kind
as `meta_parameters` itself: facts the header knows that the C declaration does
not carry. These two say how to print bytes whose meaning no C type states --
`meta_parameters_struct` for a struct's byte-array members,
`meta_parameters_function` for a function's byte-array parameters.

```yaml
meta_parameters_struct:
  ze_uuid_t:
    - [ uuid_reversed, id ]
  ze_kernel_uuid_t:
    - [ uuid_reversed, kid ]
    - [ uuid_reversed, mid ]
  ze_ipc_mem_handle_t:
    - [ blob, data ]

meta_parameters_function:
  cuDeviceGetLuid:
    - [ uuid, luid ]
```

A member with no row falls back to the default print: `char[N]` prints as a C
string, stopping at the first null char.

| Renderer | Prints |
| --- | --- |
| `uuid` | dashed hex, first byte first (cuda, hip) |
| `uuid_reversed` | dashed hex, last byte first (ze, zes) |
| `blob` | every byte escaped, stopping at none |
