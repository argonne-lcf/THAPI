# THAPI (Tracing Heterogeneous APIs)

A tracing infrastructure for heterogeneous computing applications. We curently have backend for OpenCL, CUDA and L0.

# Building and Installation

The build system is a classical autotool based system.

As a alternative, one can use [spack](https://github.com/spack/spack) to install THAPI.  
THAPI package is not yet in upstream spack, in the mean time please follow https://github.com/argonne-lcf/THAPI-spack.

## Dependencies

Packages:
 - `babeltrace2`, `libbabeltrace2-dev`
 - `liblttng-ust-dev`
 - `lttng-tools`
 - `ruby`, `ruby-dev`
 - `libffi`, `libffi-dev`

babletrace2 should be patched before install, see:
https://github.com/Kerilk/spack/tree/develop/var/spack/repos/builtin/packages/babeltrace2

Optional packages:
 - `binutils-dev` or `libiberty-dev` for demangling depending on platforms (`demangle.h`)

Ruby Gems:
 - `cast-to-yaml`
 - `nokogiri`
 - `babeltrace2`

Optional Gem:
 - `opencl_ruby_ffi`

# Usage

## OpenCL Tracer

The tracer can be heavily tuned and each event can be monitored independently from others, but for convenience a series of default presets are defined in the `tracer_opencl.sh` script:
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
  -v, --visualize               Visualize trace on thefly
  --devices                     Dump devices information
```

Traces can be viewed using `babeltrace`, `babeltrace2` or `babeltrace_opencl`. The later should give more structured information at the cost of speed.

## Level Zero (L0) Tracer

Similarly to OpenCL, a wrapper script with presets is provided, `tracer_ze.sh`:
```
tracer_ze.sh [options] [--] <application> <application-arguments>
  --help                        Show this screen
  --version                     Print the version string
  -b, --build                   Dump module build infos
  -p, --profiling               Enable profiling
  -v, --visualize               Visualize trace on thefly
  --properties                  Dump drivers and devices properties
```
Traces can be viewed using `babeltrace`, `babeltrace2` or `babeltrace_ze`. The later should give more structured information at the cost of speed.

## CUDA Tracer

Similarly to OpenCL, a wrapper script with presets is provided, `tracer_cuda.sh`:
```
 tracer_cuda.sh [options] [--] <application> <application-arguments>
  --help                        Show this screen
  --version                     Print the version string
  --cudart                      Trace CUDA runtime on top of CUDA driver
  -a, --arguments               Extract argument infos and values
  -p, --profiling               Enable profiling
  -e, --exports                 Trace export functions
  -v, --visualize               Visualize trace on thefly
  --properties                  Dump devices infos
```
 Traces can be viewed using `babeltrace`, `babeltrace2` or `babeltrace_cuda`. The later should give more structured information at the cost of speed

## iprof

`iprof` is another wrapper around the OpenCL, Level Zero, and CUDA tracers. It gives aggregated profiling information.

```
Usage: iprof [options]
    -m, --tracing-mode=MODE          Define the category of events traced
        --traced-ranks=RANK          Select with MPI rank will be traced.
                                     Use -1 to mean all ranks.
                                     Default: -1
        --[no-]profile               Device activities will not profiled
    -b, --backend BACKEND            Select which and how backends' need to handled.
                                     Format: backend_name[:backend_level],...
                                     Default: omp:2,cl:1,ze:1,cuda:1,hip:1
    -r, --replay [PATH]              Replay traces for post-morten analysis
    -t, --trace                      Pretty print the trace
    -l, --timeline                   Dump a timeline of the trace.
                                     This will create a 'out.pftrace' file that can be opened in perfetto: https://ui.perfetto.dev/#!/viewer
    -j, --json                       The tally will be dumped as json
    -e, --extended                   The tally will be printed for each Hostname / Process / Thread / Device
    -k, --kernel-verbose             The tally will report kernels execution time with SIMD width and global/local sizes
        --max-name-size SIZE         Maximum size allowed for kernels names.
                                     Use -1 to mean no limit.
                                     Default: 80
        --metadata                   Display trace Metadata
    -v, --version                    Display THAPI version
        --debug [LEVEL]              Level of debug [default 0]
                                                      __
For complaints, praises, or bug reports please use: <(o )___
   https://github.com/argonne-lcf/THAPI              ( ._> /
   or send email to {apl,bvideau}@anl.gov             `---'
```

Programming model specific variants exist: clprof.sh, zeprof.sh, and cuprof.sh.

### Example of iprof output when tracing cuda code

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
