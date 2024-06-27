# libthapictl

This library is desiged to allow applications to control event collection, similar to
how cudaProfilerStart and cudaProfilerStop work.

## Design Considerations

The simplest design would be to stop and start the entire lttng session. However many
backends use complex chains of API calls to figure out the device context of things like
memory operations and kernel launches on device. Collection of events for these API
calls must never be disabled or thapi will be unable to correctly identify future
device operations. We call these API calls "bookkeeping" calls.

In addition to "bookkeeping" events, there are additional categories for backends that
support "minimal", "default", and/or "full" tracing modes, and device profiling can also
be enabled or disabled independently. This creates further categories.

### Design 1: two sessions

The initial idea was to create two lttng sessions, one that would collect all the
"bookkeeping" events and one that would collect everything else (the "main" session). Then
thapi_ctl_start/stop could simply start/stop the main session, and always leave the
bookkeeping session running. This worked well enough for CUDA, except when the
"--no-tracing-from-start" option was used. This option causes start to not be run
unless the user calls it, so during cuInit, only the "bookkeeping" session is active.

The CUDA runtime in particular has special handling for dynamic symbols, using the
cuGetProcAddress_v2 API call as the hook to wrap dynamically loaded symbols. This works
fine if both bookkeeping and main sessions are running when cuGetProcAddress loads all
the symbols (during cuInit, triggered by first CUDA runtime call), and both are configured
to intercept the entry/exit events for that call. However if main is not started, then
it never gets to wrap the dynamic symbols, and cuGetProcAddress is not called again.

OpenCL does not have this complication, and works well with this method. ZE may be similarly complex to CUDA, and HIP may also be once we add device profiling.

We could not support "--no-tracing-from-start", and make it the user's responsibility to
call thapi_ctl_stop in the beginning of the program but AFTER cuInit and similar, but this
is fragile and hard to document. Another possiblility would be to try to force cuInit
right after tracer init time, but this is also a very invasive thing for a tracing
framework to do. For this reason, we came up with another design:

### Design 2: fine grained event enable/disable

Rather than put "bookkeeping" in a different category, we can have libthapictl know which
events are bookkeeping and which events to enable/disable for start stop. To avoid
duplicating this knowledge in xprof ruby code AND in libthapictl, all this knowledge
can be moved into libthapictl, and a "thapi-ctl" binary that is a thin wrapper around
the library start/stop can be called from xprof.

There are three commands:

init: enable bookkeeping events (always called)
start: enable tracing events, according to options requested by user (tracing mode, device profiling on/off, which backends are enabled)
stop: disable tracing events

By default tracing from start is enabled, so both init and start are called for each enabled
backend. Then the profiled application can link libthapi-ctl and call thapi_ctl_stop/start
as desired.

This approach is more complex than (1), but it is far more likely to work for all backends
without gotchas for the user like call stop before init breaking everything.

## Environment variables

Things like the LTTNG_HOME, session name, and channel name, enabled backends, and tracing
mode are passed from xprof to thapi-ctl and libthapictl via environment variables, which
are memoized with static C functions / static function vars.

## Testing

This is a very invasive change to THAPI; in addition to testing the new functionality of
starting/stopping tracing, we need to test that existing functionality like tracing modes
is not broken. To this end new integration tests are being added, that can't be run on
CI because of hardware requirements but should be easy to run manually on machines with
appropriate hardware. See libthapictl and gtensor integration test directories.
