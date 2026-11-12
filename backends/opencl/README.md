# How to Update the Headers

## Header Location

- Headers: `https://github.com/KhronosGroup/OpenCL-Headers` (the `CL/` folder), copied **unmodified** into `include/CL/`.
  - Current: `82f6031` (2026-10-22, `git describe`: `v2026.05.29-11-g82f6031`).
- Intel entry points that no public header declares: `opencl_missing_apis.h` (hand-written, like `backends/hip/hip_missing_apis.h`).
- `cl.xml` (the registry) is separate and is not covered here. `cl.xml.patch` adds the Intel functions to it.

# Steps

## 1/ Copy the headers

```bash
git clone https://github.com/KhronosGroup/OpenCL-Headers.git
git -C OpenCL-Headers checkout $commit_or_tag          # default: latest main
git -C OpenCL-Headers describe --tags                  # write it in "Current" above
rm include/CL/*.h
cp OpenCL-Headers/CL/*.h include/CL/
git status include/CL                                  # new / removed files upstream
```

- Do not edit them. Anything they lack goes into `opencl_missing_apis.h`. That way the review is just
  `diff -r OpenCL-Headers/CL include/CL`.
- `.gitignore` ignores `*.h` but re-includes `include/CL/*.h`, so new upstream headers show up in `git status`.

## 2/ Check what we include

`cl.h.include`, `gen_opencl.rb` and `gen_opencl_probes.rb` include headers by name.

Upstream deprecates a header by turning it into a stub with a `#pragma message` and no include guard.
Do not include those: gcc rejects the pragma when lttng re-includes the tracepoint header. Their content
is usually already reachable from `opencl.h`. This is why we stopped including `cl_gl_ext.h` and `cl_ext_intel.h`.

```bash
$ grep -l 'pragma message' include/CL/*.h
include/CL/cl_dx9_media_sharing_intel.h
include/CL/cl_ext_intel.h
include/CL/cl_gl_ext.h
include/CL/cl_version.h
```

(`cl_version.h` is fine: its `#pragma message` only fires when `CL_TARGET_OPENCL_VERSION` is missing or
invalid, and the three files above define it.)

## 3/ Update `opencl_missing_apis.h`

A traced function that no header declares still compiles, even with `-Werror` (a missing *type* is a
compile error). So check it with the script:

```bash
make -C $BUILD/backends/opencl tracer_opencl.c
utils_spec_update/check_missing_apis.sh $BUILD/backends/opencl/tracer_opencl.c
```

- No output: nothing to do.
- `+clFoo`: traced, but no header declares it. Add it to `opencl_missing_apis.h`, with its signature from `cl.xml.patch`.
- `-clFoo`: upstream now declares it. Remove it from `opencl_missing_apis.h`, and any type that came with it.

## 4/ Build

- Build with both gcc and clang, keeping `-Werror` (the default, `--enable-strict`). For clang, CI adds
  `CFLAGS=-Wno-gnu-folding-constant`.
- `conflicting types for ...`: upstream started declaring a name we generate.
  Example: upstream declares `<function>_t` as the function type of every entry point (`cl_function_types.h`),
  hence our pointer typedefs are named `<function>_ptr_t`.
- `make check` in `backends/opencl`.

## 5/ Compare the generated files

A header refresh should not change the generated code. The CI job "Assert generated files are equivalent"
builds the generated files of the base branch and of the PR, then runs `utils/test_compare_generated_file.py`.
Locally, with a build of each:

```bash
THAPI_REF=$REF/build THAPI_NEW=$NEW/build pytest -vv utils/
```

Any diff must be explained in the PR.
