// sink.thapipretty.text: print the `thapi_pretty:line` events made by the
// per-backend pretty filters, one per line, in the order they arrive. Any other
// event is printed the way the Ruby printer prints an event it has no printer
// for (the bt2 Ruby gem `Field#to_s`), unless `restrict` is true.
#include <babeltrace2/babeltrace.h>

#include "thapi_pretty.hpp"

#include <cstdio>

using namespace thapi_pretty;

namespace {

struct sink_t {
  bt_message_iterator *it = nullptr;
  bool context = false;
  bool restrict_ = false;
  out_t out;
};

void dec(out_t &o, int64_t v) { fmt::format_to(std::back_inserter(o), "{}", v); }
void dec(out_t &o, uint64_t v) { fmt::format_to(std::back_inserter(o), "{}", v); }

// Ruby `"#{v}"` of a field value (used for the context fields).
void value(out_t &o, const bt_field *f) {
  switch (bt_field_get_class_type(f)) {
  case BT_FIELD_CLASS_TYPE_STRING: put(o, bt_field_string_get_value(f)); break;
  case BT_FIELD_CLASS_TYPE_UNSIGNED_INTEGER: dec(o, bt_field_integer_unsigned_get_value(f)); break;
  case BT_FIELD_CLASS_TYPE_SIGNED_INTEGER: dec(o, bt_field_integer_signed_get_value(f)); break;
  default: put(o, "?");
  }
}

// bt2 Ruby gem `BTField#to_s`.
void field(out_t &o, const bt_field *f) {
  const bt_field_class *fc = bt_field_borrow_class_const(f);
  bt_field_class_type t = bt_field_get_class_type(f);
  if (bt_field_class_type_is(t, BT_FIELD_CLASS_TYPE_INTEGER)) {
    bool is_signed = bt_field_class_type_is(t, BT_FIELD_CLASS_TYPE_SIGNED_INTEGER);
    uint64_t u = is_signed ? (uint64_t)bt_field_integer_signed_get_value(f) : bt_field_integer_unsigned_get_value(f);
    int64_t s = (int64_t)u;
    uint64_t bits = bt_field_class_integer_get_field_value_range(fc);
    uint64_t twos = (is_signed && s < 0 && bits < 64) ? u & ((1ULL << bits) - 1) : u;
    switch (bt_field_class_integer_get_preferred_display_base(fc)) {
    case BT_FIELD_CLASS_INTEGER_PREFERRED_DISPLAY_BASE_BINARY: fmt::format_to(std::back_inserter(o), "0b{:b}", twos); break;
    case BT_FIELD_CLASS_INTEGER_PREFERRED_DISPLAY_BASE_OCTAL: fmt::format_to(std::back_inserter(o), "0{:o}", twos); break;
    case BT_FIELD_CLASS_INTEGER_PREFERRED_DISPLAY_BASE_HEXADECIMAL: fmt::format_to(std::back_inserter(o), "0x{:x}", twos); break;
    default: is_signed ? dec(o, s) : dec(o, u);
    }
    // TODO enumeration fields: "(v) [labels]" -- none in the backends handled so far.
    return;
  }
  switch (t) {
  case BT_FIELD_CLASS_TYPE_BOOL: put(o, bt_field_bool_get_value(f) ? "true" : "false"); return;
  case BT_FIELD_CLASS_TYPE_SINGLE_PRECISION_REAL:
    ruby_float(o, bt_field_real_single_precision_get_value(f));
    return;
  case BT_FIELD_CLASS_TYPE_DOUBLE_PRECISION_REAL:
    ruby_float(o, bt_field_real_double_precision_get_value(f));
    return;
  case BT_FIELD_CLASS_TYPE_STRING: put(o, bt_field_string_get_value(f)); return;
  case BT_FIELD_CLASS_TYPE_STRUCTURE: {
    put(o, "{");
    uint64_t n = bt_field_class_structure_get_member_count(fc);
    for (uint64_t i = 0; i < n; i++) {
      if (i) put(o, ", ");
      put(o, bt_field_class_structure_member_get_name(bt_field_class_structure_borrow_member_by_index_const(fc, i)));
      put(o, ": ");
      field(o, bt_field_structure_borrow_member_field_by_index_const(f, i));
    }
    put(o, "}");
    return;
  }
  default:
    if (bt_field_class_type_is(t, BT_FIELD_CLASS_TYPE_ARRAY)) {
      put(o, "[");
      uint64_t n = bt_field_array_get_length(f);
      for (uint64_t i = 0; i < n; i++) {
        if (i) put(o, ", ");
        field(o, bt_field_array_borrow_element_field_by_index_const(f, i));
      }
      put(o, "]");
      return;
    }
    if (bt_field_class_type_is(t, BT_FIELD_CLASS_TYPE_BLOB)) {
      const char *d = (const char *)bt_field_blob_get_data_const(f);
      put(o, std::string_view(d, bt_field_blob_get_length(f)));
      return;
    }
    if (bt_field_class_type_is(t, BT_FIELD_CLASS_TYPE_OPTION)) {
      const bt_field *x = bt_field_option_borrow_field_const(f);
      if (x) field(o, x);
      return;
    }
    put(o, "?");
  }
}

void fallback(sink_t *s, const bt_message *m, const bt_event *e) {
  out_t &o = s->out;
  o.clear();
  int64_t ns = 0;
  bt_clock_snapshot_get_ns_from_origin(bt_message_event_borrow_default_clock_snapshot_const(m), &ns);
  ruby_time(o, ns);
  if (s->context) {
    put(o, " - ");
    const bt_trace *tr = bt_stream_borrow_trace_const(bt_event_borrow_stream_const(e));
    const bt_value *h = bt_trace_borrow_environment_entry_value_by_name_const(tr, "hostname");
    if (h) put(o, bt_value_string_get(h));
    const bt_field *cc = bt_event_borrow_common_context_field_const(e);
    if (cc) {
      put(o, " - ");
      const bt_field_class *fc = bt_field_borrow_class_const(cc);
      uint64_t n = bt_field_class_structure_get_member_count(fc);
      for (uint64_t i = 0; i < n; i++) {
        if (i) put(o, ", ");
        put(o, bt_field_class_structure_member_get_name(bt_field_class_structure_borrow_member_by_index_const(fc, i)));
        put(o, ": ");
        value(o, bt_field_structure_borrow_member_field_by_index_const(cc, i));
      }
    }
  }
  put(o, " - ");
  put(o, bt_event_class_get_name(bt_event_borrow_class_const(e)));
  put(o, ": ");
  const bt_field *p = bt_event_borrow_payload_field_const(e);
  if (p) field(o, p);
  o.push_back('\n');
  std::fwrite(o.data(), 1, o.size(), stdout);
}

bt_component_class_initialize_method_status init(bt_self_component_sink *self, bt_self_component_sink_configuration *,
                                                 const bt_value *params, void *) {
  auto *s = new sink_t;
  const bt_value *v;
  if ((v = bt_value_map_borrow_entry_value_const(params, "context"))) s->context = bt_value_bool_get(v);
  if ((v = bt_value_map_borrow_entry_value_const(params, "restrict"))) s->restrict_ = bt_value_bool_get(v);
  bt_self_component_set_data(bt_self_component_sink_as_self_component(self), s);
  bt_self_component_sink_add_input_port(self, "in", nullptr, nullptr);
  static char buf[1 << 20];
  std::setvbuf(stdout, buf, _IOFBF, sizeof buf);
  return BT_COMPONENT_CLASS_INITIALIZE_METHOD_STATUS_OK;
}

void finalize(bt_self_component_sink *self) {
  auto *s = static_cast<sink_t *>(bt_self_component_get_data(bt_self_component_sink_as_self_component(self)));
  std::fflush(stdout);
  delete s;
}

bt_component_class_sink_graph_is_configured_method_status configured(bt_self_component_sink *self) {
  auto *s = static_cast<sink_t *>(bt_self_component_get_data(bt_self_component_sink_as_self_component(self)));
  bt_self_component_port_input *port = bt_self_component_sink_borrow_input_port_by_index(self, 0);
  bt_message_iterator_create_from_sink_component(self, port, &s->it);
  return BT_COMPONENT_CLASS_SINK_GRAPH_IS_CONFIGURED_METHOD_STATUS_OK;
}

bt_component_class_sink_consume_method_status consume(bt_self_component_sink *self) {
  auto *s = static_cast<sink_t *>(bt_self_component_get_data(bt_self_component_sink_as_self_component(self)));
  bt_message_array_const msgs;
  uint64_t count;
  switch (bt_message_iterator_next(s->it, &msgs, &count)) {
  case BT_MESSAGE_ITERATOR_NEXT_STATUS_END:
    bt_message_iterator_put_ref(s->it);
    s->it = nullptr;
    return BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_END;
  case BT_MESSAGE_ITERATOR_NEXT_STATUS_AGAIN: return BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_AGAIN;
  case BT_MESSAGE_ITERATOR_NEXT_STATUS_MEMORY_ERROR: return BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_MEMORY_ERROR;
  case BT_MESSAGE_ITERATOR_NEXT_STATUS_ERROR: return BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_ERROR;
  default: break;
  }
  for (uint64_t i = 0; i < count; i++) {
    const bt_message *m = msgs[i];
    if (bt_message_get_type(m) == BT_MESSAGE_TYPE_EVENT) {
      const bt_event *e = bt_message_event_borrow_event_const(m);
      const char *name = bt_event_class_get_name(bt_event_borrow_class_const(e));
      if (std::strcmp(name, "thapi_pretty:line") == 0) {
        const bt_field *f = bt_field_structure_borrow_member_field_by_index_const(bt_event_borrow_payload_field_const(e), 0);
        std::fwrite(bt_field_string_get_value(f), 1, bt_field_string_get_length(f), stdout);
        std::fputc('\n', stdout);
      } else if (!s->restrict_) {
        fallback(s, m, e);
      }
    }
    bt_message_put_ref(m);
  }
  return BT_COMPONENT_CLASS_SINK_CONSUME_METHOD_STATUS_OK;
}

// MIP 1 is needed for BLOB fields; nothing here depends on the version.
bt_component_class_get_supported_mip_versions_method_status mips(bt_self_component_class_sink *, const bt_value *, void *,
                                                                 bt_logging_level,
                                                                 bt_integer_range_set_unsigned *versions) {
  bt_integer_range_set_unsigned_add_range(versions, 0, 1);
  return BT_COMPONENT_CLASS_GET_SUPPORTED_MIP_VERSIONS_METHOD_STATUS_OK;
}

} // namespace

BT_PLUGIN_MODULE();
BT_PLUGIN(thapipretty);
BT_PLUGIN_SINK_COMPONENT_CLASS(text, consume);
BT_PLUGIN_SINK_COMPONENT_CLASS_INITIALIZE_METHOD(text, init);
BT_PLUGIN_SINK_COMPONENT_CLASS_FINALIZE_METHOD(text, finalize);
BT_PLUGIN_SINK_COMPONENT_CLASS_GRAPH_IS_CONFIGURED_METHOD(text, configured);
BT_PLUGIN_SINK_COMPONENT_CLASS_GET_SUPPORTED_MIP_VERSIONS_METHOD(text, mips);
