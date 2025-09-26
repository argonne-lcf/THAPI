# THAPI (Tracing Heterogeneous APIs)

![Static Badge](https://img.shields.io/badge/Aurora-ready-blue)
![Static Badge](https://img.shields.io/badge/Found%20a%20bug%3F-Report%20it!-red?logo=github&logoColor=white&labelColor=blue)

**THAPI** (Tracing Heterogeneous APIs) is a tracing infrastructure for heterogeneous computing applications. It currently includes backends for:

* CUDA (runtime and driver)
* OpenCL
* [Intel Level Zero (L0)](https://www.intel.com/content/www/us/en/docs/dpcpp-cpp-compiler/developer-guide-reference/2023-0/intel-oneapi-level-zero.html)
* MPI
* OpenMP
* [CXI](https://github.com/HewlettPackard/shs-libcxi)


Quick usage example:

```bash
$ mpirun -n $N -- iprof -- ./a.out
API calls | 1 Hostnames | 1 Processes | 1 Threads

                         Name |     Time | Time(%) | Calls |  Average |      Min |      Max | Failed |
     cuDevicePrimaryCtxRetain |  54.64ms |  51.77% |     1 |  54.64ms |  54.64ms |  54.64ms |      0 |
         cuMemcpyDtoHAsync_v2 |  24.11ms |  22.85% |     1 |  24.11ms |  24.11ms |  24.11ms |      0 |
[...]
                  cuDeviceGet | 640.00ns |   0.00% |     1 | 640.00ns | 640.00ns | 640.00ns |      0 |
             cuDeviceGetCount | 460.00ns |   0.00% |     1 | 460.00ns | 460.00ns | 460.00ns |      0 |
                        Total | 105.54ms | 100.00% |    98 |                                       1 |
```

More info in the [usage](#usage) section and in our [selections of amazing (⸮) talks](https://github.com/Kerilk/THAPI-Tutorial)

# Building and Installation

We recommend installing THAPI via [Spack](https://github.com/spack/spack).

THAPI package is not (yet) in upstream spack. In the mean time, please follow the instructions in
[THAPI-spack](https://github.com/argonne-lcf/THAPI-spack). 

Once you have the `THAPI-spack` repo added to your Spack configuration, you should be able to:

```bash
spack install thapi
```

## Build from source (Autotools)

If you prefer to build from source, THAPI uses a classic Autotools flow:

```bash
./autogen.sh
mkdir build
cd build
../configure --prefix `pwd`/ici
make -j install
```

Adjust `--prefix` to your preferred installation directory (and please don't copy my ugly bash with backticks and naming convension...).

<details>

<summary>Dependencies details</summary>

### Dependencies

Packages:
 - `babeltrace2`, `libbabeltrace2-dev`
 - `liblttng-ust-dev`
 - `lttng-tools`
 - `ruby`, `ruby-dev`
 - `libffi`, `libffi-dev`

Note: Some package should be patched before install see [associated Spack package](https://github.com/argonne-lcf/THAPI-spack).

Optional packages:
 - `binutils-dev` or `libiberty-dev` for demangling depending on platforms (`demangle.h`)

Ruby Gems:
 - `cast-to-yaml`
 - `nokogiri`
 - `babeltrace2`
 - `metababel`

Optional Gem:
 - `opencl_ruby_ffi`

Optional pip:
 - `h2yaml`

</details>

# Usage

## iprof


`iprof` is the main user-facing tool. The typical way to profile an MPI application is:

```bash
mpirun -n $N -- iprof -- ./a.out <app-args>
```

`iprof` supports three primary output analysis:

### Analysis

1. **Tally (default)** — aggregated per-API statistics (time, calls, averages). This is the default when you run `iprof` without additional flags.
2. **Timeline** — `iprof -l -- ...` it produces a timeline trace suitable for visualization in tools like [Perfetto](https://ui.perfetto.dev/)
3. **Detailed traces** — with `irpof -t --` you get detailed LTTng traces.

> Use `iprof --help` to get a full list of options.

#### Tally

```
tapplencourt> iprof ./a.out
API calls | 1 Hostnames | 1 Processes | 1 Threads

                         Name |     Time | Time(%) | Calls |  Average |      Min |      Max | Failed |
     cuDevicePrimaryCtxRetain |  54.64ms |  51.77% |     1 |  54.64ms |  54.64ms |  54.64ms |      0 |
         cuMemcpyDtoHAsync_v2 |  24.11ms |  22.85% |     1 |  24.11ms |  24.11ms |  24.11ms |      0 |
 cuDevicePrimaryCtxRelease_v2 |  18.16ms |  17.20% |     1 |  18.16ms |  18.16ms |  18.16ms |      0 |
           cuModuleLoadDataEx |   4.73ms |   4.48% |     1 |   4.73ms |   4.73ms |   4.73ms |      0 |
               cuModuleUnload |   1.30ms |   1.23% |     1 |   1.30ms |   1.30ms |   1.30ms |      0 |
               cuLaunchKernel |   1.05ms |   0.99% |     1 |   1.05ms |   1.05ms |   1.05ms |      0 |
                cuMemAlloc_v2 | 970.60us |   0.92% |     1 | 970.60us | 970.60us | 970.60us |      0 |
               cuStreamCreate | 402.21us |   0.38% |    32 |  12.57us |   1.58us | 183.49us |      0 |
           cuStreamDestroy_v2 | 103.36us |   0.10% |    32 |   3.23us |   2.81us |   8.80us |      0 |
              cuMemcpyDtoH_v2 |  36.17us |   0.03% |     1 |  36.17us |  36.17us |  36.17us |      0 |
         cuMemcpyHtoDAsync_v2 |  13.11us |   0.01% |     1 |  13.11us |  13.11us |  13.11us |      0 |
          cuStreamSynchronize |   8.77us |   0.01% |     1 |   8.77us |   8.77us |   8.77us |      0 |
              cuCtxSetCurrent |   5.47us |   0.01% |     9 | 607.78ns | 220.00ns |   1.74us |      0 |
         cuDeviceGetAttribute |   2.71us |   0.00% |     3 | 903.33ns | 490.00ns |   1.71us |      0 |
   cuDevicePrimaryCtxGetState |   2.70us |   0.00% |     1 |   2.70us |   2.70us |   2.70us |      0 |
                cuCtxGetLimit |   2.30us |   0.00% |     2 |   1.15us | 510.00ns |   1.79us |      0 |
         cuModuleGetGlobal_v2 |   2.24us |   0.00% |     2 |   1.12us | 440.00ns |   1.80us |      1 |
                       cuInit |   1.65us |   0.00% |     1 |   1.65us |   1.65us |   1.65us |      0 |
          cuModuleGetFunction |   1.61us |   0.00% |     1 |   1.61us |   1.61us |   1.61us |      0 |
           cuFuncGetAttribute |   1.00us |   0.00% |     1 |   1.00us |   1.00us |   1.00us |      0 |
               cuCtxGetDevice | 850.00ns |   0.00% |     1 | 850.00ns | 850.00ns | 850.00ns |      0 |
cuDevicePrimaryCtxSetFlags_v2 | 670.00ns |   0.00% |     1 | 670.00ns | 670.00ns | 670.00ns |      0 |
                  cuDeviceGet | 640.00ns |   0.00% |     1 | 640.00ns | 640.00ns | 640.00ns |      0 |
             cuDeviceGetCount | 460.00ns |   0.00% |     1 | 460.00ns | 460.00ns | 460.00ns |      0 |
                        Total | 105.54ms | 100.00% |    98 |                                       1 |

Device profiling | 1 Hostnames | 1 Processes | 1 Threads | 1 Device pointers

                Name |    Time | Time(%) | Calls | Average |     Min |     Max |
  test_target__teams | 25.14ms |  99.80% |     1 | 25.14ms | 25.14ms | 25.14ms |
     cuMemcpyDtoH_v2 | 24.35us |   0.10% |     1 | 24.35us | 24.35us | 24.35us |
cuMemcpyDtoHAsync_v2 | 18.14us |   0.07% |     1 | 18.14us | 18.14us | 18.14us |
cuMemcpyHtoDAsync_v2 |  8.77us |   0.03% |     1 |  8.77us |  8.77us |  8.77us |
               Total | 25.19ms | 100.00% |     4 |

Explicit memory traffic | 1 Hostnames | 1 Processes | 1 Threads

                Name |  Byte | Byte(%) | Calls | Average |   Min |   Max |
cuMemcpyHtoDAsync_v2 | 4.00B |  44.44% |     1 |   4.00B | 4.00B | 4.00B |
cuMemcpyDtoHAsync_v2 | 4.00B |  44.44% |     1 |   4.00B | 4.00B | 4.00B |
     cuMemcpyDtoH_v2 | 1.00B |  11.11% |     1 |   1.00B | 1.00B | 1.00B |
               Total | 9.00B | 100.00% |     3 |
```

#### Timeline

```bash
iprof -l -- ./a.out
# produces a .pb or trace file that can be opened with Perfetto UI:
# https://ui.perfetto.dev/
```


#### LTTng trace:

```bash
iprof -t -- ./a.out
```

## Stand-alone tracers (low-level / hacking)

For development and quick experiments, (and for bash lover), THAPI provides back-end-specific wrapper scripts 
named `tracer_$backend.sh` (for example `tracer_opencl.sh`, `tracer_cuda.sh`, ...). 
These are small helper scripts around LTTng that let you manually tune which events are traced and how.

Example usage help for `tracer_opencl.sh`
```
tracer_opencl.sh [options] [--] <application> <application-arguments>
  --help                        Show this screen
  --version                     Print the version string
  -l, --lightweight             Filter out som high traffic functions
  -p, --profiling               Enable profiling
  -s, --source                  Dump program sources to disk
  -a, --arguments               Dump argument and kernel infos
  -b, --build                   Dump program build infos
  -h, --host-profile            Gather precise host profiling information
  -d, --dump                    Dump kernels input and output to disk
  -i, --iteration VALUE         Dump inputs and outputs for kernel with enqueue counter VALUE
  -s, --iteration-start VALUE   Dump inputs and outputs for kernels starting with enqueue counter VALUE
  -e, --iteration-end VALUE     Dump inputs and outputs for kernels until enqueue counter VALUE
  -v, --visualize               Visualize trace on the fly
  --devices                     Dump devices information
```

Traces can be viewed using Efficios `babeltrace2`, or our own `babeltrace_thapi`. The later should give more structured
information at the cost of speed.
