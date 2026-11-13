# How to Update the Headers

## Which Version

Never newer than what Aurora / Sunspot provides (same rule as ze), and the headers and `cl.xml` move together.

What matters is the version of the **implementation** (the Intel OpenCL runtime), not the one of the loader:
the loader only forwards calls, while the extension signatures are those the implementation implements.
Use the headers oneAPI ships, which are the ones applications compile against.

The revision an implementation reports can still differ from the one the headers declare, and differ
between devices: the GPU reports `cl_khr_external_memory` 0.9.1, the headers declare 1.0.1. What matters
is the signature of each function, not the revision number: compare the prototypes in both header versions
(identical for `cl_khr_external_memory`; not for `cl_khr_command_buffer`, see "Functions of experimental
extensions").

Current (Aurora stack `26.181.0`, oneAPI `2026.1`, `intel-opencl-25.18`):

- Headers: `https://github.com/KhronosGroup/OpenCL-Headers` at `6eabe90` (2025-06-10, `git describe`: `v2024.10.24-10-g6eabe90`),
  identical to `oneapi/compiler/2026.1/include/CL`. Copied **unmodified** into `include/CL/`.
- Registry: `https://github.com/KhronosGroup/OpenCL-Docs/blob/main/xml/cl.xml` at `5efafb0` (2025-06-10). Copied **unmodified** into `cl.xml`.
  Our changes to it are in `cl.xml.patch`.
- Intel entry points that neither declares: `opencl_missing_apis.h` (hand-written, like `backends/hip/hip_missing_apis.h`).

### Find the versions on Sunspot

```bash
$ module load oneapi
$ clinfo | grep "ICD loader Version"                                     # loader
  ICD loader Version                              3.0.6
$ readlink /opt/aurora/26.181.0/support/libraries/khronos/default        # Khronos SDK (loader + headers)
v2023.12.14
$ O=$(dirname $(which icx))/../include/CL                                # oneAPI headers
$ utils_spec_update/extension_versions.sh $O    # extensions with host functions: runtime vs headers revision
cl_intel_unified_shared_memory          runtime  1.1.0  headers  0.0.0
cl_khr_command_buffer                   runtime  0.9.7  headers  0.9.7
cl_khr_command_buffer_mutable_dispatch  runtime  0.9.3  headers  0.9.3
cl_khr_icd                              runtime  1.0.0  headers  2.0.0
cl_khr_il_program                       runtime  1.0.0  headers  1.0.0
cl_khr_suggested_local_work_size        runtime  1.0.0  headers  1.0.0
```

`extension_versions.sh` lists only the extensions that add host functions (per `cl.xml`) and that a platform or
device on the node reports. A different revision matters only for functions THAPI traces, and then the
prototypes decide (see above). Here: the `cl_khr_icd` functions are the loader's, not traced, and the headers
declare Intel's own extensions `0.0.0`.

Run it on every system you care about, for example a GPU compute node (`ssh` to one of your job's nodes):
each driver reports its own extensions, per device, so different hardware supports different ones. The GPU
has no `cl_khr_command_buffer`, and reports `cl_khr_external_memory` 0.9.1. The loader cannot tell: it only
reports its own name and version, exports no extension function, and forwards
`clGetExtensionFunctionAddress(ForPlatform)` to the drivers it loads (`/etc/OpenCL/vendors/*.icd` or
`OCL_ICD_FILENAMES`; on Sunspot `libigdrcl.so` for the GPU and `libintelocl.so` for the CPU).

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

  An exact match may not exist. For `6eabe90`, no OpenCL-Docs `main` commit of `xml/cl.xml` from 2024-11-12
  to 2026-11-10 (46 tried) reproduces the headers; `5efafb0` leaves 25 lines (two extensions without functions:
  `cl_khr_external_semaphore_dx_fence` and the `cl_khr_kernel_clock` version).
- Copy it unmodified into `cl.xml`, then rebase `cl.xml.patch`:

  ```bash
  cp cl.xml /tmp/cl.xml.patched && patch /tmp/cl.xml.patched cl.xml.patch    # fix the rejects by hand
  { head -2 cl.xml.patch; diff -u cl.xml /tmp/cl.xml.patched | tail -n +3; } > /tmp/new.patch && mv /tmp/new.patch cl.xml.patch
  ```

  Then remove from `cl.xml.patch` every addition that upstream now has, or the patched file defines it twice.
  Some of these hunks are rejected, others still apply cleanly and only show up as duplicates. For `5efafb0`:
  the `cl_icdl_info` typedef was rejected, while the `cl_icdl_info` enum group and the
  `clGetICDLoaderInfoOCLICD` command applied, each giving a second copy. List what the patch defines twice:

  ```bash
  utils_spec_update/cl_xml_duplicates.sh /tmp/cl.xml.patched      # nothing
  ```

  (Grepping `cl.xml` by hand, mind the XML comments: it keeps some retired values in `<!-- -->`, such as
  `CL_DEVICE_PARENT_DEVICE_EXT` `0x4030`, defined once as `0x4054`.)

  The `<require comment=...>` rewordings matter: the generator reads them (see below).

### What `cl.xml.patch` changes

Only the first row is about entry points; the rest adapts the registry to THAPI's generators, mostly
`gen_opencl_model.rb` and the Ruby pretty printer that reads `opencl_model.yaml` (`babeltrace_opencl_lib.rb`).
"Without it" was checked for `5efafb0` by reverting each kind of hunk and regenerating the files.

| Hunk | Why | Without it |
|---|---|---|
| 10 Intel `<command>`s (`clGetExecutionInfoINTEL`, ...) and the `cl_execution_info_intel` typedef | Intel entry points that neither the headers nor upstream `cl.xml` declare. The same list as `opencl_missing_apis.h` (`check_missing_apis.sh` checks both agree). | not traced |
| A type name put in a `<require comment>` (`cl_device_unified_shared_memory_capabilities_intel - bitfield`, `cl_command_type for events created with clEnqueueAcquireEGLObjectsKHR`, `Error codes for clCreateFromEGLImageKHR`) | `gen_opencl_model.rb` gives each type of `supported_enums.yaml` the values of the `<require>`s whose comment contains its name, followed by a space or the end. | a type no comment names crashes the generator (`reduce(:+)` on `nil`); otherwise the values are missing from `opencl_model.yaml` |
| `cl_affinity_domain_ext` typedef, and the affinity-domain comment renamed to it | `supported_enums.yaml` lists that type; upstream files those values under `cl_device_partition_property_ext`. | the generator crashes |
| ` - bitfield` added to a comment | marks the type as a bitfield | the type is an enum in `opencl_model.yaml`: one name printed instead of OR'ed flags |
| `MiscNumbers`: C casts replaced by plain values | the Ruby printer `eval`s each value; a cast does not evaluate | those values are printed without a name |
| 8 `_KHR` aliases of core 3.0 enums removed (`CL_PLATFORM_NUMERIC_VERSION_KHR`, ...) | same value as the core name; the Ruby printer's value-to-name hash keeps the last one | `CL_PLATFORM_NUMERIC_VERSION_KHR` printed instead of `CL_PLATFORM_NUMERIC_VERSION` |

Hunks that changed no generated file were dropped (the `cl_image_desc` union written as one `cl_mem buffer`
member, a `cl_program_info` comment upstream already matches, a blank line).
Apart from the crashes, only the first row changes the traced API: for the others, `btx_cl_model.yaml`,
`tracer_opencl.c` and `opencl_tracepoints.tp` stay the same, and only `opencl_model.yaml` changes. The
removed `_KHR` names still appear there, with no value, because `cl_khr_extended_versioning` requires them.

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

The tracer looks up the core functions when it loads; KHR/EXT ones through the platform-less
`clGetExtensionFunctionAddress`, since no loader exports an extension function. That lookup can return
NULL even where the implementation provides the function: on the Sunspot CPU platform,
`clCreateCommandBufferKHR` is advertised and `clGetExtensionFunctionAddressForPlatform` returns it, but
`clGetExtensionFunctionAddress` returns NULL. It also returns NULL when no platform provides the extension
(the GPU platform has no command buffers and no semaphores). New KHR/EXT functions go into `supported_extensions.yaml`:
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
