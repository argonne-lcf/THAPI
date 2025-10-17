bats_require_minimum_version 1.5.0

# Runs the BATS tests that live under each backend directory in
# integration_tests/backends/* (e.g., integration_tests/backends/itt/*.bats).
@test "backends: run all backend test suites (integration_tests/backends/*)" {
  # Get this suite's file location
  local root="${BATS_TEST_DIRNAME}"

  # Ensure we're actually in integration_tests and the backends dir exists
  local backends_root="${root}/backends"
  if [[ ! -d "${backends_root}" ]]; then
    skip "No backends directory found at: ${backends_root}"
  fi

  # Collect backend directories that contain at least one *.bats file.
  local suites=()
  local d
  for d in "${backends_root}"/*; do
    [[ -d "$d" ]] || continue
    if compgen -G "${d}"/*.bats > /dev/null; then
      suites+=("$d")
    fi
  done

  if (( ${#suites[@]} == 0 )); then
    skip "No backend test suites found under ${backends_root}"
  fi

  # Run each backend's BATS tests recursively. If any fails, this test fails.
  local suite
  for suite in "${suites[@]}"; do
    echo "== Running backend suite: $(basename "$suite") =="
    run bats -r "$suite"
    echo "$output"
    [ "$status" -eq 0 ]
  done
}

