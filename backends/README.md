# Backends

## `meta_parameters_struct`

`<backend>_meta_parameters.yaml` can carry a `meta_parameters_struct` section.
Right now, it used to specify how to print a byte-array members:

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

If the menbers is not specified here, it will fall back to the default print.
For example, `char[N]` will be printed as a C string, stoping at the first null char.

We support `uuid`, `uuid_reversed`, and `blob`.
