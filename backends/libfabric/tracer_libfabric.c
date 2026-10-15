#define _GNU_SOURCE

#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>

#include <rdma/fabric.h>
#include <rdma/fi_errno.h>

#include "libfabric_tracepoints.h"

typedef int (*fi_fabric_fn)(
    struct fi_fabric_attr *,
    struct fid_fabric **,
    void *
);

static fi_fabric_fn original_fi_fabric;
static pthread_once_t resolve_once = PTHREAD_ONCE_INIT;

static void resolve_fi_fabric(void)
{
    original_fi_fabric =
        (fi_fabric_fn)dlsym(RTLD_NEXT, "fi_fabric");
}

int fi_fabric(struct fi_fabric_attr *attr,
              struct fid_fabric **fabric,
              void *context)
{
    int incoming_errno = errno;

    pthread_once(&resolve_once, resolve_fi_fabric);

    tracepoint(lttng_ust_libfabric, fi_fabric_entry,
               attr, fabric, context);

    errno = incoming_errno;

    int result = original_fi_fabric
        ? original_fi_fabric(attr, fabric, context)
        : -FI_ENOSYS;

    int provider_errno = errno;

    struct fid_fabric *created = NULL;
    if (result == 0 && fabric != NULL)
        created = *fabric;

    tracepoint(lttng_ust_libfabric, fi_fabric_exit,
               result, created);

    errno = provider_errno;
    return result;
}