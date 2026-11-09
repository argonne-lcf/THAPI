// Formatting primitives that reproduce the Ruby pretty printer
// (babeltrace_thapi `trace`) byte for byte.
#pragma once

#include <fmt/format.h>

#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string_view>
#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

namespace thapi_pretty {

using out_t = fmt::memory_buffer;

inline void put(out_t &o, std::string_view s) { o.append(s.data(), s.data() + s.size()); }

// Ruby `Float#to_s`: shortest round-trip digits; always at least one fractional
// digit. Fixed notation when the decimal point position `decpt` is in (-4, 16),
// or is 16 and there are fractional digits; else `d.ddde+XX`.
// Zero, sign, NaN and infinity are tested on the bits, and DAZ/FTZ are cleared
// while a denormal is formatted: a process that runs with DAZ set (the babeltrace2
// CLI built by icx does) would otherwise see a denormal as 0, and so does to_chars.
inline void ruby_float(out_t &o, double v) {
  uint64_t bits;
  std::memcpy(&bits, &v, sizeof bits);
  bool neg = bits >> 63;
  uint64_t mag = bits & 0x7fffffffffffffffULL;
  if (mag > 0x7ff0000000000000ULL) return put(o, "NaN");
  if (mag == 0x7ff0000000000000ULL) return put(o, neg ? "-Infinity" : "Infinity");
  if (mag == 0) return put(o, neg ? "-0.0" : "0.0");

  // to_chars scientific without precision: shortest round-trip digits, "-1.2345e+15".
  // (fmt `{:e}` without precision uses 6 digits, like printf.)
  char buf[64];
#if defined(__x86_64__) || defined(__i386__)
  bool denormal = (mag >> 52) == 0;
  unsigned csr = denormal ? _mm_getcsr() : 0;
  if (denormal) _mm_setcsr(csr & ~0x8040u);
#endif
  *std::to_chars(buf, buf + sizeof buf - 1, v, std::chars_format::scientific).ptr = '\0';
#if defined(__x86_64__) || defined(__i386__)
  if (denormal) _mm_setcsr(csr);
#endif
  const char *p = buf;
  if (*p == '-') o.push_back(*p++);
  char digits[32] = {};
  int n = 0;
  for (; *p != 'e'; p++)
    if (*p != '.') digits[n++] = *p;
  int decpt = static_cast<int>(std::strtol(p + 1, nullptr, 10)) + 1;

  if (-4 < decpt && (decpt < 16 || (decpt == 16 && n > decpt))) {
    if (decpt <= 0) {
      put(o, "0.");
      for (int i = 0; i < -decpt; i++) o.push_back('0');
      o.append(digits, digits + n);
    } else if (n <= decpt) {
      o.append(digits, digits + n);
      for (int i = n; i < decpt; i++) o.push_back('0');
      put(o, ".0");
    } else {
      o.append(digits, digits + decpt);
      o.push_back('.');
      o.append(digits + decpt, digits + n);
    }
  } else {
    o.push_back(digits[0]);
    o.push_back('.');
    if (n == 1) o.push_back('0');
    else o.append(digits + 1, digits + n);
    fmt::format_to(std::back_inserter(o), "e{:+03d}", decpt - 1);
  }
}

// Ruby `String#inspect` of a binary (ASCII-8BIT) string.
inline void ruby_inspect(out_t &o, const char *s, size_t len) {
  o.push_back('"');
  for (size_t i = 0; i < len; i++) {
    unsigned char c = s[i];
    switch (c) {
    case '"': put(o, "\\\""); break;
    case '\\': put(o, "\\\\"); break;
    case '\n': put(o, "\\n"); break;
    case '\r': put(o, "\\r"); break;
    case '\t': put(o, "\\t"); break;
    case '\f': put(o, "\\f"); break;
    case '\v': put(o, "\\v"); break;
    case '\b': put(o, "\\b"); break;
    case '\a': put(o, "\\a"); break;
    case 0x1b: put(o, "\\e"); break;
    case '#':
      if (i + 1 < len && (s[i + 1] == '{' || s[i + 1] == '$' || s[i + 1] == '@')) put(o, "\\#");
      else o.push_back('#');
      break;
    default:
      if (c >= 0x20 && c < 0x7f) o.push_back(c);
      else fmt::format_to(std::back_inserter(o), "\\x{:02X}", c);
    }
  }
  o.push_back('"');
}
inline void ruby_inspect(out_t &o, const char *s) { ruby_inspect(o, s, std::strlen(s)); }

// Ruby `"0x%016x" % v`.
inline void ruby_hex(out_t &o, uint64_t v) { fmt::format_to(std::back_inserter(o), "0x{:016x}", v); }

// Ruby `Time.at(0, ns, :nsec).strftime('%H:%M:%S.%9L')` (local time).
inline void ruby_time(out_t &o, int64_t ns) {
  int64_t s = ns / 1000000000, f = ns % 1000000000;
  if (f < 0) { f += 1000000000; s -= 1; }
  time_t t = s;
  struct tm tm;
  localtime_r(&t, &tm);
  fmt::format_to(std::back_inserter(o), "{:02}:{:02}:{:02}.{:09}", tm.tm_hour, tm.tm_min, tm.tm_sec, f);
}

} // namespace thapi_pretty
