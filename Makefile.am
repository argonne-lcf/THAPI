# Install the pkg-config file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = thapi.pc

ACLOCAL_AMFLAGS = -I m4
SUBDIRS = utils xprof sampling backends/opencl backends/ze backends/cuda backends/omp backends/hip backends/mpi backends/cxi backends/itt

EXTRA_DIST = autogen.sh README.md .valgrind/dlopen.supp thapi.pc.in

CLEANFILES = thapi.pc
