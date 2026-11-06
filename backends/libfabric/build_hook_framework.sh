mkdir -p build

cc -D_GNU_SOURCE \
   -fPIC -shared -pthread \
   -fvisibility=hidden \
   -I. \
   -Iupstream/libfabric/include \
   -Iupstream/libfabric/include/linux \
   -Iupstream/libfabric/prov/hook/include \
   hook_framework.c \
   upstream/libfabric/prov/hook/src/hook*.c \
   upstream/libfabric/src/enosys.c \
   -Wl,-z,defs \
   -o build/libthapi-fi.so \
   $(pkg-config --libs libfabric)