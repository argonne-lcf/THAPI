/*
 * fi_trace -- LD_PRELOAD interposition and vtable wrapping
 *
 * Why this is not a plain dlsym() shim
 * ------------------------------------
 * fi_send(), fi_recv(), fi_tsend(), fi_write() and friends are *static
 * inline* functions in <rdma/fi_endpoint.h> that dispatch straight
 * through the provider vtables (ep->msg->send, ep->tagged->recv, ...).
 * They are not exported symbols, so dlsym(RTLD_NEXT, "fi_send") cannot
 * find them and an interposed definition would never be called.
 *
 * The only data-path-reaching entry points libfabric actually exports
 * are fi_fabric() and fi_fabric2().  So we interpose those, and from
 * the fabric we walk down the object tree -- fabric -> domain ->
 * endpoint -- retargeting each object's vtable pointer at a private
 * copy whose data-path slots are our wrappers.  The provider's own
 * objects are never replaced, only the pointers the application
 * dispatches through, so provider-internal container_of() stays valid.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "fi_trace.h"

#include <dlfcn.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static atomic_uint fit_ep_counter;

static void fit_cq_attach_ep(struct fid *bfid, struct fit_ep *h);

/* ------------------------------------------------------------------ */
/* recovering hook state from a vtable pointer                        */
/* ------------------------------------------------------------------ */

#define EP_FROM_MSG(ep) fit_container_of((ep)->msg, struct fit_ep, msg)
#define EP_FROM_TAGGED(ep) fit_container_of((ep)->tagged, struct fit_ep, tagged)
#define EP_FROM_RMA(ep) fit_container_of((ep)->rma, struct fit_ep, rma)
#define EP_FROM_FID(fid) fit_container_of((fid)->ops, struct fit_ep, fid_ops)
#define EP_FROM_EPOPS(ep) fit_container_of((ep)->ops, struct fit_ep, ep_ops)

#define CALL_OR_ENOSYS(fn, args) ((fn) ? (fn)args : -FI_ENOSYS)

/* ================================================================== */
/* message operations                                                 */
/* ================================================================== */

static ssize_t fit_msg_send(struct fid_ep *ep,
                            const void *buf,
                            size_t len,
                            void *desc,
                            fi_addr_t dest_addr,
                            void *context) {
  struct fit_ep *h = EP_FROM_MSG(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_send", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_msg->send, (ep, buf, len, desc, dest_addr, context));
  fit_record(h, "fi_send", 'T', dest_addr, len, 1, ret, context);
  return ret;
}

static ssize_t fit_msg_sendv(struct fid_ep *ep,
                             const struct iovec *iov,
                             void **desc,
                             size_t count,
                             fi_addr_t dest_addr,
                             void *context) {
  struct fit_ep *h = EP_FROM_MSG(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_sendv", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_msg->sendv, (ep, iov, desc, count, dest_addr, context));
  fit_record(h, "fi_sendv", 'T', dest_addr, fit_iov_len(iov, count), count, ret, context);
  return ret;
}

static ssize_t fit_msg_sendmsg(struct fid_ep *ep, const struct fi_msg *msg, uint64_t flags) {
  struct fit_ep *h = EP_FROM_MSG(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_sendmsg", 'T', msg ? msg->context : NULL);
  ret = CALL_OR_ENOSYS(h->o_msg->sendmsg, (ep, msg, flags));
  fit_record(h, "fi_sendmsg", 'T', msg ? msg->addr : FI_ADDR_UNSPEC,
             msg ? fit_iov_len(msg->msg_iov, msg->iov_count) : 0, msg ? msg->iov_count : 0, ret,
             msg ? msg->context : NULL);
  return ret;
}

static ssize_t fit_msg_inject(struct fid_ep *ep, const void *buf, size_t len, fi_addr_t dest_addr) {
  struct fit_ep *h = EP_FROM_MSG(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_inject", 'T', NULL);
  ret = CALL_OR_ENOSYS(h->o_msg->inject, (ep, buf, len, dest_addr));
  fit_record(h, "fi_inject", 'T', dest_addr, len, 1, ret, NULL);
  return ret;
}

static ssize_t fit_msg_senddata(struct fid_ep *ep,
                                const void *buf,
                                size_t len,
                                void *desc,
                                uint64_t data,
                                fi_addr_t dest_addr,
                                void *context) {
  struct fit_ep *h = EP_FROM_MSG(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_senddata", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_msg->senddata, (ep, buf, len, desc, data, dest_addr, context));
  fit_record(h, "fi_senddata", 'T', dest_addr, len, 1, ret, context);
  return ret;
}

static ssize_t fit_msg_injectdata(
    struct fid_ep *ep, const void *buf, size_t len, uint64_t data, fi_addr_t dest_addr) {
  struct fit_ep *h = EP_FROM_MSG(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_injectdata", 'T', NULL);
  ret = CALL_OR_ENOSYS(h->o_msg->injectdata, (ep, buf, len, data, dest_addr));
  fit_record(h, "fi_injectdata", 'T', dest_addr, len, 1, ret, NULL);
  return ret;
}

static ssize_t fit_msg_recv(
    struct fid_ep *ep, void *buf, size_t len, void *desc, fi_addr_t src_addr, void *context) {
  struct fit_ep *h = EP_FROM_MSG(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_recv", 'R', context);
  ret = CALL_OR_ENOSYS(h->o_msg->recv, (ep, buf, len, desc, src_addr, context));
  fit_record(h, "fi_recv", 'R', src_addr, len, 1, ret, context);
  return ret;
}

static ssize_t fit_msg_recvv(struct fid_ep *ep,
                             const struct iovec *iov,
                             void **desc,
                             size_t count,
                             fi_addr_t src_addr,
                             void *context) {
  struct fit_ep *h = EP_FROM_MSG(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_recvv", 'R', context);
  ret = CALL_OR_ENOSYS(h->o_msg->recvv, (ep, iov, desc, count, src_addr, context));
  fit_record(h, "fi_recvv", 'R', src_addr, fit_iov_len(iov, count), count, ret, context);
  return ret;
}

static ssize_t fit_msg_recvmsg(struct fid_ep *ep, const struct fi_msg *msg, uint64_t flags) {
  struct fit_ep *h = EP_FROM_MSG(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_recvmsg", 'R', msg ? msg->context : NULL);
  ret = CALL_OR_ENOSYS(h->o_msg->recvmsg, (ep, msg, flags));
  fit_record(h, "fi_recvmsg", 'R', msg ? msg->addr : FI_ADDR_UNSPEC,
             msg ? fit_iov_len(msg->msg_iov, msg->iov_count) : 0, msg ? msg->iov_count : 0, ret,
             msg ? msg->context : NULL);
  return ret;
}

/* ================================================================== */
/* tagged operations                                                  */
/* ================================================================== */

static ssize_t fit_tag_send(struct fid_ep *ep,
                            const void *buf,
                            size_t len,
                            void *desc,
                            fi_addr_t dest_addr,
                            uint64_t tag,
                            void *context) {
  struct fit_ep *h = EP_FROM_TAGGED(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_tsend", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_tagged->send, (ep, buf, len, desc, dest_addr, tag, context));
  fit_record(h, "fi_tsend", 'T', dest_addr, len, 1, ret, context);
  return ret;
}

static ssize_t fit_tag_sendv(struct fid_ep *ep,
                             const struct iovec *iov,
                             void **desc,
                             size_t count,
                             fi_addr_t dest_addr,
                             uint64_t tag,
                             void *context) {
  struct fit_ep *h = EP_FROM_TAGGED(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_tsendv", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_tagged->sendv, (ep, iov, desc, count, dest_addr, tag, context));
  fit_record(h, "fi_tsendv", 'T', dest_addr, fit_iov_len(iov, count), count, ret, context);
  return ret;
}

static ssize_t fit_tag_sendmsg(struct fid_ep *ep, const struct fi_msg_tagged *msg, uint64_t flags) {
  struct fit_ep *h = EP_FROM_TAGGED(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_tsendmsg", 'T', msg ? msg->context : NULL);
  ret = CALL_OR_ENOSYS(h->o_tagged->sendmsg, (ep, msg, flags));
  fit_record(h, "fi_tsendmsg", 'T', msg ? msg->addr : FI_ADDR_UNSPEC,
             msg ? fit_iov_len(msg->msg_iov, msg->iov_count) : 0, msg ? msg->iov_count : 0, ret,
             msg ? msg->context : NULL);
  return ret;
}

static ssize_t
fit_tag_inject(struct fid_ep *ep, const void *buf, size_t len, fi_addr_t dest_addr, uint64_t tag) {
  struct fit_ep *h = EP_FROM_TAGGED(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_tinject", 'T', NULL);
  ret = CALL_OR_ENOSYS(h->o_tagged->inject, (ep, buf, len, dest_addr, tag));
  fit_record(h, "fi_tinject", 'T', dest_addr, len, 1, ret, NULL);
  return ret;
}

static ssize_t fit_tag_senddata(struct fid_ep *ep,
                                const void *buf,
                                size_t len,
                                void *desc,
                                uint64_t data,
                                fi_addr_t dest_addr,
                                uint64_t tag,
                                void *context) {
  struct fit_ep *h = EP_FROM_TAGGED(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_tsenddata", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_tagged->senddata, (ep, buf, len, desc, data, dest_addr, tag, context));
  fit_record(h, "fi_tsenddata", 'T', dest_addr, len, 1, ret, context);
  return ret;
}

static ssize_t fit_tag_injectdata(struct fid_ep *ep,
                                  const void *buf,
                                  size_t len,
                                  uint64_t data,
                                  fi_addr_t dest_addr,
                                  uint64_t tag) {
  struct fit_ep *h = EP_FROM_TAGGED(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_tinjectdata", 'T', NULL);
  ret = CALL_OR_ENOSYS(h->o_tagged->injectdata, (ep, buf, len, data, dest_addr, tag));
  fit_record(h, "fi_tinjectdata", 'T', dest_addr, len, 1, ret, NULL);
  return ret;
}

static ssize_t fit_tag_recv(struct fid_ep *ep,
                            void *buf,
                            size_t len,
                            void *desc,
                            fi_addr_t src_addr,
                            uint64_t tag,
                            uint64_t ignore,
                            void *context) {
  struct fit_ep *h = EP_FROM_TAGGED(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_trecv", 'R', context);
  ret = CALL_OR_ENOSYS(h->o_tagged->recv, (ep, buf, len, desc, src_addr, tag, ignore, context));
  fit_record(h, "fi_trecv", 'R', src_addr, len, 1, ret, context);
  return ret;
}

static ssize_t fit_tag_recvv(struct fid_ep *ep,
                             const struct iovec *iov,
                             void **desc,
                             size_t count,
                             fi_addr_t src_addr,
                             uint64_t tag,
                             uint64_t ignore,
                             void *context) {
  struct fit_ep *h = EP_FROM_TAGGED(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_trecvv", 'R', context);
  ret = CALL_OR_ENOSYS(h->o_tagged->recvv, (ep, iov, desc, count, src_addr, tag, ignore, context));
  fit_record(h, "fi_trecvv", 'R', src_addr, fit_iov_len(iov, count), count, ret, context);
  return ret;
}

static ssize_t fit_tag_recvmsg(struct fid_ep *ep, const struct fi_msg_tagged *msg, uint64_t flags) {
  struct fit_ep *h = EP_FROM_TAGGED(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_trecvmsg", 'R', msg ? msg->context : NULL);
  ret = CALL_OR_ENOSYS(h->o_tagged->recvmsg, (ep, msg, flags));
  fit_record(h, "fi_trecvmsg", 'R', msg ? msg->addr : FI_ADDR_UNSPEC,
             msg ? fit_iov_len(msg->msg_iov, msg->iov_count) : 0, msg ? msg->iov_count : 0, ret,
             msg ? msg->context : NULL);
  return ret;
}

/* ================================================================== */
/* RMA operations                                                     */
/* ================================================================== */

static ssize_t fit_rma_write(struct fid_ep *ep,
                             const void *buf,
                             size_t len,
                             void *desc,
                             fi_addr_t dest_addr,
                             uint64_t addr,
                             uint64_t key,
                             void *context) {
  struct fit_ep *h = EP_FROM_RMA(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_write", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_rma->write, (ep, buf, len, desc, dest_addr, addr, key, context));
  fit_record(h, "fi_write", 'T', dest_addr, len, 1, ret, context);
  return ret;
}

static ssize_t fit_rma_writev(struct fid_ep *ep,
                              const struct iovec *iov,
                              void **desc,
                              size_t count,
                              fi_addr_t dest_addr,
                              uint64_t addr,
                              uint64_t key,
                              void *context) {
  struct fit_ep *h = EP_FROM_RMA(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_writev", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_rma->writev, (ep, iov, desc, count, dest_addr, addr, key, context));
  fit_record(h, "fi_writev", 'T', dest_addr, fit_iov_len(iov, count), count, ret, context);
  return ret;
}

static ssize_t fit_rma_writemsg(struct fid_ep *ep, const struct fi_msg_rma *msg, uint64_t flags) {
  struct fit_ep *h = EP_FROM_RMA(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_writemsg", 'T', msg ? msg->context : NULL);
  ret = CALL_OR_ENOSYS(h->o_rma->writemsg, (ep, msg, flags));
  fit_record(h, "fi_writemsg", 'T', msg ? msg->addr : FI_ADDR_UNSPEC,
             msg ? fit_iov_len(msg->msg_iov, msg->iov_count) : 0, msg ? msg->iov_count : 0, ret,
             msg ? msg->context : NULL);
  return ret;
}

static ssize_t fit_rma_inject(struct fid_ep *ep,
                              const void *buf,
                              size_t len,
                              fi_addr_t dest_addr,
                              uint64_t addr,
                              uint64_t key) {
  struct fit_ep *h = EP_FROM_RMA(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_inject_write", 'T', NULL);
  ret = CALL_OR_ENOSYS(h->o_rma->inject, (ep, buf, len, dest_addr, addr, key));
  fit_record(h, "fi_inject_write", 'T', dest_addr, len, 1, ret, NULL);
  return ret;
}

static ssize_t fit_rma_writedata(struct fid_ep *ep,
                                 const void *buf,
                                 size_t len,
                                 void *desc,
                                 uint64_t data,
                                 fi_addr_t dest_addr,
                                 uint64_t addr,
                                 uint64_t key,
                                 void *context) {
  struct fit_ep *h = EP_FROM_RMA(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_writedata", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_rma->writedata,
                       (ep, buf, len, desc, data, dest_addr, addr, key, context));
  fit_record(h, "fi_writedata", 'T', dest_addr, len, 1, ret, context);
  return ret;
}

static ssize_t fit_rma_injectdata(struct fid_ep *ep,
                                  const void *buf,
                                  size_t len,
                                  uint64_t data,
                                  fi_addr_t dest_addr,
                                  uint64_t addr,
                                  uint64_t key) {
  struct fit_ep *h = EP_FROM_RMA(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_inject_writedata", 'T', NULL);
  ret = CALL_OR_ENOSYS(h->o_rma->injectdata, (ep, buf, len, data, dest_addr, addr, key));
  fit_record(h, "fi_inject_writedata", 'T', dest_addr, len, 1, ret, NULL);
  return ret;
}

static ssize_t fit_rma_read(struct fid_ep *ep,
                            void *buf,
                            size_t len,
                            void *desc,
                            fi_addr_t src_addr,
                            uint64_t addr,
                            uint64_t key,
                            void *context) {
  struct fit_ep *h = EP_FROM_RMA(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_read", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_rma->read, (ep, buf, len, desc, src_addr, addr, key, context));
  fit_record(h, "fi_read", 'T', src_addr, len, 1, ret, context);
  return ret;
}

static ssize_t fit_rma_readv(struct fid_ep *ep,
                             const struct iovec *iov,
                             void **desc,
                             size_t count,
                             fi_addr_t src_addr,
                             uint64_t addr,
                             uint64_t key,
                             void *context) {
  struct fit_ep *h = EP_FROM_RMA(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_readv", 'T', context);
  ret = CALL_OR_ENOSYS(h->o_rma->readv, (ep, iov, desc, count, src_addr, addr, key, context));
  fit_record(h, "fi_readv", 'T', src_addr, fit_iov_len(iov, count), count, ret, context);
  return ret;
}

static ssize_t fit_rma_readmsg(struct fid_ep *ep, const struct fi_msg_rma *msg, uint64_t flags) {
  struct fit_ep *h = EP_FROM_RMA(ep);
  ssize_t ret;

  fit_record_entry(h, "fi_readmsg", 'T', msg ? msg->context : NULL);
  ret = CALL_OR_ENOSYS(h->o_rma->readmsg, (ep, msg, flags));
  fit_record(h, "fi_readmsg", 'T', msg ? msg->addr : FI_ADDR_UNSPEC,
             msg ? fit_iov_len(msg->msg_iov, msg->iov_count) : 0, msg ? msg->iov_count : 0, ret,
             msg ? msg->context : NULL);
  return ret;
}

/* ================================================================== */
/* endpoint lifecycle                                                 */
/* ================================================================== */

/*
 * Install our copies of the endpoint's data-path vtables.
 *
 * Idempotent, and deliberately re-run after every bind and after
 * FI_ENABLE.  An endpoint is not finished at the moment fi_endpoint()
 * returns: a provider may hand back placeholder ops and swap in the real
 * ones later, once it knows which capabilities were negotiated and the
 * transmit and receive contexts exist.  Patching only at creation then
 * loses the race -- the provider overwrites our pointers, every
 * subsequent operation goes straight to the provider, and nothing
 * reports an error.  The symptom is a trace with a full set of "hooked"
 * lines and not one data record, which is precisely what CXI produced.
 *
 * Re-arming is cheap (three pointer compares when nothing changed) and
 * happens only on the control path, never per operation.
 */
static void fit_ep_arm_ops(struct fit_ep *h, const char *when) {
  struct fid_ep *ep = h->ep;

  if (fit_cfg.trace_msg && ep->msg && ep->msg != &h->msg) {
    if (h->o_msg)
      fit_log("endpoint %u: provider replaced msg ops at %s;"
              " re-arming",
              h->ep_id, when);
    h->o_msg = ep->msg;
    h->msg = *ep->msg;
    h->msg.send = fit_msg_send;
    h->msg.sendv = fit_msg_sendv;
    h->msg.sendmsg = fit_msg_sendmsg;
    h->msg.inject = fit_msg_inject;
    h->msg.senddata = fit_msg_senddata;
    h->msg.injectdata = fit_msg_injectdata;
    h->msg.recv = fit_msg_recv;
    h->msg.recvv = fit_msg_recvv;
    h->msg.recvmsg = fit_msg_recvmsg;
    ep->msg = &h->msg;
  }

  if (fit_cfg.trace_tagged && ep->tagged && ep->tagged != &h->tagged) {
    if (h->o_tagged)
      fit_log("endpoint %u: provider replaced tagged ops at"
              " %s; re-arming",
              h->ep_id, when);
    h->o_tagged = ep->tagged;
    h->tagged = *ep->tagged;
    h->tagged.send = fit_tag_send;
    h->tagged.sendv = fit_tag_sendv;
    h->tagged.sendmsg = fit_tag_sendmsg;
    h->tagged.inject = fit_tag_inject;
    h->tagged.senddata = fit_tag_senddata;
    h->tagged.injectdata = fit_tag_injectdata;
    h->tagged.recv = fit_tag_recv;
    h->tagged.recvv = fit_tag_recvv;
    h->tagged.recvmsg = fit_tag_recvmsg;
    ep->tagged = &h->tagged;
  }

  if (fit_cfg.trace_rma && ep->rma && ep->rma != &h->rma) {
    if (h->o_rma)
      fit_log("endpoint %u: provider replaced rma ops at %s;"
              " re-arming",
              h->ep_id, when);
    h->o_rma = ep->rma;
    h->rma = *ep->rma;
    h->rma.write = fit_rma_write;
    h->rma.writev = fit_rma_writev;
    h->rma.writemsg = fit_rma_writemsg;
    h->rma.inject = fit_rma_inject;
    h->rma.writedata = fit_rma_writedata;
    h->rma.injectdata = fit_rma_injectdata;
    h->rma.read = fit_rma_read;
    h->rma.readv = fit_rma_readv;
    h->rma.readmsg = fit_rma_readmsg;
    ep->rma = &h->rma;
  }
}

static int fit_ep_bind_inner(struct fid *fid, struct fid *bfid, uint64_t flags) {
  struct fit_ep *h = EP_FROM_FID(fid);
  int ret = h->o_fid->bind ? h->o_fid->bind(fid, bfid, flags) : -FI_ENOSYS;

  /* Binding a CQ or a counter is exactly the sort of event that tells
   * a provider which data path this endpoint will use. */
  if (!ret)
    fit_ep_arm_ops(h, "bind");

  if (ret || !bfid)
    return ret;

  /* Remember the AV so remote fi_addr_t values can be reverse-mapped. */
  if (bfid->fclass == FI_CLASS_AV)
    h->av = (struct fid_av *)bfid;

  /* And note which endpoint owns the CQ, so its completions can be
   * attributed to a local NIC address. */
  if (bfid->fclass == FI_CLASS_CQ)
    fit_cq_attach_ep(bfid, h);

  return ret;
}

static int fit_ep_bind(struct fid *fid, struct fid *bfid, uint64_t flags) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(EP_FROM_FID(fid)->ep_id);
  int fit_ret;

  fit_record_ctrl_entry("fi_ep_bind", fit_obj);
  fit_ret = fit_ep_bind_inner(fid, bfid, flags);
  fit_record_ctrl_exit("fi_ep_bind", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_ep_control_inner(struct fid *fid, int command, void *arg) {
  struct fit_ep *h = EP_FROM_FID(fid);
  int ret = h->o_fid->control ? h->o_fid->control(fid, command, arg) : -FI_ENOSYS;

  /* The local address only becomes available once enabled -- and so,
   * for some providers, does the real data path. */
  if (!ret && command == FI_ENABLE) {
    fit_ep_arm_ops(h, "enable");
    fit_ep_resolve_src(h);
  }

  return ret;
}

static int fit_ep_control(struct fid *fid, int command, void *arg) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(EP_FROM_FID(fid)->ep_id);
  int fit_ret;

  fit_record_ctrl_entry("fi_ep_control", fit_obj);
  fit_ret = fit_ep_control_inner(fid, command, arg);
  fit_record_ctrl_exit("fi_ep_control", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_ep_close_inner(struct fid *fid) {
  struct fit_ep *h = EP_FROM_FID(fid);
  int ret;

  ret = h->o_fid->close ? h->o_fid->close(fid) : -FI_ENOSYS;
  if (ret)
    return ret;

  /* The provider object is gone; our side state can go too. */
  fit_ep_peers_free(h);
  pthread_mutex_destroy(&h->lock);
  free(h);
  return 0;
}

static int fit_ep_close(struct fid *fid) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(EP_FROM_FID(fid)->ep_id);
  int fit_ret;

  fit_record_ctrl_entry("fi_close_ep", fit_obj);
  fit_ret = fit_ep_close_inner(fid);
  fit_record_ctrl_exit("fi_close_ep", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_ep_tx_ctx_inner(
    struct fid_ep *sep, int index, struct fi_tx_attr *attr, struct fid_ep **tx_ep, void *context) {
  struct fit_ep *h = EP_FROM_EPOPS(sep);
  int ret = h->o_ep->tx_ctx ? h->o_ep->tx_ctx(sep, index, attr, tx_ep, context) : -FI_ENOSYS;

  if (!ret && tx_ep && *tx_ep)
    fit_hook_ep(*tx_ep, h->topo);
  return ret;
}

static int fit_ep_tx_ctx(
    struct fid_ep *sep, int index, struct fi_tx_attr *attr, struct fid_ep **tx_ep, void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(EP_FROM_EPOPS(sep)->ep_id);
  int fit_ret;

  fit_record_ctrl_entry("fi_tx_context", fit_obj);
  fit_ret = fit_ep_tx_ctx_inner(sep, index, attr, tx_ep, context);
  fit_record_ctrl_exit("fi_tx_context", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_ep_rx_ctx_inner(
    struct fid_ep *sep, int index, struct fi_rx_attr *attr, struct fid_ep **rx_ep, void *context) {
  struct fit_ep *h = EP_FROM_EPOPS(sep);
  int ret = h->o_ep->rx_ctx ? h->o_ep->rx_ctx(sep, index, attr, rx_ep, context) : -FI_ENOSYS;

  if (!ret && rx_ep && *rx_ep)
    fit_hook_ep(*rx_ep, h->topo);
  return ret;
}

static int fit_ep_rx_ctx(
    struct fid_ep *sep, int index, struct fi_rx_attr *attr, struct fid_ep **rx_ep, void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(EP_FROM_EPOPS(sep)->ep_id);
  int fit_ret;

  fit_record_ctrl_entry("fi_rx_context", fit_obj);
  fit_ret = fit_ep_rx_ctx_inner(sep, index, attr, rx_ep, context);
  fit_record_ctrl_exit("fi_rx_context", fit_obj, fit_ret);
  return fit_ret;
}

void fit_hook_ep(struct fid_ep *ep, const struct fit_topo *topo) {
  struct fit_ep *h;

  if (!ep)
    return;

  h = calloc(1, sizeof(*h));
  if (!h) {
    fit_log("out of memory hooking endpoint");
    return;
  }

  pthread_mutex_init(&h->lock, NULL);
  h->ep = ep;
  h->ep_id = atomic_fetch_add(&fit_ep_counter, 1);
  /* Borrowed from the domain, which outlives every endpoint on it. */
  h->topo = topo;
  snprintf(h->src, sizeof(h->src), "unknown");
  fit_sanitize(h->src, &h->src_len);

  fit_ep_arm_ops(h, "create");

  if (ep->fid.ops) {
    h->o_fid = ep->fid.ops;
    h->fid_ops = *ep->fid.ops;
    h->fid_ops.bind = fit_ep_bind;
    h->fid_ops.control = fit_ep_control;
    h->fid_ops.close = fit_ep_close;
    ep->fid.ops = &h->fid_ops;
  }

  if (ep->ops) {
    h->o_ep = ep->ops;
    h->ep_ops = *ep->ops;
    h->ep_ops.tx_ctx = fit_ep_tx_ctx;
    h->ep_ops.rx_ctx = fit_ep_rx_ctx;
    ep->ops = &h->ep_ops;
  }

  fit_log("hooked endpoint %u (%p)", h->ep_id, (void *)ep);
}

/* ================================================================== */
/* address vector                                                     */
/*                                                                    */
/* Wrapped for one reason only: fi_av_remove() frees an fi_addr_t for  */
/* reuse, which silently invalidates every cached fi_addr_t -> NIC     */
/* address mapping.  Bump a generation counter so the caches notice.   */
/* ================================================================== */

#define AV_FROM_OPS(av) fit_container_of((av)->ops, struct fit_av, ops)

static int
fit_av_remove_inner(struct fid_av *av, fi_addr_t *fi_addr, size_t count, uint64_t flags) {
  struct fit_av *a = AV_FROM_OPS(av);
  int ret = a->orig->remove ? a->orig->remove(av, fi_addr, count, flags) : -FI_ENOSYS;

  if (!ret)
    atomic_fetch_add_explicit(&fit_av_gen, 1, memory_order_release);
  return ret;
}

static int fit_av_remove(struct fid_av *av, fi_addr_t *fi_addr, size_t count, uint64_t flags) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_av_remove", fit_obj);
  fit_ret = fit_av_remove_inner(av, fi_addr, count, flags);
  fit_record_ctrl_exit("fi_av_remove", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_av_close_inner(struct fid *fid) {
  struct fit_av *a = fit_container_of(fid->ops, struct fit_av, fid_ops);
  int ret = a->o_fid->close ? a->o_fid->close(fid) : -FI_ENOSYS;

  if (!ret) {
    atomic_fetch_add_explicit(&fit_av_gen, 1, memory_order_release);
    free(a);
  }
  return ret;
}

static int fit_av_close(struct fid *fid) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_close_av", fit_obj);
  fit_ret = fit_av_close_inner(fid);
  fit_record_ctrl_exit("fi_close_av", fit_obj, fit_ret);
  return fit_ret;
}

void fit_hook_av(struct fid_av *av) {
  struct fit_av *a;

  if (!av || !av->ops || av->ops->remove == fit_av_remove)
    return;

  a = calloc(1, sizeof(*a));
  if (!a)
    return;

  if (av->fid.ops) {
    a->o_fid = av->fid.ops;
    a->fid_ops = *av->fid.ops;
    a->fid_ops.close = fit_av_close;
    av->fid.ops = &a->fid_ops;
  }

  a->orig = av->ops;
  a->ops = *av->ops;
  a->ops.remove = fit_av_remove;
  av->ops = &a->ops;

  fit_log("hooked address vector %p", (void *)av);
}

/* ================================================================== */
/* completion queue                                                   */
/*                                                                    */
/* This is where an operation acquires a duration.  The posting record */
/* says when the application handed the transfer to the provider; the  */
/* completion record says when the application learned it was done.    */
/* They are joined on op_context.                                      */
/*                                                                     */
/* The completion also carries two things the posting record cannot:   */
/* the length the provider actually transferred (a receive posts a     */
/* buffer, not a message size), and, via fi_cq_readfrom(), the address */
/* the data really came from.                                          */
/* ================================================================== */

#define CQ_FROM_OPS(cq) fit_container_of((cq)->ops, struct fit_cq, ops)

static atomic_uint fit_cq_counter;

/* Every fi_cq_*_entry layout begins with these fields, in this order,
 * so one accessor serves all of them.  FI_CQ_FORMAT_CONTEXT stops after
 * op_context, which is why entry_sz decides how much may be read. */
struct fit_cqe_head {
  void *op_context;
  uint64_t flags;
  size_t len;
};

static size_t fit_cq_entry_size(enum fi_cq_format format) {
  switch (format) {
  case FI_CQ_FORMAT_CONTEXT:
    return sizeof(struct fi_cq_entry);
  case FI_CQ_FORMAT_MSG:
    return sizeof(struct fi_cq_msg_entry);
  case FI_CQ_FORMAT_DATA:
    return sizeof(struct fi_cq_data_entry);
  case FI_CQ_FORMAT_TAGGED:
    return sizeof(struct fi_cq_tagged_entry);
  default:
    return 0;
  }
}

/*
 * Name the operation the completion refers to.  Flags are only present
 * from FI_CQ_FORMAT_MSG upwards; with FI_CQ_FORMAT_CONTEXT the provider
 * tells us nothing but the context, and the posting record it joins to
 * has to supply the operation type.
 */
static const char *fit_cq_opname(uint64_t flags, int have_flags) {
  if (!have_flags)
    return "cq_completion";

  if (flags & FI_ATOMIC)
    return (flags & FI_REMOTE_WRITE) ? "cq_remote_atomic" : "cq_atomic";
  if (flags & FI_REMOTE_WRITE)
    return "cq_remote_write";
  if (flags & FI_REMOTE_READ)
    return "cq_remote_read";
  if (flags & FI_WRITE)
    return "cq_write";
  if (flags & FI_READ)
    return "cq_read";
  if (flags & FI_SEND)
    return (flags & FI_TAGGED) ? "cq_tsend" : "cq_send";
  if (flags & FI_RECV)
    return (flags & FI_TAGGED) ? "cq_trecv" : "cq_recv";
  return "cq_completion";
}

/* Walk a batch the provider has just filled in and emit one record per
 * entry.  src is the parallel array fi_cq_readfrom() fills, or NULL. */
/*
 * A provider may implement one read entry point in terms of another, and
 * it does so through the *public* inline wrapper -- the tcp provider's
 * ofi_cq_read() is literally "return fi_cq_readfrom(cq, buf, count,
 * NULL)".  That inline dispatches through the vtable, so it lands back in
 * here and one application poll would be recorded twice.
 *
 * Count the nesting and report only from the outermost frame: that is the
 * call the application actually made, and it is the one carrying the src
 * array if there is one, since an internal re-dispatch can only drop
 * information, never add it.
 */
static __thread unsigned fit_cq_depth;

static void fit_cq_report(struct fit_cq *c, const void *buf, ssize_t n, const fi_addr_t *src) {
  int have_flags = c->entry_sz >= sizeof(struct fi_cq_msg_entry);
  uint64_t now = fit_now_us();
  ssize_t i;

  if (!c->entry_sz)
    return;

  for (i = 0; i < n; i++) {
    const struct fit_cqe_head *e =
        (const struct fit_cqe_head *)((const char *)buf + (size_t)i * c->entry_sz);
    uint64_t flags = have_flags ? e->flags : 0;
    size_t len = have_flags ? e->len : 0;

    fit_record_cq(c, fit_cq_opname(flags, have_flags), src ? src[i] : FI_ADDR_UNSPEC, len, 0,
                  e->op_context, now);
  }
}

static ssize_t fit_cq_read(struct fid_cq *cq, void *buf, size_t count) {
  struct fit_cq *c = CQ_FROM_OPS(cq);
  unsigned outer = fit_cq_depth++;
  ssize_t ret;

  ret = CALL_OR_ENOSYS(c->orig->read, (cq, buf, count));
  fit_cq_depth = outer;

  /* A polling loop calls this constantly and almost always gets
   * -FI_EAGAIN.  Nothing happened, so nothing is recorded. */
  if (ret > 0 && !outer)
    fit_cq_report(c, buf, ret, NULL);
  return ret;
}

static ssize_t fit_cq_readfrom(struct fid_cq *cq, void *buf, size_t count, fi_addr_t *src_addr) {
  struct fit_cq *c = CQ_FROM_OPS(cq);
  unsigned outer = fit_cq_depth++;
  ssize_t ret;

  ret = CALL_OR_ENOSYS(c->orig->readfrom, (cq, buf, count, src_addr));
  fit_cq_depth = outer;

  if (ret > 0 && !outer)
    fit_cq_report(c, buf, ret, src_addr);
  return ret;
}

static ssize_t
fit_cq_sread(struct fid_cq *cq, void *buf, size_t count, const void *cond, int timeout) {
  struct fit_cq *c = CQ_FROM_OPS(cq);
  unsigned outer = fit_cq_depth++;
  ssize_t ret;

  ret = CALL_OR_ENOSYS(c->orig->sread, (cq, buf, count, cond, timeout));
  fit_cq_depth = outer;

  if (ret > 0 && !outer)
    fit_cq_report(c, buf, ret, NULL);
  return ret;
}

static ssize_t fit_cq_sreadfrom(struct fid_cq *cq,
                                void *buf,
                                size_t count,
                                fi_addr_t *src_addr,
                                const void *cond,
                                int timeout) {
  struct fit_cq *c = CQ_FROM_OPS(cq);
  unsigned outer = fit_cq_depth++;
  ssize_t ret;

  ret = CALL_OR_ENOSYS(c->orig->sreadfrom, (cq, buf, count, src_addr, cond, timeout));
  fit_cq_depth = outer;

  if (ret > 0 && !outer)
    fit_cq_report(c, buf, ret, src_addr);
  return ret;
}

/*
 * A failed operation never appears in the normal completion stream, so
 * without this a transfer that errored would show a posting record and
 * no completion, indistinguishable from one still in flight.
 */
static ssize_t fit_cq_readerr(struct fid_cq *cq, struct fi_cq_err_entry *buf, uint64_t flags) {
  struct fit_cq *c = CQ_FROM_OPS(cq);
  ssize_t ret = CALL_OR_ENOSYS(c->orig->readerr, (cq, buf, flags));

  if (ret > 0 && buf)
    fit_record_cq(c, fit_cq_opname(buf->flags, 1),
#ifdef HAVE_FI_CQ_ERR_SRC_ADDR
                  buf->src_addr,
#else
                  /* Older libfabric has no src_addr here. */
                  FI_ADDR_UNSPEC,
#endif
                  buf->len, -buf->err, buf->op_context, fit_now_us());
  return ret;
}

static int fit_cq_close_inner(struct fid *fid) {
  struct fit_cq *c = fit_container_of(fid->ops, struct fit_cq, fid_ops);
  int ret;

  ret = c->o_fid->close ? c->o_fid->close(fid) : -FI_ENOSYS;
  if (!ret)
    free(c);
  return ret;
}

static int fit_cq_close(struct fid *fid) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(fit_container_of(fid->ops, struct fit_cq, fid_ops)->cq_id);
  int fit_ret;

  fit_record_ctrl_entry("fi_close_cq", fit_obj);
  fit_ret = fit_cq_close_inner(fid);
  fit_record_ctrl_exit("fi_close_cq", fit_obj, fit_ret);
  return fit_ret;
}

void fit_hook_cq(struct fid_cq *cq, enum fi_cq_format format) {
  struct fit_cq *c;

  if (!cq || !cq->ops || cq->ops->read == fit_cq_read)
    return;

  c = calloc(1, sizeof(*c));
  if (!c)
    return;

  c->cq = cq;
  c->cq_id = atomic_fetch_add(&fit_cq_counter, 1);
  c->format = format;
  c->entry_sz = fit_cq_entry_size(format);

  /*
   * FI_CQ_FORMAT_UNSPEC leaves the choice to the provider and there
   * is no way to ask what it picked.  Every layout starts with
   * op_context, so fall back to that much: completions are still
   * timestamped and still join to their posting record, they just
   * carry no length of their own.
   */
  if (!c->entry_sz) {
    c->entry_sz = sizeof(struct fi_cq_entry);
    fit_log("cq %u opened with FI_CQ_FORMAT_UNSPEC; "
            "completion lengths unavailable",
            c->cq_id);
  }

  if (cq->fid.ops) {
    c->o_fid = cq->fid.ops;
    c->fid_ops = *cq->fid.ops;
    c->fid_ops.close = fit_cq_close;
    cq->fid.ops = &c->fid_ops;
  }

  c->orig = cq->ops;
  c->ops = *cq->ops;
  c->ops.read = fit_cq_read;
  c->ops.readfrom = fit_cq_readfrom;
  c->ops.readerr = fit_cq_readerr;
  c->ops.sread = fit_cq_sread;
  c->ops.sreadfrom = fit_cq_sreadfrom;
  cq->ops = &c->ops;

  fit_log("hooked cq %u (%p) format %d", c->cq_id, (void *)cq, (int)format);
}

/* Called when an endpoint binds a CQ, so a completion can be attributed
 * to a local NIC address.  Only safe if we hooked that CQ ourselves --
 * the read slot is the tell. */
static void fit_cq_attach_ep(struct fid *bfid, struct fit_ep *h) {
  struct fid_cq *cq = (struct fid_cq *)bfid;
  struct fit_cq *c;

  if (!cq->ops || cq->ops->read != fit_cq_read)
    return;

  c = CQ_FROM_OPS(cq);
  if (c->ep && c->ep != h) {
    /* Shared across endpoints; guessing one would be wrong. */
    c->ep_ambiguous = 1;
    c->ep = NULL;
    return;
  }
  if (!c->ep_ambiguous)
    c->ep = h;
}

/* ================================================================== */
/* domain                                                             */
/* ================================================================== */

#define DOM_FROM_OPS(dom) fit_container_of((dom)->ops, struct fit_domain, ops)

static int fit_dom_endpoint_inner(struct fid_domain *domain,
                                  struct fi_info *info,
                                  struct fid_ep **ep,
                                  void *context) {
  struct fit_domain *d = DOM_FROM_OPS(domain);
  int ret = d->orig->endpoint ? d->orig->endpoint(domain, info, ep, context) : -FI_ENOSYS;

  if (!ret && ep && *ep)
    fit_hook_ep(*ep, &d->topo);
  return ret;
}

static int fit_dom_endpoint(struct fid_domain *domain,
                            struct fi_info *info,
                            struct fid_ep **ep,
                            void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_endpoint", fit_obj);
  fit_ret = fit_dom_endpoint_inner(domain, info, ep, context);
  fit_record_ctrl_exit("fi_endpoint", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_dom_endpoint2_inner(struct fid_domain *domain,
                                   struct fi_info *info,
                                   struct fid_ep **ep,
                                   uint64_t flags,
                                   void *context) {
  struct fit_domain *d = DOM_FROM_OPS(domain);
  int ret = d->orig->endpoint2 ? d->orig->endpoint2(domain, info, ep, flags, context) : -FI_ENOSYS;

  if (!ret && ep && *ep)
    fit_hook_ep(*ep, &d->topo);
  return ret;
}

static int fit_dom_endpoint2(struct fid_domain *domain,
                             struct fi_info *info,
                             struct fid_ep **ep,
                             uint64_t flags,
                             void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_endpoint2", fit_obj);
  fit_ret = fit_dom_endpoint2_inner(domain, info, ep, flags, context);
  fit_record_ctrl_exit("fi_endpoint2", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_dom_scalable_ep_inner(struct fid_domain *domain,
                                     struct fi_info *info,
                                     struct fid_ep **sep,
                                     void *context) {
  struct fit_domain *d = DOM_FROM_OPS(domain);
  int ret = d->orig->scalable_ep ? d->orig->scalable_ep(domain, info, sep, context) : -FI_ENOSYS;

  if (!ret && sep && *sep)
    fit_hook_ep(*sep, &d->topo);
  return ret;
}

static int fit_dom_scalable_ep(struct fid_domain *domain,
                               struct fi_info *info,
                               struct fid_ep **sep,
                               void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_scalable_ep", fit_obj);
  fit_ret = fit_dom_scalable_ep_inner(domain, info, sep, context);
  fit_record_ctrl_exit("fi_scalable_ep", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_dom_srx_ctx_inner(struct fid_domain *domain,
                                 struct fi_rx_attr *attr,
                                 struct fid_ep **rx_ep,
                                 void *context) {
  struct fit_domain *d = DOM_FROM_OPS(domain);
  int ret = d->orig->srx_ctx ? d->orig->srx_ctx(domain, attr, rx_ep, context) : -FI_ENOSYS;

  /* Off by default.  Several providers (tcp among them) route an
   * endpoint's receives through an internal shared receive context, so
   * hooking both records every fi_recv twice -- once on the endpoint
   * the application called, once on the srx underneath it.  Set
   * FI_TRACE_SRX=1 if the application posts to an srx directly. */
  if (!ret && rx_ep && *rx_ep && fit_cfg.trace_srx)
    fit_hook_ep(*rx_ep, &d->topo);
  return ret;
}

static int fit_dom_srx_ctx(struct fid_domain *domain,
                           struct fi_rx_attr *attr,
                           struct fid_ep **rx_ep,
                           void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_srx_context", fit_obj);
  fit_ret = fit_dom_srx_ctx_inner(domain, attr, rx_ep, context);
  fit_record_ctrl_exit("fi_srx_context", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_dom_cq_open_inner(struct fid_domain *domain,
                                 struct fi_cq_attr *attr,
                                 struct fid_cq **cq,
                                 void *context) {
  struct fit_domain *d = DOM_FROM_OPS(domain);
  int ret = d->orig->cq_open ? d->orig->cq_open(domain, attr, cq, context) : -FI_ENOSYS;

  if (!ret && cq && *cq && fit_cfg.trace_cq)
    fit_hook_cq(*cq, attr ? attr->format : FI_CQ_FORMAT_UNSPEC);
  return ret;
}

static int fit_dom_cq_open(struct fid_domain *domain,
                           struct fi_cq_attr *attr,
                           struct fid_cq **cq,
                           void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_cq_open", fit_obj);
  fit_ret = fit_dom_cq_open_inner(domain, attr, cq, context);
  fit_record_ctrl_exit("fi_cq_open", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_dom_av_open_inner(struct fid_domain *domain,
                                 struct fi_av_attr *attr,
                                 struct fid_av **av,
                                 void *context) {
  struct fit_domain *d = DOM_FROM_OPS(domain);
  int ret = d->orig->av_open ? d->orig->av_open(domain, attr, av, context) : -FI_ENOSYS;

  if (!ret && av && *av)
    fit_hook_av(*av);
  return ret;
}

static int fit_dom_av_open(struct fid_domain *domain,
                           struct fi_av_attr *attr,
                           struct fid_av **av,
                           void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_av_open", fit_obj);
  fit_ret = fit_dom_av_open_inner(domain, attr, av, context);
  fit_record_ctrl_exit("fi_av_open", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_dom_close_inner(struct fid *fid) {
  struct fit_domain *d = fit_container_of(fid->ops, struct fit_domain, fid_ops);
  int ret = d->o_fid->close ? d->o_fid->close(fid) : -FI_ENOSYS;

  if (!ret)
    free(d);
  return ret;
}

static int fit_dom_close(struct fid *fid) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_close_domain", fit_obj);
  fit_ret = fit_dom_close_inner(fid);
  fit_record_ctrl_exit("fi_close_domain", fit_obj, fit_ret);
  return fit_ret;
}

void fit_hook_domain(struct fid_domain *domain, const struct fi_info *info) {
  struct fit_domain *d;

  if (!domain || !domain->ops)
    return;

  d = calloc(1, sizeof(*d));
  if (!d)
    return;

  /* Before patching anything: the extension interface is reached
   * through the domain's own ops, and asking while they are still
   * entirely the provider's keeps this independent of what we are
   * about to change. */
  fit_topo_resolve(&d->topo, domain, info);

  if (domain->fid.ops) {
    d->o_fid = domain->fid.ops;
    d->fid_ops = *domain->fid.ops;
    d->fid_ops.close = fit_dom_close;
    domain->fid.ops = &d->fid_ops;
  }

  d->orig = domain->ops;
  d->ops = *domain->ops;
  d->ops.endpoint = fit_dom_endpoint;
  d->ops.endpoint2 = fit_dom_endpoint2;
  d->ops.scalable_ep = fit_dom_scalable_ep;
  d->ops.srx_ctx = fit_dom_srx_ctx;
  d->ops.cq_open = fit_dom_cq_open;
  d->ops.av_open = fit_dom_av_open;
  domain->ops = &d->ops;

  if (d->topo.valid)
    fit_log("hooked domain %p (%s: group %u switch %u port %u)", (void *)domain, d->topo.name,
            d->topo.group, d->topo.sw, d->topo.port);
  else
    fit_log("hooked domain %p", (void *)domain);
}

/* ================================================================== */
/* fabric                                                             */
/* ================================================================== */

#define FAB_FROM_OPS(fab) fit_container_of((fab)->ops, struct fit_fabric, ops)

static int fit_fab_domain_inner(struct fid_fabric *fabric,
                                struct fi_info *info,
                                struct fid_domain **dom,
                                void *context) {
  struct fit_fabric *f = FAB_FROM_OPS(fabric);
  int ret = f->orig->domain ? f->orig->domain(fabric, info, dom, context) : -FI_ENOSYS;

  if (!ret && dom && *dom)
    fit_hook_domain(*dom, info);
  return ret;
}

static int fit_fab_domain(struct fid_fabric *fabric,
                          struct fi_info *info,
                          struct fid_domain **dom,
                          void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_domain", fit_obj);
  fit_ret = fit_fab_domain_inner(fabric, info, dom, context);
  fit_record_ctrl_exit("fi_domain", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_fab_domain2_inner(struct fid_fabric *fabric,
                                 struct fi_info *info,
                                 struct fid_domain **dom,
                                 uint64_t flags,
                                 void *context) {
  struct fit_fabric *f = FAB_FROM_OPS(fabric);
  int ret = f->orig->domain2 ? f->orig->domain2(fabric, info, dom, flags, context) : -FI_ENOSYS;

  if (!ret && dom && *dom)
    fit_hook_domain(*dom, info);
  return ret;
}

static int fit_fab_domain2(struct fid_fabric *fabric,
                           struct fi_info *info,
                           struct fid_domain **dom,
                           uint64_t flags,
                           void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_domain2", fit_obj);
  fit_ret = fit_fab_domain2_inner(fabric, info, dom, flags, context);
  fit_record_ctrl_exit("fi_domain2", fit_obj, fit_ret);
  return fit_ret;
}

static int fit_fab_close_inner(struct fid *fid) {
  struct fit_fabric *f = fit_container_of(fid->ops, struct fit_fabric, fid_ops);
  int ret;

  ret = f->o_fid->close ? f->o_fid->close(fid) : -FI_ENOSYS;
  if (!ret)
    free(f);
  return ret;
}

static int fit_fab_close(struct fid *fid) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_close_fabric", fit_obj);
  fit_ret = fit_fab_close_inner(fid);
  fit_record_ctrl_exit("fi_close_fabric", fit_obj, fit_ret);
  return fit_ret;
}

void fit_hook_fabric(struct fid_fabric *fabric) {
  struct fit_fabric *f;

  if (!fabric || !fabric->ops)
    return;

  f = calloc(1, sizeof(*f));
  if (!f)
    return;

  if (fabric->fid.ops) {
    f->o_fid = fabric->fid.ops;
    f->fid_ops = *fabric->fid.ops;
    f->fid_ops.close = fit_fab_close;
    fabric->fid.ops = &f->fid_ops;
  }

  f->orig = fabric->ops;
  f->ops = *fabric->ops;
  f->ops.domain = fit_fab_domain;
  f->ops.domain2 = fit_fab_domain2;
  fabric->ops = &f->ops;

  fit_log("hooked fabric %p", (void *)fabric);
}

/* ================================================================== */
/* interposed exported symbols                                        */
/* ================================================================== */

typedef int (*fi_fabric_fn)(struct fi_fabric_attr *, struct fid_fabric **, void *);
typedef int (*fi_fabric2_fn)(struct fi_info *, struct fid_fabric **, uint64_t, void *);

static fi_fabric_fn real_fi_fabric;
static fi_fabric2_fn real_fi_fabric2;

/*
 * libfabric exports several versions of the same name: fi_fabric exists as
 * both fi_fabric@FABRIC_1.0 and the default fi_fabric@@FABRIC_1.1.  A plain
 * dlsym() is free to return either, and on glibc it returns the 1.0 compat
 * stub -- which immediately calls fi_fabric() again, lands back in this
 * library, and recurses until the stack runs out.
 *
 * So ask for an explicit version, newest first, and take the first hit.
 * The newest defined version of a symbol is by convention the default one.
 * A libfabric built without symbol versioning has no versions to match, and
 * the plain dlsym() fallback is then unambiguous.
 */
#ifndef __GLIBC__
#error "fi_trace is Linux/glibc only: it needs LD_PRELOAD symbol interposition, \
dlvsym() to pick the right versioned fi_fabric, and gettid(2). \
On macOS or elsewhere, run it in a container: ./build/docker-demo.sh"
#endif

static void *fit_dlnext(const char *name) {
  static const char *const vers[] = {
      "FABRIC_1.9", "FABRIC_1.8", "FABRIC_1.7", "FABRIC_1.6", "FABRIC_1.5",
      "FABRIC_1.4", "FABRIC_1.3", "FABRIC_1.2", "FABRIC_1.1", "FABRIC_1.0",
  };
  size_t i;
  void *sym;

  for (i = 0; i < sizeof(vers) / sizeof(vers[0]); i++) {
    sym = dlvsym(RTLD_NEXT, name, vers[i]);
    if (sym)
      return sym;
  }

  (void)dlerror();
  return dlsym(RTLD_NEXT, name);
}

__attribute__((constructor)) static void fit_init(void) {
  fit_config_init();

  if (!fit_cfg.enabled) {
    fit_log("disabled via FI_TRACE_ENABLE");
    return;
  }

  real_fi_fabric = (fi_fabric_fn)fit_dlnext("fi_fabric");
  real_fi_fabric2 = (fi_fabric2_fn)fit_dlnext("fi_fabric2");

  if (!real_fi_fabric && !real_fi_fabric2) {
    fit_log("cannot find libfabric fi_fabric(); tracing off");
    return;
  }

  // fit_writer_init();
  fit_armed = 1;
  fit_log("armed: dir=%s ops=%s%s%s rx=%d resolve=%d", fit_cfg.dir, fit_cfg.trace_msg ? "msg " : "",
          fit_cfg.trace_tagged ? "tagged " : "", fit_cfg.trace_rma ? "rma" : "", fit_cfg.trace_rx,
          fit_cfg.resolve);
}

/* These must be exported even though the library is built with
 * -fvisibility=hidden -- they are the whole point of the preload. */
#define FIT_PUBLIC __attribute__((visibility("default")))

FIT_PUBLIC int fi_fabric(struct fi_fabric_attr *attr, struct fid_fabric **fabric, void *context);
FIT_PUBLIC int
fi_fabric2(struct fi_info *info, struct fid_fabric **fabric, uint64_t flags, void *context);

static int fi_fabric_inner(struct fi_fabric_attr *attr, struct fid_fabric **fabric, void *context) {
  int ret;

  if (!real_fi_fabric)
    real_fi_fabric = (fi_fabric_fn)fit_dlnext("fi_fabric");
  if (!real_fi_fabric)
    return -FI_ENOSYS;

  ret = real_fi_fabric(attr, fabric, context);
  if (!ret && fit_armed && fabric && *fabric)
    fit_hook_fabric(*fabric);
  return ret;
}

FIT_PUBLIC
int fi_fabric(struct fi_fabric_attr *attr, struct fid_fabric **fabric, void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_fabric", fit_obj);
  fit_ret = fi_fabric_inner(attr, fabric, context);
  fit_record_ctrl_exit("fi_fabric", fit_obj, fit_ret);
  return fit_ret;
}

static int
fi_fabric2_inner(struct fi_info *info, struct fid_fabric **fabric, uint64_t flags, void *context) {
  int ret;

  if (!real_fi_fabric2)
    real_fi_fabric2 = (fi_fabric2_fn)fit_dlnext("fi_fabric2");
  if (!real_fi_fabric2)
    return -FI_ENOSYS;

  ret = real_fi_fabric2(info, fabric, flags, context);
  if (!ret && fit_armed && fabric && *fabric)
    fit_hook_fabric(*fabric);
  return ret;
}

FIT_PUBLIC
int fi_fabric2(struct fi_info *info, struct fid_fabric **fabric, uint64_t flags, void *context) {
  /* Captured before the call: the close wrappers free this object. */
  uint32_t fit_obj = (uint32_t)(0);
  int fit_ret;

  fit_record_ctrl_entry("fi_fabric2", fit_obj);
  fit_ret = fi_fabric2_inner(info, fabric, flags, context);
  fit_record_ctrl_exit("fi_fabric2", fit_obj, fit_ret);
  return fit_ret;
}
