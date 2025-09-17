ACLOCAL_AMFLAGS = -I m4
SUBDIRS = utils xprof sampling backends/opencl backends/ze backends/cuda backends/omp backends/hip backends/mpi backends/cxi

if ENABLE_CLANG_PARSER
SUBDIRS += backends/itt
endif

EXTRA_DIST = autogen.sh README.md .valgrind/dlopen.supp
