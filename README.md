# THAPI (Tracing Heterogeneous APIs)

A tracing infrastructure for heterogeneous computing applications.

# Building and Installation

The build system is a classical autotool based system.

As a alternative one case use [spack](https://github.com/spack/spack) to install THAPI.  
THAPI package is not in upstream spack, so in the mean time please use https://github.com/argonne-lcf/THAPI-spack.

## Dependencies

Packages:
 - `babeltrace`, `libbabeltrace-dev`
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
 - `babeltrace`
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
iprof: a tracer / summarizer of OpenCL, L0, and CUDA driver calls
Usage:
 iprof -h | --help 
 iprof [option]... <application> <application-arguments>
 iprof [option]... -r [<trace>]...

  -h, --help         Show this screen
  -v, --version      Print the version string
  -e, --extended     Print information for each Hostname / Process / Thread / Device
  -t, --trace        Display trace
  -l, --timeline     Dump the timeline
  -m, --mangle       Use mangled name
  -j, --json         Summary will be printed as json
  -a, --asm          Dump in your current directory low level kernels informations (asm,isa,visa,...).
  -f, --full         All API calls will be traced. By default and for performance raison, some of them will be ignored
  --metadata         Display metadata
  --max-name-size    Maximun size allowed for names
  -r, --replay       <application> <application-arguments> will be traited as pathes to traces folders (/home/videau/lttng-traces/...)
                     If no arguments are provided, will use the latest trace available

 Example:
 iprof ./a.out

iprof will save the trace in /home/videau/lttng-traces/
 Please tidy up from time to time
                                                   __
For complain, praise, or bug reports please use: <(o )___
   https://github.com/argonne-lcf/THAPI           ( ._> /
   or send email to {apl,bvideau}@anl.gov          `---'
```

Programming model specific variants exist: clprof.sh, zeprof.sh, and cuprof.sh.
