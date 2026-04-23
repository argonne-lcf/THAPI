bats_require_minimum_version 1.5.0

setup_file() {
  [ -x whichlib64 ] || make -s
}

# ── Usage ─────────────────────────────────────────────────────────────

@test "usage: no arguments prints help and exits 1" {
  run -1 --separate-stderr whichlib64
  [[ "$stderr" == *"Usage:"* ]]
}

# ── Matrix: no soversion ──────────────────────────────────────────────

@test "row 1: no sover, no DT_NEEDED, not findable → not found (exit 1)" {
  echo 'int main(void){return 0;}' | gcc -x c -o "$BATS_TEST_TMPDIR/noop" -
  run -1 whichlib64 "$BATS_TEST_TMPDIR/noop" libdoesnotexist.so
  [ "$output" = "libdoesnotexist.so not found" ]
}

@test "row 2: no sover, no DT_NEEDED, unversioned on disk → found (exit 0)" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  echo 'int main(void){return 0;}' | gcc -x c -o "$BATS_TEST_TMPDIR/noop" -
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/noop" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ -z "$stderr" ]
}

@test "row 4: no sover, DT_NEEDED unversioned → found (exit 0)" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR"
  run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ -z "$stderr" ]
}

@test "row 5: no sover, DT_NEEDED versioned → warning (exit 2)" {
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/libbar.so.1" bar.c
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR"
  run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so.1" ]
  [[ "$stderr" == *"Warning: no soversion specified for libbar.so but found versioned libbar.so.1"* ]]
}

# ── Matrix: with soversion ────────────────────────────────────────────

@test "row 6: sover 1, no DT_NEEDED, not findable → not found (exit 1)" {
  echo 'int main(void){return 0;}' | gcc -x c -o "$BATS_TEST_TMPDIR/noop" -
  run -1 whichlib64 "$BATS_TEST_TMPDIR/noop" libdoesnotexist.so:1
  [ "$output" = "libdoesnotexist.so not found" ]
}

@test "row 7: sover 1, no DT_NEEDED, unversioned on disk → found (exit 0)" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  echo 'int main(void){return 0;}' | gcc -x c -o "$BATS_TEST_TMPDIR/noop" -
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/noop" libbar.so:1
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ -z "$stderr" ]
}

@test "row 9: sover 1, DT_NEEDED unversioned → found (exit 0)" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR"
  run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so:1
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ -z "$stderr" ]
}

@test "row 10: sover 1, DT_NEEDED versioned matching → found (exit 0)" {
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/libbar.so.1" bar.c
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR"
  run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so:1
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so.1" ]
  [ -z "$stderr" ]
}

@test "row 11: sover 2, DT_NEEDED versioned mismatch → warning (exit 2)" {
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/libbar.so.1" bar.c
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR"
  run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so:2
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so.1" ]
  [[ "$stderr" == *"Warning: expected libbar.so.2 but found libbar.so.1"* ]]
}

# ── Edge cases ────────────────────────────────────────────────────────

@test "find_lib locates a library through a symlink" {
  gcc -shared -o "$BATS_TEST_TMPDIR/lib_foo.so" bar.c
  ln -s lib_foo.so "$BATS_TEST_TMPDIR/lib_foo_symlink.so"
  gcc -o "$BATS_TEST_TMPDIR/foo_symlink" foo.c \
    -L"$BATS_TEST_TMPDIR" -l:lib_foo_symlink.so -Wl,-rpath,"$BATS_TEST_TMPDIR"
  run -0 whichlib64 "$BATS_TEST_TMPDIR/foo_symlink" lib_foo.so
  [ "$output" = "$BATS_TEST_TMPDIR/lib_foo.so" ]
}

@test "find_lib skips a 32-bit library" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR"
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
  gcc -shared -o "$BATS_TEST_TMPDIR/lib/libbar.so" bar.c
  gcc -o "$BATS_TEST_TMPDIR/bin/foo" foo.c \
    -L"$BATS_TEST_TMPDIR/lib" -lbar -Wl,-rpath,'$ORIGIN/../lib'
  run -0 whichlib64 "$BATS_TEST_TMPDIR/bin/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/bin/../lib/libbar.so" ]
}

@test "find_lib prefers rpath over LD_LIBRARY_PATH" {
  mkdir -p "$BATS_TEST_TMPDIR/rpath_dir" "$BATS_TEST_TMPDIR/ldpath_dir"
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/rpath_dir/libbar.so.1" bar.c
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/ldpath_dir/libbar.so.1" bar.c
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/rpath_dir/libbar.so"
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/ldpath_dir/libbar.so"
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c \
    -L"$BATS_TEST_TMPDIR/rpath_dir" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR/rpath_dir"
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR/ldpath_dir" \
    run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/rpath_dir/libbar.so.1" ]
}

@test "find_lib resolves DT_NEEDED soname via dlopen when no rpath" {
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/libbar.so.1" bar.c
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar
  patchelf --remove-rpath "$BATS_TEST_TMPDIR/foo"
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so.1" ]
}

@test "find_lib locates a library via LD_LIBRARY_PATH (no rpath)" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  gcc -o "$BATS_TEST_TMPDIR/foo_norpath" foo.c -L"$BATS_TEST_TMPDIR" -lbar
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -0 whichlib64 "$BATS_TEST_TMPDIR/foo_norpath" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
}

@test "no_soversion_check resolves symlink when rpath finds unversioned name" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so.1" bar.c
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR"
  run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [[ "$stderr" == *"Warning: no soversion specified for libbar.so but found versioned libbar.so.1"* ]]
}

@test "version_check resolves symlink when rpath finds unversioned name" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so.1" bar.c
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR"
  run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so:2
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [[ "$stderr" == *"Warning: expected libbar.so.2 but found libbar.so.1"* ]]
}

@test "dlopen preserves SONAME filename when actual file has deeper version" {
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/libbar.so.1.2" bar.c
  ln -s libbar.so.1.2 "$BATS_TEST_TMPDIR/libbar.so.1"
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar
  patchelf --remove-rpath "$BATS_TEST_TMPDIR/foo"
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so.1" ]
}

@test "find_lib resolves bare binary name via PATH" {
  mkdir -p "$BATS_TEST_TMPDIR/bin" "$BATS_TEST_TMPDIR/lib"
  gcc -shared -o "$BATS_TEST_TMPDIR/lib/libbar.so" bar.c
  gcc -o "$BATS_TEST_TMPDIR/bin/foo" foo.c \
    -L"$BATS_TEST_TMPDIR/lib" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR/lib"
  PATH="/nonexistent:$BATS_TEST_TMPDIR/bin:$PATH" run -0 whichlib64 foo libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/lib/libbar.so" ]
}

@test "dlopen preserves symlinked directory in path (no rpath)" {
  mkdir -p "$BATS_TEST_TMPDIR/real_dir"
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/real_dir/libbar.so.1" bar.c
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/real_dir/libbar.so"
  ln -s real_dir "$BATS_TEST_TMPDIR/link_dir"
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR/link_dir" -lbar
  patchelf --remove-rpath "$BATS_TEST_TMPDIR/foo"
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR/link_dir" run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/link_dir/libbar.so.1" ]
}

@test "cache lookup matches ldconfig for system library" {
  echo 'int main(void){return 0;}' | gcc -x c -o "$BATS_TEST_TMPDIR/noop" -
  patchelf --remove-rpath "$BATS_TEST_TMPDIR/noop"
  local expected
  expected=$(ldconfig -p | awk '/libc\.so\.6.*x86-64/{print $NF; exit}')
  [ -n "$expected" ]
  LD_LIBRARY_PATH="" run -2 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/noop" libc.so
  [ "$output" = "$expected" ]
}

@test "find_lib with bare name not in PATH falls back to dlopen" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  echo 'int main(void){return 0;}' | gcc -x c -o "$BATS_TEST_TMPDIR/noop" -
  PATH="/nonexistent" LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -0 whichlib64 noop libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
}

@test "find_lib resolves library via DT_RPATH (--disable-new-dtags)" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar \
    -Wl,-rpath,"$BATS_TEST_TMPDIR" -Wl,--disable-new-dtags
  run -0 --separate-stderr whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ -z "$stderr" ]
}

@test "find_lib finds library in second entry of colon-separated LD_LIBRARY_PATH" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  echo 'int main(void){return 0;}' | gcc -x c -o "$BATS_TEST_TMPDIR/noop" -
  LD_LIBRARY_PATH="/nonexistent:$BATS_TEST_TMPDIR" run -0 whichlib64 "$BATS_TEST_TMPDIR/noop" libbar.so
  [ "$output" = "$BATS_TEST_TMPDIR/libbar.so" ]
}

@test "search_default_paths reached with versioned DT_NEEDED, no rpath, no LD_LIBRARY_PATH" {
  gcc -shared -Wl,-soname,libbar.so.1 -o "$BATS_TEST_TMPDIR/libbar.so.1" bar.c
  ln -s libbar.so.1 "$BATS_TEST_TMPDIR/libbar.so"
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar
  patchelf --remove-rpath "$BATS_TEST_TMPDIR/foo"
  LD_LIBRARY_PATH="" run -1 whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so
  [ "$output" = "libbar.so not found" ]
}

# ── Multiple libraries ────────────────────────────────────────────────

@test "multiple libs: one line per lib, mixed found and not-found" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  echo 'int main(void){return 0;}' | gcc -x c -o "$BATS_TEST_TMPDIR/noop" -
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -1 whichlib64 "$BATS_TEST_TMPDIR/noop" libbar.so libnothere.so
  local line1 line2
  line1=$(echo "$output" | sed -n '1p')
  line2=$(echo "$output" | sed -n '2p')
  [ "$line1" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ "$line2" = "libnothere.so not found" ]
}

@test "multiple libs: all found" {
  gcc -shared -o "$BATS_TEST_TMPDIR/libbar.so" bar.c
  gcc -shared -o "$BATS_TEST_TMPDIR/libbaz.so" bar.c
  gcc -o "$BATS_TEST_TMPDIR/foo" foo.c -L"$BATS_TEST_TMPDIR" -lbar -Wl,-rpath,"$BATS_TEST_TMPDIR"
  LD_LIBRARY_PATH="$BATS_TEST_TMPDIR" run -0 whichlib64 "$BATS_TEST_TMPDIR/foo" libbar.so libbaz.so
  local line1 line2
  line1=$(echo "$output" | sed -n '1p')
  line2=$(echo "$output" | sed -n '2p')
  [ "$line1" = "$BATS_TEST_TMPDIR/libbar.so" ]
  [ "$line2" = "$BATS_TEST_TMPDIR/libbaz.so" ]
}
