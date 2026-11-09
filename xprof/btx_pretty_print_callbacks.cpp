// sink.thapipretty.text: print the `thapi_pretty:line` events made by the
// per-backend pretty filters, one per line, in the order they arrive. Any other
// event is printed the way the Ruby printer prints an event it has no printer
// for (the bt2 Ruby gem `Field#to_s`), unless `restricted` is true.
//
// A metababel SINK of btx_pretty_model.yaml: the lines come to a callback, every
// other event to on_upstream_message_callback.
#include <babeltrace2/babeltrace.h>
#include <metababel/metababel.h>

#include "thapi_pretty.hpp"

#include <cctype>
#include <cstdio>

using namespace thapi_pretty;

namespace {

struct printer_t {
  bool context = false;
  bool restrict_ = false;
  out_t out;
};

template <class T> void dec(out_t &o, T v) { thapi_pretty::dec(it_t(o), v); }

// A name as the gem prints it: one starting with ':' is a Ruby symbol, printed
// with Symbol#inspect (`:name`, or `:"..."` when it is not an identifier).
void name(out_t &o, const char *n) {
  if (n[0] != ':') return put(o, n);
  const char *s = n + 1;
  bool ident = std::isalpha((unsigned char)s[0]) || s[0] == '_';
  for (const char *c = s; *c && ident; c++) ident = std::isalnum((unsigned char)*c) || *c == '_';
  put(o, ":");
  if (ident) put(o, s);
  else ruby_inspect(o, s);
}

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
  if (bt_field_class_type_is(t, BT_FIELD_CLASS_TYPE_ENUMERATION)) { // "(value) [labels]"
    bt_field_class_enumeration_mapping_label_array labels;
    uint64_t count = 0;
    put(o, "(");
    if (bt_field_class_type_is(t, BT_FIELD_CLASS_TYPE_SIGNED_ENUMERATION)) {
      bt_field_enumeration_signed_get_mapping_labels(f, &labels, &count);
      dec(o, bt_field_integer_signed_get_value(f));
    } else {
      bt_field_enumeration_unsigned_get_mapping_labels(f, &labels, &count);
      dec(o, bt_field_integer_unsigned_get_value(f));
    }
    put(o, ") [");
    for (uint64_t i = 0; i < count; i++) {
      if (i) put(o, ", ");
      name(o, labels[i]);
    }
    put(o, "]");
    return;
  }
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
    return;
  }
  switch (t) {
  case BT_FIELD_CLASS_TYPE_BOOL: put(o, bt_field_bool_get_value(f) ? "true" : "false"); return;
  case BT_FIELD_CLASS_TYPE_SINGLE_PRECISION_REAL: ruby_float(o, bt_field_real_single_precision_get_value(f)); return;
  case BT_FIELD_CLASS_TYPE_DOUBLE_PRECISION_REAL: ruby_float(o, bt_field_real_double_precision_get_value(f)); return;
  case BT_FIELD_CLASS_TYPE_STRING: put(o, bt_field_string_get_value(f)); return;
  case BT_FIELD_CLASS_TYPE_STRUCTURE: {
    put(o, "{");
    uint64_t n = bt_field_class_structure_get_member_count(fc);
    for (uint64_t i = 0; i < n; i++) {
      if (i) put(o, ", ");
      name(o, bt_field_class_structure_member_get_name(bt_field_class_structure_borrow_member_by_index_const(fc, i)));
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
    if (bt_field_class_type_is(t, BT_FIELD_CLASS_TYPE_VARIANT)) { // "{option: value}"
      put(o, "{");
      const bt_field *x = bt_field_variant_borrow_selected_option_field_const(f);
      if (x) {
        name(o, bt_field_class_variant_option_get_name(bt_field_variant_borrow_selected_option_class_const(f)));
        put(o, ": ");
        field(o, x);
      }
      put(o, "}");
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

void fallback(printer_t *p, const bt_message *m, const bt_event *e) {
  out_t &o = p->out;
  o.clear();
  int64_t ns = 0;
  bt_clock_snapshot_get_ns_from_origin(bt_message_event_borrow_default_clock_snapshot_const(m), &ns);
  ruby_time(o, ns);
  if (p->context) {
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
  const bt_field *payload = bt_event_borrow_payload_field_const(e);
  if (payload) field(o, payload);
  o.push_back('\n');
  std::fwrite(o.data(), 1, o.size(), stdout);
}

void initialize(void **usr_data) { *usr_data = new printer_t; }

void finalize(void *usr_data) {
  std::fflush(stdout);
  delete static_cast<printer_t *>(usr_data);
}

void read_params(void *usr_data, btx_params_t *params) {
  auto *p = static_cast<printer_t *>(usr_data);
  p->context = params->context;
  p->restrict_ = params->restricted;
}

void line(void *btx_handle, void *usr_data, int64_t ts, const char *text) {
  std::fputs(text, stdout);
  std::fputc('\n', stdout);
}

// The events no pretty filter formatted (and the other messages, ignored).
void on_upstream_message(void *btx_handle, void *usr_data, const bt_message *m) {
  auto *p = static_cast<printer_t *>(usr_data);
  if (!p->restrict_ && bt_message_get_type(m) == BT_MESSAGE_TYPE_EVENT)
    fallback(p, m, bt_message_event_borrow_event_const(m));
}

} // namespace

void btx_register_usr_callbacks(void *btx_handle) {
  btx_register_callbacks_initialize_component(btx_handle, &initialize);
  btx_register_callbacks_finalize_component(btx_handle, &finalize);
  btx_register_callbacks_read_params(btx_handle, &read_params);
  btx_register_callbacks_thapi_pretty_line(btx_handle, &line);
  btx_register_on_upstream_message_callback(btx_handle, &on_upstream_message);
}
