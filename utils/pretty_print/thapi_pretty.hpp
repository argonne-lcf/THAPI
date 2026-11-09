// Formatting primitives that reproduce the Ruby pretty printer
// (babeltrace_thapi `trace`) byte for byte, and the fmt::formatter pieces the
// generated per-backend formatters use.
#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string_view>

namespace thapi_pretty {

using out_t = fmt::memory_buffer;
using it_t = fmt::appender;

inline it_t put(it_t o, std::string_view s) { return std::copy(s.begin(), s.end(), o); }

template <class T> inline it_t dec(it_t o, T v) { return fmt::format_to(o, "{}", v); }
inline it_t dec(it_t o, int8_t v) { return dec(o, (int)v); }
inline it_t dec(it_t o, uint8_t v) { return dec(o, (unsigned)v); }
inline it_t dec(it_t o, char v) { return dec(o, (int)(signed char)v); }

// Ruby `Float#to_s`: shortest round-trip digits; always at least one fractional
// digit. Fixed notation when the decimal point position `decpt` is in (-4, 16),
// or is 16 and there are fractional digits; else `d.ddde+XX`.
// The shortest digits come from fmt (`{}`, Dragonbox: integer arithmetic on the
// bits); zero, sign, NaN and infinity are tested on the bits too, so a process
// running with DAZ/FTZ (the babeltrace2 CLI built by icx does) prints the same.
inline it_t ruby_float(it_t o, double v) {
  uint64_t bits;
  std::memcpy(&bits, &v, sizeof bits);
  bool neg = bits >> 63;
  uint64_t mag = bits & 0x7fffffffffffffffULL;
  if (mag > 0x7ff0000000000000ULL) return put(o, "NaN");
  if (mag == 0x7ff0000000000000ULL) return put(o, neg ? "-Infinity" : "Infinity");
  if (mag == 0) return put(o, neg ? "-0.0" : "0.0");

  // fmt writes the shortest digits in fixed or exponent form ("123.45", "1e-05",
  // "1.5e+16"): read them back as `digits` and the decimal point position.
  char buf[64];
  *fmt::format_to(buf, "{}", v) = '\0';
  const char *p = buf;
  if (*p == '-') *o++ = *p++;
  char digits[32];
  int n = 0, decpt = 0;
  bool point = false;
  for (; *p && *p != 'e'; p++) {
    if (*p == '.') { point = true; continue; }
    if (n == 0 && *p == '0') { if (point) decpt--; continue; } // leading zeros
    digits[n++] = *p;
    if (!point) decpt++;
  }
  if (*p == 'e') decpt += static_cast<int>(std::strtol(p + 1, nullptr, 10));
  while (n > 1 && digits[n - 1] == '0') n--; // trailing zeros of an integer

  if (-4 < decpt && (decpt < 16 || (decpt == 16 && n > decpt))) {
    if (decpt <= 0) {
      o = put(o, "0.");
      for (int i = 0; i < -decpt; i++) *o++ = '0';
      return put(o, std::string_view(digits, n));
    }
    if (n <= decpt) {
      o = put(o, std::string_view(digits, n));
      for (int i = n; i < decpt; i++) *o++ = '0';
      return put(o, ".0");
    }
    o = put(o, std::string_view(digits, decpt));
    *o++ = '.';
    return put(o, std::string_view(digits + decpt, n - decpt));
  }
  *o++ = digits[0];
  *o++ = '.';
  o = n == 1 ? put(o, "0") : put(o, std::string_view(digits + 1, n - 1));
  return fmt::format_to(o, "e{:+03d}", decpt - 1);
}

// A float widened to double. A denormal float is widened with integer
// arithmetic: under DAZ the FPU would read it as 0.
inline double widen(float f) {
  uint32_t b;
  std::memcpy(&b, &f, sizeof b);
  uint32_t m = b & 0x7fffffu;
  if ((b & 0x7f800000u) != 0 || m == 0) return f; // normal, zero, inf, NaN: exact
  int p = 31 - __builtin_clz(m);                  // highest set bit: value = 2^(p-149) * 1.f
  uint64_t d = (uint64_t)(b >> 31) << 63 | (uint64_t)(p - 149 + 1023) << 52 | (uint64_t)(m - (1u << p)) << (52 - p);
  double r;
  std::memcpy(&r, &d, sizeof r);
  return r;
}
// Ruby `Float#to_s` of a single-precision value (FFI and bt2 give it as a double).
inline it_t ruby_float(it_t o, float f) { return ruby_float(o, widen(f)); }

// Ruby `String#inspect` of a binary (ASCII-8BIT) string.
inline it_t ruby_inspect(it_t o, const char *s, size_t len) {
  *o++ = '"';
  for (size_t i = 0; i < len; i++) {
    unsigned char c = s[i];
    switch (c) {
    case '"': o = put(o, "\\\""); break;
    case '\\': o = put(o, "\\\\"); break;
    case '\n': o = put(o, "\\n"); break;
    case '\r': o = put(o, "\\r"); break;
    case '\t': o = put(o, "\\t"); break;
    case '\f': o = put(o, "\\f"); break;
    case '\v': o = put(o, "\\v"); break;
    case '\b': o = put(o, "\\b"); break;
    case '\a': o = put(o, "\\a"); break;
    case 0x1b: o = put(o, "\\e"); break;
    case '#':
      o = (i + 1 < len && (s[i + 1] == '{' || s[i + 1] == '$' || s[i + 1] == '@')) ? put(o, "\\#") : put(o, "#");
      break;
    default:
      if (c >= 0x20 && c < 0x7f) *o++ = c;
      else o = fmt::format_to(o, "\\x{:02X}", c);
    }
  }
  *o++ = '"';
  return o;
}

// Ruby `"0x%016x" % v`.
inline it_t ruby_hex(it_t o, uint64_t v) { return fmt::format_to(o, "0x{:016x}", v); }

// Ruby `"0x%016x" % v` of a signed value: a negative value prints as ".." and
// its two's complement digits, sign-extended with 'f' to the width.
inline it_t ruby_hex_signed(it_t o, int64_t v) {
  if (v >= 0) return ruby_hex(o, (uint64_t)v);
  char d[17];
  fmt::format_to(d, "{:016x}", (uint64_t)v);
  int i = 0;
  while (i < 15 && d[i] == 'f' && d[i + 1] >= '8') i++; // drop sign digits the next one repeats
  std::string_view digits(d + i, 16 - i);
  o = put(o, "0x..");
  if (digits[0] != 'f') *o++ = 'f';
  for (size_t k = digits.size() + (digits[0] != 'f'); k < 14; k++) *o++ = 'f';
  return put(o, digits);
}

// Ruby `Time.at(0, ns, :nsec).strftime('%H:%M:%S.%9L')` (local time).
inline it_t ruby_time(it_t o, int64_t ns) {
  int64_t s = ns / 1000000000, f = ns % 1000000000;
  if (f < 0) { f += 1000000000; s -= 1; }
  time_t t = s;
  struct tm tm;
  localtime_r(&t, &tm);
  return fmt::format_to(o, "{:02}:{:02}:{:02}.{:09}", tm.tm_hour, tm.tm_min, tm.tm_sec, f);
}

// FFI `CharArray#to_s`: the bytes up to the first NUL.
inline it_t char_array(it_t o, const char *s, size_t max) { return put(o, std::string_view(s, strnlen(s, max))); }

// A 16-byte UUID, dashes after 4-2-2-2: ze prints the bytes back to front,
// cuda, hip and mpi in order.
inline it_t uuid(it_t o, const uint8_t *a, bool reverse) {
  for (int i = 0; i < 16; i++) {
    if (i == 4 || i == 6 || i == 8 || i == 10) *o++ = '-';
    o = fmt::format_to(o, "{:02x}", a[reverse ? 15 - i : i]);
  }
  return o;
}

// Base of the generated formatters: no format spec.
struct no_spec {
  constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }
};

// The same primitives on a buffer.
inline void put(out_t &o, std::string_view s) { o.append(s.data(), s.data() + s.size()); }
inline void ruby_float(out_t &o, double v) { ruby_float(it_t(o), v); }
inline void ruby_float(out_t &o, float v) { ruby_float(it_t(o), v); }
inline void ruby_inspect(out_t &o, const char *s, size_t len) { ruby_inspect(it_t(o), s, len); }
inline void ruby_inspect(out_t &o, const char *s) { ruby_inspect(it_t(o), s, std::strlen(s)); }
inline void ruby_hex(out_t &o, uint64_t v) { ruby_hex(it_t(o), v); }
inline void ruby_time(out_t &o, int64_t ns) { ruby_time(it_t(o), ns); }

} // namespace thapi_pretty
