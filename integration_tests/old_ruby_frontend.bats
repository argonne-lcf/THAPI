bats_require_minimum_version 1.5.0

# The `iprof` launcher must stay parseable by an old Ruby (the system default on
# some machines) so that, on a too-old interpreter, it prints a friendly version
# message instead of a raw SyntaxError. All modern-syntax logic lives in the
# required `xprof.rb`, which is only parsed after the version gate passes.

setup() {
  # ruby CLI need a path
  iprof_path=$(command -v iprof)

  # Single source of truth: read the minimal ruby version baked into iprof itself
  # (configure.ac may not be present, e.g. CI only ships the install tree). The
  # value keeps its quotes, so it drops straight into the ruby below.
  min_version=$(awk '/^THAPI_RUBY_MINIMAL_VERSION/ {print $3}' "${iprof_path}")

  # The ruby to test against. Defaults to `ruby`; point THAPI_OLD_RUBY at an old
  # interpreter to exercise this on a machine whose default ruby is recent.
  old_ruby="${THAPI_OLD_RUBY:-ruby}"
  if "${old_ruby}" -e "exit(Gem::Version.new(RUBY_VERSION) >= Gem::Version.new(${min_version}))"; then
    skip "${old_ruby} is >= ${min_version}; set THAPI_OLD_RUBY to an older ruby to run this test"
  fi
}

@test "frontend_parses_under_old_ruby" {
  run -0 "${old_ruby}" -c "${iprof_path}"
}

@test "frontend_prints_friendly_message_under_old_ruby" {
  run ! "${old_ruby}" "${iprof_path}" --help
  [[ "${output}" == *"too old"* ]]
  [[ "${output}" != *"SyntaxError"* ]]
  [[ "${output}" != *"unexpected"* ]]
}
