bats_require_minimum_version 1.5.0

# thapi_demangle reads a mangled name on stdin and writes the demangled name on
# stdout (echoed back unchanged when it cannot be demangled). It is on PATH via
# setup_suite (pkg-config --variable=bindir thapi). `thapi_demangle --features`
# lists the demangling backends compiled in.

@test "demangle: baseline symbol is demangled" {
  run -0 bash -c "echo '_Z3fooi' | thapi_demangle"
  [ "$output" = 'foo(int)' ]
}

@test "demangle: symbol is demangled (llvm fallback)" {
  local features
  features=$(thapi_demangle --features)
  if [[ ! "$features" =~ llvm-demangle ]]; then
    skip "built without the llvm::demangle fallback"
  fi
  # A small reproducer, then the full symbol it was reduced from.
  local sym
  for sym in \
    '_ZTSZZ1XIEEUlNS_EE_EUlS_E_' \
    '_ZTSZZN5amrex6launchILi6EZNS_6detail16ParallelFor_doitIN10MGABCTagIEEZNS_11ParallelForIS_ZNS_12MLCellLinOpTIFbEEES_NS_EEUlS_E_EENS_9ectorIT_EET_EUlS_E_EEvS_EUlN7ndLi1EEEE_E11gpuStream_tEENUlN7handlerEE_ES_EUlS_E_'; do
    run -0 bash -c "echo '$sym' | thapi_demangle"
    [ "$output" != "$sym" ]
  done
}
