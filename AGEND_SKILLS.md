THAPI is splited into 2 main componant:

- The tracer (who generate the LTtng tracepoint)
- The analysis (babeltrace2 pluging who analyse the trace)

#  Convensation for Tracer

- We use LTTNG 
- We try to pass all the information to the tracepoint
  - For pointer, we also dump the "pointee" when it make sence (struct for example)
- For "intercept" tracer (where we intercept call), we split the tracepoint into `_entry`, and `_exit` tracepoint. This allow to debug "hang/ segault" during the function we trace
  - We use metatadata information, stored in yaml file, to know which args are input or output
```
foo(int a_input, * struct b_output) {
  gen_tracepoint(a_input)
  call foo_orig
  gen_tracepoint(b_output, b_output_val)
}

```
- The tracepoint contain the `hostname`, vpid, vtid.
- We try to have one "tracepoint" per function traced.
- The tracer should do the minmiun amount of work. The goal is to as fast as possible (no locking, etc etc), without perturbing the application who traced. All logic, should be in the tracer
- Hence, the trace should have all the required semantic information to allow the analysis to be performance


  # Analsys

  - Done via Babeltrace2
  - Each backend, need to be able to transform their trace into "interval" trace
  - One need to careful to handle `host,process, thread`  (as each trace, can have multiple, hostnames, processes, and threads)
  - Each backemd, need to provide a "pretty-printing" pluging

  # Testing

  We have 2 testings stragey:
  
  - For the analysis, we have "unit-tests" that can be run offline, whitout the requirement to have the "tracing programing model" working
     - We read a text dump of the "raw trace" and then pass it to the babeltrace2 pluging and assert the output match
       - Used for `intervals`, and `pretty-printing`
  - For the "tracer":
    - When possible, we really on "integration-test" who run on the CI (github for now). If possible, run "end-to-end" tests to verify the behavior of the THAPI/iprof
    - For example, run and trace a "real" program

  # Other convention

  - We try hard, to do code-reuse between the multiple backends
  - Some backends, parse the headers of the "traced programing model", to automaticaly generate all the tracepoint mechanism. This is the recommanded way for "intercepd" backend
  - To install dependency one can use `spack`.
  - We try to minizi the ammount of dependencies (as it's hard to package)
  - Refactor to minimize the amount of code, simplify the API, increase coherency are always welcome
   
