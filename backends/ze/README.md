#  Header Location

- Standard: `https://github.com/argonne-lcf/level-zero-spec/tree/ddi_ver`
- Loader: `https://github.com/oneapi-src/level-zero/tags`
- Extension: `https://github.com/intel/compute-runtime/blob/master/level_zero/include/level_zero/driver_experimental/zex_api.h`

# Steps:

## 1/ Loader Repo

- We will use the loader repo to get most of the headers. The loader contains the spec header (`ze_api.h`) and the loader-specific API (`loader/ze_loader.h`).

  - Note: It is preferable to have a loader already installed on your system:
     - We may need it to check for symbols that are "defined in the header but not exported by the lib"
     - We are wary of exposing a newer loader than the system one, as users may request symbols that we cannot forward

  - If you have access to a Level Zero lib, compile and run:
```bash
$ icpx -lze_loader utils_spec_update/query_ze_version.cpp  && ./a.out
Driver version: 259.33578
API version: 1.13
Loader component versions:
  [0] Name:        loader
      Spec:        1.13
      Lib version: 1.24.0
  [1] Name:        tracing layer
      Spec:        1.13
      Lib version: 1.24.0
```
   - This will give you the loader version.

- If you have access to a loader, don't copy/paste the `/usr/include/level_zero/` folder into `$THAPI_ROOT/backend/ze/include/`.
  If not, use `git clone --depth 1 --branch $(lib_version) https://github.com/oneapi-src/level-zero.git`, where `lib_version` is the version you want.
  To find the latest version, run:
```bash
$ git ls-remote --sort="v:refname"  --tags  https://github.com/oneapi-src/level-zero.git | tail  -1
6369d8d642e9c7625e67f38664267f171b8e42dc        refs/tags/v1.28.2
```

## 2/ DDI Ver

### Sync
- Sync the fork (` https://github.com/argonne-lcf/level-zero-spec.git`) with the original remote
- Sync the `ddi_ver` branch

### Building `ddi_table`

Then we will build the `ddi_table` corresponding to the Level Zero API version.

If you don't know the Header/API version associated with the driver previously used, you can grep for `ZE_API_VERSION_CURRENT`:
```bash
$ grep define ZE_API_VERSION_CURRENT ./level_zero/ze_api.h | head -1
ZE_API_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 13 ),                      ///< latest known version
```

Now it's time to generate `ddi_ver.h`:
```bash
git clone -b ddi_ver https://github.com/argonne-lcf/level-zero-spec.git
cd level-zero-spec/scripts/
uv pip install -r third_party/requirements.txt
python run.py  --ver $version --\!debug --\!html
```

- The headers will be generated in `level-zero-spec/include/`. Copy `../include/*_ddi_ver.h` into `$THAPI_ROOT/backend/ze/include/`.
- You can sanity-check that the headers are the same And that `ZE_API_VERSION_CURRENT_M` define the same version
```
grep "ZE_API_VERSION_CURRENT" ../include/ze_api.h # Sanity check
    ZE_API_VERSION_CURRENT = ZE_MAKE_VERSION( 1, 13 ),                      ///< latest known version
#ifndef ZE_API_VERSION_CURRENT_M
#define ZE_API_VERSION_CURRENT_M  ZE_MAKE_VERSION( 1, 15 )
#endif // ZE_API_VERSION_CURRENT_M
```
(We don't talk about `ZE_API_VERSION_CURRENT_M`...)

- Note that `layers/zel_tracing_ddi_ver.h` is not generated manually, and need manual update.

## 3/ Optional: ZEX
- We are missing the `zex` header:
  - Found at `https://github.com/intel/compute-runtime/blob/master/level_zero/include/level_zero/driver_experimental/zex_api.h`

## Now Try to Compile:

- Try to compile

# Potential Problems

## 1

```
tracer_ze.c:197:8: error: use of undeclared identifier 'ZE_STRUCTURE_TYPE_DEVICE_CACHE_LINE_SIZE_EXT'; did you mean 'ZE_STRUCTURE_TYPE_DEVICE_CACHELINE_SIZE_EXT'?
  197 |   case ZE_STRUCTURE_TYPE_DEVICE_CACHE_LINE_SIZE_EXT:
      |        ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      |        ZE_STRUCTURE_TYPE_DEVICE_CACHELINE_SIZE_EXT
```

Due to Intel's lack of naming consistency, you may need to update the `struct_type_conversion_table` in `ze_model.rb`.

## 2

```
tracer_ze.c:8456:20: error: unused function 'zelTracerResetAllCallbacks_hid' [-Werror,-Wunused-function]
 8456 | static ze_result_t zelTracerResetAllCallbacks_hid(zel_tracer_handle_t hTracer)__attribute__ ((alias ("zelTracerResetAllCallbacks")));
      |                    ^~~~~~~~~~~~~~~~~
```
Need to modify the `$zel_commands.each` block that generates `#{c.decl_hidden_alias};` in `gen_ze.rb`.

## 3

Runtime error: When running `babeltrace_thapi` (`iprof -t`), constant error:
```
PogrammableParamValueInfoExp>': uninitialized constant ZE::ZETMetricProgrammableParamValueInfoExp::ZET_MAX_METRIC_PROGRAMMABLE_VALUE_DESCRIPTION_EXP (NameError)

           :description, [ :char, ZET_MAX_METRIC_PROGRAMMABLE_VALUE_DESCRIPTION_EXP ]
```
Update in `backends/ze/gen_ze_library.rb`, Module ZE.
TODO: `h2yaml` should be able to find all the constants defined.
