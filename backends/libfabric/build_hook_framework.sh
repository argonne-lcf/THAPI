mkdir -p build

# cc -D_GNU_SOURCE \
#    -fPIC -shared -pthread \
#    -fvisibility=hidden \
#    -I. \
#    -Iupstream/libfabric/include \
#    -Iupstream/libfabric/include/linux \
#    -Iupstream/libfabric/prov/hook/include \
#    hook_framework.c \
#    upstream/libfabric/prov/hook/src/hook*.c \
#    upstream/libfabric/src/enosys.c \
#    -Wl,-z,defs \
#    -o build/libthapi-fi.so \
#    $(pkg-config --libs libfabric)

cc -D_GNU_SOURCE \
   -DHAVE_CLOCK_GETTIME=1 \
   -fPIC -shared -pthread \
   -fvisibility=hidden \
   -I. \
   -Ibuild \
   -Iupstream/libfabric \
   -Iupstream/libfabric/include \
   -Iupstream/libfabric/include/linux \
   -Iupstream/libfabric/prov/hook/include \
   $(pkg-config --cflags lttng-ust) \
   hook_framework.c \
   build/libfabric_tracepoints.c \
   upstream/libfabric/prov/hook/src/hook*.c \
   upstream/libfabric/src/enosys.c \
   -Wl,-z,defs \
   -o build/libthapi-fi.so \
   $(pkg-config --libs libfabric lttng-ust)

echo "Build exit status: $?"