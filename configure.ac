#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([thapi], [0.0.11], [bvideau@anl.gov])
AC_CONFIG_SRCDIR([opencl/tracer_opencl_helpers.include.c])
AC_CONFIG_HEADERS([utils/config.h])

# build artefacts in separate dir
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_MACRO_DIR([m4])

# automake should fail on any error
AM_INIT_AUTOMAKE([-Wall -Werror foreign 1.12 subdir-objects])
AM_PROG_AR

# check for libtool
LT_INIT([disable-static])

# Checks for programs.
AC_PROG_LN
AC_PROG_LN_S
AC_PROG_MKDIR_P
AC_PROG_CXX
AC_PROG_CC
if test -z "$RUBY"; then
  AC_CHECK_PROG(RUBY,[ruby],[ruby],[no])
  test "$RUBY" == "no" && AC_MSG_ERROR([Required program 'ruby' not found.])
fi
if test -z "$ERB"; then
  AC_CHECK_PROG(ERB,[erb],[erb],[no])
  test "$ERB" == "no" && AC_MSG_ERROR([Required program 'erb' not found.])
fi
if test -z "$METABABEL"; then
  AC_CHECK_PROG(METABABEL,[metababel],[metababel],[no])
  test "$METABABEL" == "no" && AC_MSG_ERROR([Required program 'metababel' not found.])
fi
if test -z "$LTTNG_GEN_TP"; then
  AC_CHECK_PROG(LTTNG_GEN_TP,[lttng-gen-tp],[lttng-gen-tp],[no])
  test "$LTTNG_GEN_TP" == "no" && AC_MSG_ERROR([Required program 'lttng-gen-tp' not found.])
fi
if test -z "$BABELTRACE2_PROG"; then
  AC_CHECK_PROG(BABELTRACE2_PROG,[babeltrace2],[babeltrace2],[no])
  test "$BABELTRACE2_PROG" == "no" && AC_MSG_ERROR([Required program 'babeltrace2' not found.])
fi
if test -z "$PATCH"; then
  AC_CHECK_PROG(PATCH,[patch],[patch],[no])
  test "$PATCH" == "no" && AC_MSG_ERROR([Required program 'patch' not found.])
fi
if test -z "$PROTOC"; then
  AC_CHECK_PROG(PROTOC,[protoc],[protoc],[no])
  test "$PROTOC" == "no" && AC_MSG_ERROR([Required program 'protoc' not found.])
fi

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([iberty], [cplus_demangle])
AC_CHECK_HEADERS([demangle.h], [using_demangle=yes], [using_demangle=no])
AS_IF([test "x$using_demangle" != xyes],
  [AC_CHECK_HEADERS([libiberty/demangle.h], [], [AC_MSG_WARN([Couldn't find demangle.h, demangling will be deactivated...])])])

AC_ARG_ENABLE([strict], AS_HELP_STRING([--enable-strict], [Enable -Werror]), [], [enable_strict=yes])
AM_CONDITIONAL([STRICT], [test "x$enable_strict" = xyes])

AC_ARG_ENABLE([mpi],
  AS_HELP_STRING([--disable-mpi], [Disable mpi sync daemon]), [found_mpi=yes], [found_mpi=no])
AS_IF([test "x$enable_mpi" != xno], [
  AX_MPI([found_mpi=yes], [found_mpi=no])
])
AM_CONDITIONAL([FOUND_MPI], [test "x$found_mpi" = xyes])

PKG_CHECK_MODULES([LIBFFI], [libffi >= 3.2])
PKG_CHECK_MODULES([BABELTRACE2], [babeltrace2 >= 2.0])
PKG_CHECK_MODULES([LTTNG_UST], [lttng-ust >= 2.10])
PKG_CHECK_MODULES([LTTNG_CTL], [lttng-ctl >= 2.13.13])
PKG_CHECK_MODULES([PROTOBUF], [protobuf >= 3.0])

AX_RUBY_EXTENSION([cast-to-yaml], [yes])
AX_RUBY_EXTENSION([nokogiri], [yes])
AX_RUBY_EXTENSION([babeltrace2], [yes])
AX_RUBY_EXTENSION([metababel >= 1.1.1], [yes])

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h stddef.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime ftruncate memmove memset strdup strstr strtoull strlen strchr])

AC_CONFIG_FILES([
	Makefile
	sampling/Makefile
	opencl/Makefile
	ze/Makefile
	cuda/Makefile
	omp/Makefile
	hip/Makefile
	xprof/Makefile
	utils/Makefile
  libthapictl/Makefile
])
AC_CONFIG_FILES([utils/test_wrapper_thapi_text_pretty.sh], [chmod +x utils/test_wrapper_thapi_text_pretty.sh])
AC_CONFIG_FILES([opencl/tracer_opencl.sh], [chmod +x opencl/tracer_opencl.sh])
AC_CONFIG_FILES([opencl/extract_enqueues], [chmod +x opencl/extract_enqueues])
AC_CONFIG_FILES([ze/tracer_ze.sh], [chmod +x ze/tracer_ze.sh])
AC_CONFIG_FILES([cuda/tracer_cuda.sh], [chmod +x cuda/tracer_cuda.sh])
AC_CONFIG_FILES([omp/tracer_omp.sh], [chmod +x omp/tracer_omp.sh])
AC_CONFIG_FILES([hip/tracer_hip.sh], [chmod +x hip/tracer_hip.sh])
AC_CONFIG_FILES([utils/babeltrace_thapi], [chmod +x utils/babeltrace_thapi])
AC_CONFIG_FILES([xprof/xprof.rb], [chmod +x xprof/xprof.rb])
AC_OUTPUT
