#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "ofi_hook.h"

static struct hook_prov_ctx thapi_ctx;
static struct fi_ops_msg thapi_msg_ops;
static pthread_once_t thapi_msg_once = PTHREAD_ONCE_INIT;

static ssize_t thapi_send(struct fid_ep *ep, const void *buf, size_t len,
                         void *desc, fi_addr_t dest_addr, void *context){
    struct hook_ep *endpoint = container_of(ep, struct hook_ep, ep);
    fprintf(stderr,
            "[THAPI libfabric] fi_send entry ep=%p buf=%p len=%zu "
            "desc=%p dest=%llu context=%p\n",
            (void *)ep, (void *)buf, len, desc,
            (unsigned long long)dest_addr, context);

    ssize_t ret = fi_send(endpoint->hep, buf, len, desc, dest_addr, context);

    fprintf(stderr, "[THAPI libfabric] fi_send exit ep=%p ret=%zd\n",
            (void *)ep, ret);
    return ret;
}

static void thapi_init_message_table(void){
    //Preserve recv, sendv, sendmsg, etc.; replace only send.
    thapi_msg_ops = hook_msg_ops;
    thapi_msg_ops.send = thapi_send;
}

static int thapi_endpoint_init(struct fid *fid){
    struct fid_ep *ep = container_of(fid, struct fid_ep, fid);
    int ret = pthread_once(&thapi_msg_once, thapi_init_message_table);
    if (ret)
        return -FI_EOTHER;
    ep->msg = &thapi_msg_ops;
    return 0;
}

static int thapi_fabric(struct fi_fabric_attr *attr,
                        struct fid_fabric **fabric, void *context){
    if (!attr || !attr->fabric || !fabric)
        return -FI_EINVAL;
    struct hook_fabric *wrapper = calloc(1, sizeof(*wrapper));
    if (!wrapper)
        return -FI_ENOMEM;

    hook_fabric_init(wrapper, HOOK_TRACE, attr->fabric,
                     (struct fi_provider *)context,
                     &hook_fabric_fid_ops, &thapi_ctx);
    *fabric = &wrapper->fabric;
    return 0;
}

static struct hook_prov_ctx thapi_ctx = {
    .prov = {
        .version = FI_VERSION(1, 0),
        .fi_version = OFI_VERSION_LATEST,
        .name = "ofi_hook_thapi",
        .getinfo = NULL,
        .fabric = thapi_fabric,
    },
    .ini_fid = {
        [FI_CLASS_EP] = thapi_endpoint_init,
    },
};

FI_EXT_INI
{
    return &thapi_ctx.prov;
}
