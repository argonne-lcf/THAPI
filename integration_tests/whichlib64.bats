bats_require_minimum_version 1.5.0

# ── Helpers ──────────────────────────────────────────────────────────

build_noop() {
  echo 'int main(void){return 0;}' | gcc -x c -o "$BATS_TEST_TMPDIR/noop" -
}

build_libbar() {
  local dir="${1:-$BATS_TEST_TMPDIR}"
  gcc -shared -o "$dir/libbar.so" "$BATS_TEST_DIRNAME/bar.c"
}

build_libbar_versioned() {
  local dir="${1:-$BATS_TEST_TMPDIR}"
  gcc -shared -Wl,-soname,libbar.so.1 -o "$dir/libbar.so.1" "$BATS_TEST_DIRNAME/bar.c"
  ln -s libbar.so.1 "$dir/libbar.so"
}

build_foo_with_rpath() {
  local libdir="${1:-$BATS_TEST_TMPDIR}"
  gcc -o "$BATS_TEST_TMPDIR/foo" "$BATS_TEST_DIRNAME/foo.c" -L"$libdir" -lbar -Wl,-rpath,"$libdir"
}

build_foo_no_rpath() {
  local libdir="${1:-$BATS_TEST_TMPDIR}"
  gcc -o "$BATS_TEST_TMPDIR/foo" "$BATS_TEST_DIRNAME/foo.c" -L"$libdir" -lbar
  patchelf --remove-rpath "$BATS_TEST_TMPDIR/foo"
}

setup_ldconf() {
  local libdir="$1"
  mkdir -p "$BATS_TEST_TMPDIR/confdir"
  echo "$libdir" >"$BATS_TEST_TMPDIR/confdir/test.conf"
  echo "include $BATS_TEST_TMPDIR/confdir/*.conf" >"$BATS_TEST_TMPDIR/ld.so.conf"
}

# ── Usage ─────────────────────────────────────────────────────────────

@test "usage: no arguments prints help and exits 1" {
  run -1 --separate-stderr whichlib64
  [[ "$stderr" == *"Usage:"* ]]
}

# ── Matrix: no soversion ──────────────────────────────────────────────

@test "row 1: no sover, no DT_NEEDED, not findable → not found (exit 1)" {
  build_noop
  run -1 whichlib64 "$BATS_TEST_TMPDIR/noop" libdoesnotexist.so
  [ "$output" = "libdoesnotexist.so not found" ]
}

@test "row 2: no sover, no DT_NEEDED, unversioned on disk → found (exit 0)" {
  build_libbar
  build_noop
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/noop" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ -z "$stderr" ]
}

@test "row 4: no sover, DT_NEEDED unversioned → found (exit 0)" {
  build_libbar
  build_foo_with_rpath
  run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ -z "$stderr" ]
}

@test "row 5: no sover, DT_NEEDED versioned → warning (exit 2)" {
  build_libbar_versioned
  build_foo_with_rpath
  run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so.1" ]
  [[ "$stderr" == *"Warning: no soversion specified for libbar.so but found versioned libbar.so.1"* ]]
}

# ── Matrix: with soversion ────────────────────────────────────────────

@test "row 6: sover 1, no DT_NEEDED, not findable → not found (exit 1)" {
  build_noop
  run -1 whichlib64 "$BATS_TEST_TMPDIR/noop" libdoesnotexist.so:1
  [ "$output" = "libdoesnotexist.so not found" ]
}

@test "row 7: sover 1, no DT_NEEDED, unversioned on disk → found (exit 0)" {
  build_libbar
  build_noop
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/noop" libbar.so:1
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ -z "$stderr" ]
}

@test "row 9: sover 1, DT_NEEDED unversioned → found (exit 0)" {
  build_libbar
  build_foo_with_rpath
  run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so:1
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ -z "$stderr" ]
}

@test "row 10: sover 1, DT_NEEDED versioned matching → found (exit 0)" {
  build_libbar_versioned
  build_foo_with_rpath
  run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so:1
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so.1" ]
  [ -z "$stderr" ]
}

@test "row 11: sover 2, DT_NEEDED versioned mismatch → warning (exit 2)" {
  build_libbar_versioned
  build_foo_with_rpath
  run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so:2
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so.1" ]
  [[ "$stderr" == *"Warning: expected libbar.so.2 but found libbar.so.1"* ]]
}

# ── Edge cases ────────────────────────────────────────────────────────

@test "find_lib locates a library through a symlink" {
  gcc -shared -o "$BATS_TEST_TMPDIR/lib_foo.so" "$BATS_TEST_DIRNAME/bar.c"
  ln -s lib_foo.so "$BATS_TEST_TMPDIR/lib_foo_symlink.so"
  gcc -o "$BATS_TEST_TMPDIR/foo_symlink" "$BATS_TEST_DIRNAME/foo.c" \
    -L"$BATS_TEST_TMPDIR" -l:lib_foo_symlink.so -Wl,-rpath,"$BATS_TEST_TMPDIR"
  run -0 whichlib64 "$BATS_TEST_TMPDIR/foo_symlink" lib_foo.so
  [ "$output" = "$BATS_TEST_TMPDIR/lib_foo.so" ]
}

@test "find_lib skips a 32-bit library" {
  build_libbar
  build_foo_with_rpath
  printf '\x7fELF\x01' >"$BATS_TEST_TMPDIR/libbar.so"
  dd if=/dev/zero bs=1 count=11 >>"$BATS_TEST_TMPDIR/libbar.so" 2>/dev/null
  run -1 whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
}

@test "find_lib skips a 32-bit library via dlopen" {
  printf '\x7fELF\x01' >"$BATS_TEST_TMPDIR/libbar.so"
  dd if=/dev/zero bs=1 count=11 >>"$BATS_TEST_TMPDIR/libbar.so" 2>/dev/null
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -1 whichlib64 "" libbar.so
}

@test "find_lib handles nonexistent binary" {
  run -1 whichlib64 /no/such/binary libbar.so
}

@test "find_lib handles binary that is not an ELF" {
  echo "not an elf" >"$BATS_TEST_TMPDIR/textfile"
  run -1 whichlib64 "$BATS_TEST_TMPDIR/textfile" libbar.so
}

@test "find_lib resolves \$ORIGIN in rpath" {
  mkdir -p "$BATS_TEST_TMPDIR/bin" "$BATS_TEST_TMPDIR/lib"
  build_libbar "$BATS_TEST_TMPDIR/lib"
  gcc -o "$BATS_TEST_TMPDIR/bin/foo" "$BATS_TEST_DIRNAME/foo.c" \
    -L"$BATS_TEST_TMPDIR/lib" -lbar -Wl,-rpath,'$ORIGIN/../lib'
  run -0 whichlib64 "$BATS_TEST_TMPDIR/bin/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/bin/../lib/libbar.so" ]
}

@test "find_lib prefers rpath over LD_LIBRARY_PATH" {
  mkdir -p "$BATS_TEST_TMPDIR/rpath_dir" "$BATS_TEST_TMPDIR/ldpath_dir"
  build_libbar_versioned "$BATS_TEST_TMPDIR/rpath_dir"
  build_libbar_versioned "$BATS_TEST_TMPDIR/ldpath_dir"
  gcc -o "$BATS_TEST_TMPDIR/foo" "$BATS_TEST_DIRNAME/foo.c" \
    -L"$BATS_TEST_TMPDIR/rpath_dir" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR/rpath_dir"
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR/ldpath_dir" \
    run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/rpath_dir/libbar.so.1" ]
}

# bats test_tags=need_patchelf
@test "find_lib resolves DT_NEEDED soname via dlopen when no rpath" {
  build_libbar_versioned
  build_foo_no_rpath
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so.1" ]
}

@test "find_lib locates a library via LD_LIBRARY_PATH (no rpath)" {
  build_libbar
  gcc -o "$BATS_TEST_TMPDIR/foo_norpath" "$BATS_TEST_DIRNAME/foo.c" -L"$BATS_TEST_TMPDIR" -lbar
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -0 whichlib64 "$BATS_TEST_TMPDIR/foo_norpath" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
}

@test "no_soversion_check resolves symlink when rpath finds unversioned name" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so.1" "$BATS_TEST_DIRNAME/bar.c"
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  build_foo_with_rpath
  run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [[ "$stderr" == *"Warning: no soversion specified for libbar.so but found versioned libbar.so.1"* ]]
}

@test "version_check resolves symlink when rpath finds unversioned name" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so.1" "$BATS_TEST_DIRNAME/bar.c"
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  build_foo_with_rpath
  run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so:2
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [[ "$stderr" == *"Warning: expected libbar.so.2 but found libbar.so.1"* ]]
}

# bats test_tags=need_patchelf
@test "dlopen preserves SONAME filename when actual file has deeper version" {
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/libbar.so.1.2" "$BATS_TEST_DIRNAME/bar.c"
  ln -s libbar.so.1.2 "$BATS_TEST_TMPDIR/libbar.so.1"
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  build_foo_no_rpath
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so.1" ]
}

@test "find_lib resolves bare binary name via PATH" {
  mkdir -p "$BATS_TEST_TMPDIR/bin" "$BATS_TEST_TMPDIR/lib"
  build_libbar "$BATS_TEST_TMPDIR/lib"
  gcc -o "$BATS_TEST_TMPDIR/bin/foo" "$BATS_TEST_DIRNAME/foo.c" \
    -L"$BATS_TEST_TMPDIR/lib" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR/lib"
  PATH="/nonexistent:$BATS_TEST_TMPDIR/bin:$PATH" run -0 whichlib64 foo libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/lib/libbar.so" ]
}

# bats test_tags=need_patchelf
@test "dlopen preserves symlinked directory in path (no rpath)" {
  mkdir -p "$BATS_TEST_TMPDIR/real_dir"
  build_libbar_versioned "$BATS_TEST_TMPDIR/real_dir"
  ln -s real_dir "$BATS_TEST_TMPDIR/link_dir"
  gcc -o "$BATS_TEST_TMPDIR/foo" "$BATS_TEST_DIRNAME/foo.c" -L"$BATS_TEST_TMPDIR/link_dir" -lbar
  patchelf --remove-rpath "$BATS_TEST_TMPDIR/foo"
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR/link_dir" run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/link_dir/libbar.so.1" ]
}

# bats test_tags=need_patchelf
@test "cache lookup matches ldconfig for system library" {
  build_noop
  patchelf --remove-rpath "$BATS_TEST_TMPDIR/noop"
  local expected
  expected=$(ldconfig -p | awk '/libc\.so\.6.*x86-64/{print $NF; exit}')
  [ -n "$expected" ]
  LD_LIBRARY_PATH="" run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/noop" libc.so
  [ "$output" = "$expected" ]
}

@test "find_lib with bare name not in PATH falls back to dlopen" {
  build_libbar
  build_noop
  PATH="/nonexistent:$PATH" LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -0 whichlib64 noop libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
}

@test "find_lib resolves library via DT_RPATH (--disable-new-dtags)" {
  build_libbar
  gcc -o "$BATS_TEST_TMPDIR/foo" "$BATS_TEST_DIRNAME/foo.c" -L"$BATS_TEST_TMPDIR" -lbar \
    -Wl,-rpath,"$BATS_TEST_TMPDIR" -Wl,--disable-new-dtags
  run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ -z "$stderr" ]
}

@test "find_lib finds library in second entry of colon-separated LD_LIBRARY_PATH" {
  build_libbar
  build_noop
  LD_LIBRARY_PATH="/nonexistent:$BATS_TEST_TMPDIR" run -0 whichlib64 "$BATS_TEST_TMPDIR/noop" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
}

# bats test_tags=need_patchelf
@test "search_default_paths reached with versioned DT_NEEDED, no rpath, no LD_LIBRARY_PATH" {
  build_libbar_versioned
  build_foo_no_rpath
  LD_LIBRARY_PATH="" run -1 whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "libbar.so not found" ]
}

# ── Multiple libraries ────────────────────────────────────────────────

@test "multiple libs: one line per lib, mixed found and not-found" {
  build_libbar
  build_noop
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -1 whichlib64 "$BATS_TEST_TMPDIR/noop" libbar.so libnothere.so
  local line1 line2
  line1=$(echo "$output" | sed -n '1p')
  line2=$(echo "$output" | sed -n '2p')
  [ "$line1" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ "$line2" = "libnothere.so not found" ]
}

@test "multiple libs: all found" {
  build_libbar
  gcc -shared -o "$BATS_TEST_TMPDIR/libbaz.so" "$BATS_TEST_DIRNAME/bar.c"
  build_foo_with_rpath
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -0 whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so libbaz.so
  local line1 line2
  line1=$(echo "$output" | sed -n '1p')
  line2=$(echo "$output" | sed -n '2p')
  [ "$line1" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ "$line2" = "$BATS_TEST_TMPDIR/libbaz.so" ]
}

# ── ldconfig conf paths ──────────────────────────────────────────────

@test "find_lib locates library via ldconfig conf paths" {
  mkdir -p "$BATS_TEST_TMPDIR/libdir"
  build_libbar "$BATS_TEST_TMPDIR/libdir"
  setup_ldconf "$BATS_TEST_TMPDIR/libdir"
  build_noop
  WHICHLIB64_CONF="$BATS_TEST_TMPDIR/ld.so.conf" LD_LIBRARY_PATH="" run -0 whichlib64 "$BATS_TEST_TMPDIR/noop" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libdir/libbar.so" ]
}

@test "find_lib locates versioned-only library via glob fallback (no binary)" {
  mkdir -p "$BATS_TEST_TMPDIR/libdir"
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/libdir/libbar.so.1" "$BATS_TEST_DIRNAME/bar.c"
  setup_ldconf "$BATS_TEST_TMPDIR/libdir"
  WHICHLIB64_CONF="$BATS_TEST_TMPDIR/ld.so.conf" LD_LIBRARY_PATH="" run -2 --separate-stderr whichlib64 "" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libdir/libbar.so.1" ]
}

# bats test_tags=need_patchelf
@test "find_lib locates versioned library via ldconfig conf paths with DT_NEEDED" {
  mkdir -p "$BATS_TEST_TMPDIR/libdir"
  build_libbar_versioned "$BATS_TEST_TMPDIR/libdir"
  gcc -o "$BATS_TEST_TMPDIR/foo" "$BATS_TEST_DIRNAME/foo.c" -L"$BATS_TEST_TMPDIR/libdir" -lbar
  patchelf --remove-rpath "$BATS_TEST_TMPDIR/foo"
  rm "$BATS_TEST_TMPDIR/libdir/libbar.so"
  setup_ldconf "$BATS_TEST_TMPDIR/libdir"
  WHICHLIB64_CONF="$BATS_TEST_TMPDIR/ld.so.conf" LD_LIBRARY_PATH="" run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libdir/libbar.so.1" ]
}
