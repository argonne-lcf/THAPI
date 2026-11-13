# How to Update the Headers

## Which Version

Never newer than what Aurora / Sunspot provides (same rule as ze), and the headers and `cl.xml` move together.

What matters is the version of the **implementation** (the Intel OpenCL runtime), not the one of the loader:
the loader only forwards calls, while the extension signatures are those the implementation implements.
Use the headers oneAPI ships, which are the ones applications compile against.

Current (Aurora stack `26.181.0`, oneAPI `2026.1`, `intel-opencl-25.18`):

- Headers: `https://github.com/KhronosGroup/OpenCL-Headers` at `6eabe90` (2025-06-10, `git describe`: `v2024.10.24-10-g6eabe90`),
  identical to `oneapi/compiler/2026.1/include/CL`. Copied **unmodified** into `include/CL/`.
- Registry: `https://github.com/KhronosGroup/OpenCL-Docs/blob/main/xml/cl.xml` at `5efafb0` (2025-06-10). Copied **unmodified** into `cl.xml`.
  Our changes to it are in `cl.xml.patch`.
- Intel entry points that neither declares: `opencl_missing_apis.h` (hand-written, like `backends/hip/hip_missing_apis.h`).

### Find the versions on Sunspot

```bash
$ module load oneapi
$ clinfo | grep -E "ICD loader Version|cl_khr_command_buffer +0x"      # loader, and an extension revision
  ICD loader Version                              3.0.6
  cl_khr_command_buffer 0x9007 (0.9.7)
$ readlink /opt/aurora/26.181.0/support/libraries/khronos/default        # Khronos SDK (loader + headers)
v2023.12.14
$ O=$(dirname $(which icx))/../include/CL                                # oneAPI headers
$ grep -A1 "define CL_KHR_COMMAND_BUFFER_EXTENSION_VERSION" $O/cl_ext.h
#define CL_KHR_COMMAND_BUFFER_EXTENSION_VERSION \
    CL_MAKE_VERSION(0, 9, 7)
```

Check a compute node too (`ssh` to one of your job's nodes): the GPU does not implement the same extensions
as the CPU device (for example, no `cl_khr_command_buffer` on the GPU).

Then find the OpenCL-Headers commit the oneAPI headers come from:

```bash
git clone https://github.com/KhronosGroup/OpenCL-Headers.git && cd OpenCL-Headers
for c in $(git tag) $(git log --format=%h --since=<a year before> main -- CL); do
  echo "$(for f in CL/*.h; do git show $c:$f | diff - $O/$(basename $f); done | grep -c '^[<>]') $c"
done | sort -n | head -3        # 0 = exact match
```

The loader (here `v2023.12.14`) can be older than the headers: check that it exports every core function
the new `cl.h` declares (`nm -D --defined-only libOpenCL.so.1`). It exports no extension function at all;
those are reached through `clGetExtensionFunctionAddress(ForPlatform)`.

# Steps

## 1/ Headers

```bash
git -C OpenCL-Headers checkout $commit
rm include/CL/*.h
cp OpenCL-Headers/CL/*.h include/CL/
diff -r include/CL $O | grep -v "^Only in"      # nothing: same as oneAPI's
git status include/CL                            # new / removed files upstream
```

- Do not edit them. Anything they lack goes into `opencl_missing_apis.h`.
- `.gitignore` ignores `*.h` but re-includes `include/CL/*.h`, so new upstream headers show up in `git status`.

Upstream deprecates a header by turning it into a stub with a `#pragma message` and no include guard.
`cl.h.include`, `gen_opencl.rb` and `gen_opencl_probes.rb` must not include those: gcc rejects the pragma
when lttng re-includes the tracepoint header. Their content is usually reachable from `opencl.h`.

```bash
$ grep -l 'pragma message' include/CL/*.h
include/CL/cl_dx9_media_sharing_intel.h
include/CL/cl_ext_intel.h
include/CL/cl_gl_ext.h
include/CL/cl_version.h
```

(`cl_version.h` is fine: its `#pragma message` only fires when `CL_TARGET_OPENCL_VERSION` is missing or
invalid, and the three files above define it.)

## 2/ `cl.xml`

The extension headers (`cl_ext.h`, `cl_gl.h`, `cl_egl.h`, `cl_d3d1*.h`, `cl_dx9_media_sharing.h`, `cl_layer.h`,
`cl_va_api_media_sharing_intel.h`, `cl_function_types.h`) are generated from `cl.xml` by OpenCL-Headers'
`scripts/gen_headers.py`. So the `cl.xml` in sync with the headers is the one that regenerates them.
Try the OpenCL-Docs commits of `xml/cl.xml` up to the date of the headers' commit:

```bash
git clone https://github.com/KhronosGroup/OpenCL-Docs.git
cd OpenCL-Headers/scripts                       # at the headers' commit: the generator changes too
for c in $(git -C ../../OpenCL-Docs log --format=%h --before=<day after the headers' commit> -- xml/cl.xml | head -15); do
  git -C ../../OpenCL-Docs show $c:xml/cl.xml > /tmp/cl.xml; rm -rf /tmp/gen; mkdir /tmp/gen
  uv run --with Mako python gen_headers.py -registry /tmp/cl.xml -o /tmp/gen > /dev/null
  echo "$c $(for f in /tmp/gen/*.h; do diff $f ../CL/$(basename $f); done | grep -c '^[<>]')"
done
```

- Take the commit with the smallest diff, and check that the remaining diff declares no function:

  ```bash
  fns() { grep -ho '^cl[A-Za-z0-9]*(' "$@" | sort -u; }
  diff <(fns /tmp/gen/*.h) <(cd ../CL && fns $(ls /tmp/gen))       # nothing
  ```

  An exact match may not exist: OpenCL-Headers is sometimes generated from a `cl.xml` that never was on
  OpenCL-Docs `main`. For `6eabe90`, `5efafb0` leaves 25 lines (two extensions without functions:
  `cl_khr_external_semaphore_dx_fence` and the `cl_khr_kernel_clock` version).
- Copy it unmodified into `cl.xml`, then rebase `cl.xml.patch`:

  ```bash
  cp cl.xml /tmp/cl.xml.patched && patch /tmp/cl.xml.patched cl.xml.patch    # fix the rejects by hand
  { head -2 cl.xml.patch; diff -u cl.xml /tmp/cl.xml.patched | tail -n +3; } > /tmp/new.patch && mv /tmp/new.patch cl.xml.patch
  ```

  Drop what upstream now has (for `5efafb0`: `cl_icdl_info` and `clGetICDLoaderInfoOCLICD`), and check nothing
  is defined twice. The `<require comment=...>` rewordings matter: the generator reads them (` - bitfield`,
  the type name first).

## 3/ Build and fix

```bash
make -C $BUILD/backends/opencl -j WERROR=-Werror                 # also with clang: CFLAGS=-Wno-gnu-folding-constant
utils_spec_update/check_missing_apis.sh $BUILD/backends/opencl/tracer_opencl.c
```

`check_missing_apis.sh` checks `opencl_missing_apis.h`: a traced function that no header declares still
compiles, even with `-Werror`. No output: nothing to do. `+clFoo`: add it, with its signature from
`cl.xml.patch`. `-clFoo`: upstream now declares it, remove it.

Then see "Potential problems" below.

## 4/ Compare the generated files, and test on a GPU node

The generated code **does** change: new functions are traced, and fixed `cl.xml` entries change some
signatures. Review the diff, and explain each change in the PR. The CI job "Assert generated files are
equivalent" shows it; locally, with a build of `devel` and one of the update:

```bash
THAPI_REF=$REF/build THAPI_NEW=$NEW/build pytest -vv utils/
```

List the functions added and removed (one `<fn>_ptr_t` typedef per traced function):

```bash
traced() { sed -n 's/.*(CL_API_CALL \*\(cl[A-Za-z0-9]*\)_\(ptr_\)\{0,1\}t).*/\1/p' $1/backends/opencl/tracer_opencl.c | sort -u; }
comm -3 <(traced $REF/build) <(traced $NEW/build)
```

Then on a GPU node, with both builds: trace an OpenCL and a SYCL-on-OpenCL app
(`ONEAPI_DEVICE_SELECTOR=opencl:gpu`), and replay old traces. Compare the event sequence, the pretty
output (`iprof -t`), the tally (`iprof -r <trace>`), and stderr: nothing new may be printed there.
Run `make check` in `backends/opencl`.

# Potential problems, things to check

## `Missing symbol clFooKHR!` printed when an app starts

The tracer looks up the core functions when it loads; KHR/EXT ones through `clGetExtensionFunctionAddress`.
That fails when the implementation does not provide the extension (the GPU runtime lacks most of them),
and no loader exports an extension function. New KHR/EXT functions go into `supported_extensions.yaml`:
they are then traced per platform, when the application gets them with
`clGetExtensionFunctionAddress(ForPlatform)`. A function listed there is never looked up at load time.

## Functions of experimental extensions

`cl.xml` marks them `experimental="true"` (formerly `provisional`), and the headers declare them only with
`CL_ENABLE_BETA_EXTENSIONS`. Symptom: `unknown type name 'cl_command_buffer_khr'`. Their signatures change
between revisions (`cl_khr_command_buffer` 0.9.4 to 0.9.5 added a `properties` argument), so we do not trace
them (`EXPERIMENTAL_FUNCTIONS` in `opencl_model.rb`). Applications still get the implementation's functions.

## New entry points that are not for applications

- `clIcd*KHR` (`cl_khr_icd`): called by the loader on the drivers. Add them to `ABSENT_FUNCTIONS`.
- A vendor suffix not in `VENDOR_EXT` (`clSetContentSizeBufferPoCL`): add the suffix.

## `closure->types[0] = &;` (`expected expression`)

A new opaque handle type (`typedef struct _cl_foo_khr * cl_foo_khr`) has no libffi type. Add it to
`CL_EXT_OBJECTS`.

## `use of undeclared identifier 'ffi_type_...'`

A parameter type maps to a libffi type that does not exist (`int` is `ffi_type_sint`). Fix `FFI_TYPE_MAP`.

## A variable named after its type, in `btx_upstream.c`

`cl_semaphore_khr = (cl_semaphore_khr)...`: the event field of a returned handle is named after its type
(`cl_context` gives `context`). Check `lttng_return_type` strips the new spelling (`cl_semaphore_khr` gives `semaphore`).

## A changed spelling in `cl.xml`

- No parameter used to be one `void` parameter; it is now none (`clUnloadCompiler`). The generator must
  still print `(void)`.
- Look at each `-` line of the generated diff: a fix in the registry (`clCreateFromGLBuffer`'s `errcode_ret`
  became `cl_int *`) is fine, anything else is a bug.

## Name clashes

`conflicting types for ...`: upstream started declaring a name we generate. Example: upstream declares
`<function>_t` as the function type of every entry point (`cl_function_types.h`), hence our pointer typedefs
are named `<function>_ptr_t`.
