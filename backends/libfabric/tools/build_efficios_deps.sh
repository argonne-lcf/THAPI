#!/usr/bin/env bash
#
# Build the LTTng/babeltrace stack THAPI needs, into one prefix.
#
#     bash backends/libfabric/tools/build_efficios_deps.sh
#     source ~/efficios_dep/env.sh      # then re-run configure
#
# Transcribed from .github/actions/setup-thapi/action.yml, which is the
# authoritative recipe -- the README is not.  Four packages, in dependency
# order, and the exact refs matter:
#
#   userspace-rcu  v0.15.3            upstream   (lttng-ust needs it)
#   lttng-ust      v2.16.0            upstream   (configure.ac demands >= 2.16)
#   lttng-tools    anl-v2.16.0        ANL FORK
#   babeltrace     anl-ms3-v2.1.2     ANL FORK
#
# Two of those are forks and two are not, which is easy to get backwards.
# lttng-ust is plain upstream at a tag newer than most distros ship; the
# distro 2.13/2.14 is simply too old.  lttng-tools and babeltrace carry ANL
# patches THAPI depends on, and the upstream versions of those build fine and
# then misbehave at run time.
#
# Environment:
#   PREFIX   install prefix          (default ~/efficios_dep)
#   JOBS     parallelism             (default nproc, capped at 16)
#   CC/CXX   compilers               (default gcc/g++ -- see below)
#
# On Aurora the default compiler is icx.  These four packages are ordinary
# autotools C projects that expect GCC, and icx's stricter defaults make them
# fail in uninteresting ways, so this script pins gcc unless you override it.
# That is fine: they are plain shared libraries and THAPI links them through
# pkg-config, so the compiler used to build them need not match the compiler
# used to build THAPI.

set -euo pipefail

PREFIX=${PREFIX:-$HOME/efficios_dep}
JOBS=${JOBS:-$(n=$(nproc 2>/dev/null || echo 4); [ "$n" -gt 16 ] && echo 16 || echo "$n")}
CC=${CC:-gcc}
CXX=${CXX:-g++}
WORK=${WORK:-$PREFIX/src}

export CC CXX
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
export LD_LIBRARY_PATH="$PREFIX/lib:${LD_LIBRARY_PATH:-}"
export PATH="$PREFIX/bin:$PATH"

mkdir -p "$WORK"

say() { printf "\n\033[1m==> %s\033[0m\n" "$*"; }

# fetch <dir> <url> <ref>
fetch() {
  if [ -d "$WORK/$1/.git" ]; then
    say "$1: already cloned"
  else
    say "$1: cloning $3"
    git clone --depth 1 -b "$3" "$2" "$WORK/$1"
  fi
}

# build <dir> [extra configure args...]
build() {
  local d=$1; shift
  say "$d: building"
  cd "$WORK/$d"
  [ -x ./configure ] || ./bootstrap
  ./configure --prefix="$PREFIX" "$@"
  make -j"$JOBS"
  make install
}

fetch userspace-rcu https://github.com/urcu/userspace-rcu          v0.15.3
fetch lttng-ust     https://github.com/lttng/lttng-ust             v2.16.0
fetch lttng-tools   https://github.com/argonne-lcf/lttng-tools.git anl-v2.16.0
fetch babeltrace    https://github.com/argonne-lcf/babeltrace.git  anl-ms3-v2.1.2

build userspace-rcu
build lttng-ust   --disable-man-pages
build lttng-tools --disable-man-pages --disable-bin-lttng-crash
build babeltrace  --disable-man-pages

cat > "$PREFIX/env.sh" <<EOF
# source this before configuring THAPI
export PATH="$PREFIX/bin:\$PATH"
export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:\${PKG_CONFIG_PATH:-}"
export CPATH="$PREFIX/include:\${CPATH:-}"
export LIBRARY_PATH="$PREFIX/lib:\${LIBRARY_PATH:-}"
export LD_LIBRARY_PATH="$PREFIX/lib:\${LD_LIBRARY_PATH:-}"
# user-installed ruby gems: a module/spack ruby sets GEM_PATH explicitly and
# drops ~/.local/share/gem, so --user-install gems become unrequirable.
export GEM_PATH="\$(ruby -e 'print Gem.user_dir'):\${GEM_PATH:-}"
export PATH="\$(ruby -e 'print Gem.user_dir')/bin:\$PATH"
EOF

say "done"
cat <<EOF

Installed into $PREFIX

  source $PREFIX/env.sh

Then verify you are picking up the new stack, not the system one:

  pkg-config --modversion lttng-ust      # want >= 2.16
  pkg-config --modversion babeltrace2    # want 2.1.2 from $PREFIX
  which lttng babeltrace2                # want $PREFIX/bin/...

and re-run configure from a CLEAN build directory -- configure caches the
failed lttng-ust probe, so reusing the old one will keep reporting 2.14.0:

  rm -rf build && mkdir build && cd build
  ../configure --prefix=\$(pwd)/ici
EOF
