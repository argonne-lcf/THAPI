#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([thapi], [0.0.6], [bvideau@anl.gov])
AC_CONFIG_SRCDIR([opencl/tracer_opencl_helpers.include.c])
AC_CONFIG_HEADERS([utils/config.h])

# build artefacts in separate dir
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_MACRO_DIR([m4])

# automake should fail on any error
AM_INIT_AUTOMAKE([-Wall -Werror foreign 1.12])
AM_PROG_AR

# check for libtool
LT_INIT([disable-static])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
if test -z "$RUBY"; then
  AC_CHECK_PROG(RUBY,ruby,ruby)
fi
if test -z "$ERB"; then
  AC_CHECK_PROG(ERB,erb,erb)
fi
if test -z "$LTTNG_GEN_TP"; then
  AC_CHECK_PROG(LTTNG_GEN_TP,lttng-gen-tp,lttng-gen-tp)
fi
if test -z "$BABELTRACE2_PROG"; then
  AC_CHECK_PROG(BABELTRACE2_PROG,babeltrace2,babeltrace2)
fi
if test -z "$PATCH"; then
  AC_CHECK_PROG(PATCH,patch,patch)
fi

# Checks for libraries.
AC_CHECK_LIB([dl], [dlopen])

PKG_CHECK_MODULES([LIBFFI], [libffi >= 3.2])
PKG_CHECK_MODULES([BABELTRACE2], [babeltrace2 >= 2.0])
PKG_CHECK_MODULES([LTTNG_UST], [lttng-ust >= 2.10])

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
AC_CHECK_FUNCS([atexit clock_gettime ftruncate memmove memset strdup strstr strtoull])

AC_CONFIG_FILES([Makefile
                 utils/Makefile
                 opencl/Makefile
                 ze/Makefile])
#                 cuda/Makefile])
AC_CONFIG_FILES([opencl/tracer_opencl.sh], [chmod +x opencl/tracer_opencl.sh])
AC_CONFIG_FILES([opencl/clprof.sh], [chmod +x opencl/clprof.sh])
AC_CONFIG_FILES([opencl/babeltrace_opencl], [chmod +x opencl/babeltrace_opencl])
AC_CONFIG_FILES([ze/babeltrace_ze], [chmod +x ze/babeltrace_ze])
AC_CONFIG_FILES([ze/tracer_ze.sh], [chmod +x ze/tracer_ze.sh])
AC_CONFIG_FILES([ze/zeprof.sh], [chmod +x ze/zeprof.sh])
AC_OUTPUT
