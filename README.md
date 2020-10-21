# THAPI (Tracing Heterogeneous APIs)

A tracing infrastructure for heterogeneous computing applications.

# Building and Installation

The build system is a classical autotool based system.

## Dependencies

Packages:
 - `babeltrace`, `libbabeltrace-dev`
 - `babeltrace2`, `libbabeltrace2-dev`
 - `liblttng-ust-dev`
 - `lttng-tools`
 - `ruby`, `ruby-dev`
 - `libffi`, `libffi-dev`

 Ruby Gems:
 - `cast-to-yaml`
 - `nokogiri`
 - `babeltrace`

# Usage

## OpenCL Tracer

The tracer can be heavily tuned and each event can be monitored independently from others, but for convenience a series of default presets are defined in the `tracer_opencl.sh` script:
```
tracer_opencl.sh [options] -- cmd
    -l | --lightweight : remove kernel argument related calls from the trace
    -p | --profiling : enable kernel profiling
    -s | --source : dump kernel source in /tmp, path will be given in the trace. text, IL and binary sources are dumped.
    -a | --arguments : get extended information about kernels and their arguments
    -b | --build : get extended information about kernel build info and dump built binaries and objects
    -h | --host-profile : enable precise host profiling
    -d | --dump : dump kernel parameters and buffers to disk to be able to replay tem out of the application
    -i | --iteration ITER_NUMBER : specify the rank of the enqueue to dump
    -s | --iteration-start ITER_NUMBER : specify the rank of the enqueue to start dumping
    -e | --iteration-end ITER_NUMBER : specify the rank of the enqueue to start dumping (included)
    -v | --visualize : interactive vizualization of the trace using babeltrace
    --devices : systematically dump device information when DeviceIDs are queried
```

Traces can be viewed using `babeltrace`, `babeltrace2` or `babeltrace_opencl`. The later should give more structured information at the cost of speed.

## Level Zero Tracer

Similarly to OpenCL, a wrapper script with presets is provided, `tracer_ze.sh`:
```
tracer_ze.sh [options] -- cmd
    -p | --profiling : enable kernel profiling
    -v | --visualize : interactive vizualization of the trace using babeltrace
```
Traces can be viewed using `babeltrace`, `babeltrace2` or `babeltrace_ze`. The later should give more structured information at the cost of speed.

## clprof.sh (iprof)

`clprof.sh` also often aliased as `iprof` is another wrapper around the OpenCL tracer. It gives aggregated profiling information.

```
iprof: a tracing / summarizer of OpenCL Calls
Usage:
 iprof -h | --help 
 iprof [option]... <application> <application-arguments>
 iprof [option]... -r [<trace>]

  -h, --help         Show this screen
  -e, --extended     Print information for each Hostname / Process / Thread / Device
  -t, --trace        Display the trace
  -a, --asm          Dump in your current directory low level kernels informations (asm,isa,visa,...).
  -r, --replay       Allow you to replay an old trace. Take a path as argument (/home/videau/lttng-traces/...)
                     If no argument is provided, will use the latest trace available

 Example:
 iprof ./a.out

iprof will save the trace in $HOME/lttng-traces/
Please tidy up from time to time
                                                       __ 
 For complain, praise, bug repport please use:       <(o )___
    https://xgitlab.cels.anl.gov/heteroflow/tracer    ( ._> /
    or send email to apl@anl.gov | bvideau@anl.gov     `---'
```
