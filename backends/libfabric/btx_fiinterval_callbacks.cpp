/*
 * Analysis side of the libfabric backend: turn the raw lttng_ust_fi trace into
 * THAPI's "interval" stream.
 *
 * Three jobs, in increasing order of difficulty:
 *
 *   1. Pair op_entry with op_exit for a per-call duration.  This is the usual
 *      THAPI entry/exit pairing and measures how long the libfabric call
 *      itself took -- i.e. how long the provider spent accepting the request.
 *
 *   2. Join a posting to its completion for a transfer duration.  This is the
 *      part no other THAPI backend has to do.  A libfabric transfer is not a
 *      function call with a return; it is a posting that the application later
 *      reaps from a completion queue, possibly from a different thread.  The
 *      two are joined on op_context, which is the application's own pointer.
 *
 *   3. Attribute bytes to a peer, for the traffic stream.  Only the completion
 *      knows how many bytes actually moved.
 *
 * ep_info events build a side table so the pretty-printer and the traffic
 * metadata can name the NIC and its dragonfly coordinates; this replaces the
 * sidecar CSV + fitrace-merge join of the standalone tracer.
 */

#include "xprof_utils.hpp"
#include <metababel/metababel.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/* ------------------------------------------------------------------ */
/* state                                                              */
/* ------------------------------------------------------------------ */

struct posting_s {
  int64_t ts;            /* when the application posted            */
  std::string op;        /* "fi_send", "fi_trecv", ...             */
  std::string peer;      /* as known at posting time; a wildcard
                            receive will be corrected by the
                            completion, which knows the real sender */
  uint64_t posted_bytes; /* buffer length, not transferred length  */
};

struct ep_info_s {
  std::string src_addr;
  std::string domain_name;
  bool topo_valid;
  uint32_t group, switch_id, port;
};

struct data_s {
  /*
   * Per-thread LIFO of open entry timestamps.  A wrapper cannot re-enter
   * itself, but a layered provider (rxm over verbs, or the tcp provider
   * implementing fi_cq_read in terms of fi_cq_readfrom) dispatches back
   * through the public inline and lands in another wrapper, so nesting is
   * real and a stack is required rather than a single slot.
   */
  std::unordered_map<hpt_t, std::vector<std::pair<int64_t, std::string>>> op_stack;
  std::unordered_map<hpt_t, std::vector<std::pair<int64_t, std::string>>> ctrl_stack;

  /*
   * Outstanding postings keyed by {hostname, vpid, op_context}.
   *
   * Note what is NOT in the key: vtid.  A progress thread reaping another
   * thread's completions is a normal libfabric design, so keying on the
   * posting thread would silently drop every join in such an application.
   * The cost is that two threads in one process posting with the same context
   * pointer at the same time become ambiguous -- but libfabric already
   * requires op_context to be unique among outstanding operations on an
   * endpoint, so a correct application cannot hit this.
   */
  std::unordered_map<std::string, posting_s> outstanding;

  std::unordered_map<std::string, ep_info_s> endpoints; /* key: host/pid/ep_id */
};
typedef struct data_s data_t;

static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }

static void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }

/* ------------------------------------------------------------------ */
/* helpers                                                            */
/* ------------------------------------------------------------------ */

static std::string ctx_key(const char *hostname, int64_t vpid, uintptr_t op_context) {
  return std::string(hostname) + "/" + std::to_string(vpid) + "/" + std::to_string(op_context);
}

static std::string ep_key(const char *hostname, int64_t vpid, uint32_t ep_id) {
  return std::string(hostname) + "/" + std::to_string(vpid) + "/" + std::to_string(ep_id);
}

/*
 * Traffic metadata: who the bytes went to or came from, qualified by the local
 * NIC when we know it.  This is the string that shows up in iprof's traffic
 * table, so it wants to be stable and greppable rather than pretty.
 */
static std::string traffic_metadata(
    data_t *s, const char *hostname, int64_t vpid, uint32_t ep_id, const std::string &peer) {
  auto it = s->endpoints.find(ep_key(hostname, vpid, ep_id));
  if (it == s->endpoints.end())
    return peer;

  std::string m = it->second.src_addr + "->" + peer;
  if (it->second.topo_valid)
    m += " [" + it->second.domain_name + " g" + std::to_string(it->second.group) + "s" +
         std::to_string(it->second.switch_id) + "p" + std::to_string(it->second.port) + "]";
  return m;
}

/* ------------------------------------------------------------------ */
/* data path                                                          */
/* ------------------------------------------------------------------ */

static void op_entry_callback(void *btx_handle,
                              void *usr_data,
                              int64_t ts,
                              const char *hostname,
                              int64_t vpid,
                              uint64_t vtid,
                              char *op,
                              uint32_t ep_id,
                              uintptr_t op_context) {
  auto *s = static_cast<data_t *>(usr_data);
  s->op_stack[{hostname, vpid, vtid}].push_back({ts, op});
}

static void op_exit_callback(void *btx_handle,
                             void *usr_data,
                             int64_t ts,
                             const char *hostname,
                             int64_t vpid,
                             uint64_t vtid,
                             char *op,
                             uint32_t ep_id,
                             char *src_addr,
                             char *peer_addr,
                             char *direction,
                             uint64_t iov_count,
                             uint64_t size_bytes,
                             int64_t ret,
                             uintptr_t op_context) {
  auto *s = static_cast<data_t *>(usr_data);
  auto &stack = s->op_stack[{hostname, vpid, vtid}];

  /*
   * An exit with no entry happens at the head of a trace that started while
   * the application was already inside libfabric.  Drop it rather than
   * inventing a start timestamp.
   */
  if (stack.empty())
    return;

  const int64_t entry_ts = stack.back().first;
  const std::string entry_op = stack.back().second;
  stack.pop_back();

  /*
   * Duration of the libfabric call itself.  err_flag is set on any negative
   * return, which includes -FI_EAGAIN; that is a legitimate "try later", not a
   * failure, but it is also not work the application wanted, and flagging it
   * makes it visible in the tally instead of inflating the success counts.
   */
  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, entry_ts, BACKEND_FI,
                              entry_op.c_str(), (ts - entry_ts), (ret < 0));

  if (ret < 0)
    return; /* nothing became outstanding; the tracer already nulled context */

  /*
   * A NULL op_context is legal and common -- fi_inject takes no context at
   * all, and plenty of applications pass NULL to the others.  Such a posting
   * can never be joined to a completion, because nothing distinguishes it from
   * any other contextless operation in flight.  Record it as a call (done
   * above) but not as an outstanding transfer.
   */
  if (!op_context)
    return;

  /*
   * Insert overwrites by design.  op_context is the application's pointer and
   * is recycled the instant an operation completes, so at most one live entry
   * per key is correct; an overwrite means we missed a completion.
   */
  s->outstanding[ctx_key(hostname, vpid, op_context)] = {entry_ts, entry_op, peer_addr, size_bytes};
}

static void cq_completion_callback(void *btx_handle,
                                   void *usr_data,
                                   int64_t ts,
                                   const char *hostname,
                                   int64_t vpid,
                                   uint64_t vtid,
                                   char *op,
                                   uint32_t obj_id,
                                   char *src_addr,
                                   char *peer_addr,
                                   uint64_t size_bytes,
                                   int64_t ret,
                                   uintptr_t op_context) {
  auto *s = static_cast<data_t *>(usr_data);

  if (!op_context)
    return;

  auto it = s->outstanding.find(ctx_key(hostname, vpid, op_context));
  if (it == s->outstanding.end())
    return; /* completion for a posting we never saw -- normal at trace start */

  const posting_s post = it->second;
  s->outstanding.erase(it);

  /*
   * The duration the application actually experienced: post to reap.
   *
   * This is an upper bound on the transfer time, and deliberately so.  An
   * application that posts and then does ten milliseconds of compute before
   * polling will show ten milliseconds here.  That is the right number to
   * report -- it is the latency the application lived with -- but it is not
   * the NIC's transfer time, and nothing in a libfabric trace can give you
   * that.  Compare against the CXI counters (iprof --backends fi,cxi) when you
   * need the hardware view.
   */
  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, post.ts, BACKEND_FI,
                              (post.op + ".transfer").c_str(), (ts - post.ts), (ret < 0));

  if (ret < 0)
    return;

  /*
   * Bytes are attributed from the completion, not the posting.  A receive
   * posts a buffer and completes with the message length; using the posted
   * length would overstate received traffic by whatever slack the application
   * left.  Likewise peer_addr: a wildcard receive posts FI_ADDR_UNSPEC and
   * only fi_cq_readfrom() reveals who actually sent.
   */
  const std::string peer = (peer_addr && peer_addr[0] && std::string(peer_addr) != "fi_addr_unspec")
                               ? peer_addr
                               : post.peer;

  btx_push_message_lttng_traffic(btx_handle, hostname, vpid, vtid, post.ts, BACKEND_FI,
                                 post.op.c_str(), size_bytes,
                                 traffic_metadata(s, hostname, vpid, obj_id, peer).c_str());
}

/* ------------------------------------------------------------------ */
/* control path                                                       */
/* ------------------------------------------------------------------ */

static void ctrl_entry_callback(void *btx_handle,
                                void *usr_data,
                                int64_t ts,
                                const char *hostname,
                                int64_t vpid,
                                uint64_t vtid,
                                char *op,
                                uint32_t obj_id) {
  auto *s = static_cast<data_t *>(usr_data);
  s->ctrl_stack[{hostname, vpid, vtid}].push_back({ts, op});
}

static void ctrl_exit_callback(void *btx_handle,
                               void *usr_data,
                               int64_t ts,
                               const char *hostname,
                               int64_t vpid,
                               uint64_t vtid,
                               char *op,
                               uint32_t obj_id,
                               int64_t ret) {
  auto *s = static_cast<data_t *>(usr_data);
  auto &stack = s->ctrl_stack[{hostname, vpid, vtid}];
  if (stack.empty())
    return;

  /*
   * The control path nests for real: fi_domain() calls into the provider,
   * which opens an endpoint, which binds a CQ.  A separate stack from the data
   * path keeps that nesting from interleaving with postings.
   */
  const int64_t entry_ts = stack.back().first;
  const std::string entry_op = stack.back().second;
  stack.pop_back();

  btx_push_message_lttng_host(btx_handle, hostname, vpid, vtid, entry_ts, BACKEND_FI,
                              entry_op.c_str(), (ts - entry_ts), (ret < 0));
}

/* ------------------------------------------------------------------ */
/* endpoint description                                               */
/* ------------------------------------------------------------------ */

static void ep_info_callback(void *btx_handle,
                             void *usr_data,
                             int64_t ts,
                             const char *hostname,
                             int64_t vpid,
                             uint64_t vtid,
                             uint32_t ep_id,
                             char *src_addr,
                             char *domain_name,
                             uint32_t topo_valid,
                             uint32_t group,
                             uint32_t switch_id,
                             uint32_t port) {
  auto *s = static_cast<data_t *>(usr_data);
  s->endpoints[ep_key(hostname, vpid, ep_id)] = {src_addr, domain_name, topo_valid != 0,
                                                 group,    switch_id,   port};
}

/* ------------------------------------------------------------------ */

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
  btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);

  btx_register_callbacks_lttng_ust_fi_op_entry(btx_handle, &op_entry_callback);
  btx_register_callbacks_lttng_ust_fi_op_exit(btx_handle, &op_exit_callback);
  btx_register_callbacks_lttng_ust_fi_cq_completion(btx_handle, &cq_completion_callback);
  btx_register_callbacks_lttng_ust_fi_ctrl_entry(btx_handle, &ctrl_entry_callback);
  btx_register_callbacks_lttng_ust_fi_ctrl_exit(btx_handle, &ctrl_exit_callback);
  btx_register_callbacks_lttng_ust_fi_ep_info(btx_handle, &ep_info_callback);
}
