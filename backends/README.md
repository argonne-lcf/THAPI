# Backends

## `meta_parameters_struct` and `meta_parameters_function`

`<backend>_meta_parameters.yaml` can carry two rendering sections. They say how
to print bytes whose meaning no C type states: `meta_parameters_struct` for a
struct's byte-array members, `meta_parameters_function` for a function's
byte-array parameters.

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

If the menbers is not specified here, it will fall back to the default print.
For example, `char[N]` will be printed as a C string, stoping at the first null char.

We support `uuid`, `uuid_reversed`, and `blob`.
