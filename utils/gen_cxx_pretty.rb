#!/usr/bin/env ruby
# Usage: gen_cxx_pretty.rb BTX_MODEL.yaml FFI_LIBRARY.rb > btx_pretty_callbacks.cpp
#
# Write the callbacks of a metababel FILTER that formats each event of
# BTX_MODEL.yaml as the Ruby pretty printer (babeltrace_thapi `trace`) does,
# and pushes the text downstream as a `thapi_pretty:line` event.
#
# The FFI library (e.g. ze_library.rb) is read at generation time only: it gives
# the enum names, the bitmask flags and the struct layouts the Ruby printer uses.
require 'yaml'

model_path, lib_path = ARGV
$LOAD_PATH.unshift(File.dirname(File.expand_path(lib_path)))
require File.expand_path(lib_path)

def length_field?(name)
  name.start_with?('_') && name.end_with?('_length')
end

def c_str(s)
  '"' + s.gsub('\\', '\\\\\\\\').gsub('"', '\"') + '"'
end

INT_TYPES = {
  FFI::Type::INT8 => 'int8_t', FFI::Type::UINT8 => 'uint8_t',
  FFI::Type::INT16 => 'int16_t', FFI::Type::UINT16 => 'uint16_t',
  FFI::Type::INT32 => 'int32_t', FFI::Type::UINT32 => 'uint32_t',
  FFI::Type::INT64 => 'int64_t', FFI::Type::UINT64 => 'uint64_t',
  FFI::Type::LONG => 'int64_t', FFI::Type::ULONG => 'uint64_t',
  FFI::Type::LONG_LONG => 'int64_t', FFI::Type::ULONG_LONG => 'uint64_t'
}.freeze

# The C++ functions that print FFI values, generated once per FFI type.
class Printers
  attr_reader :protos, :defs

  def initialize
    @fn = {}
    @protos = []
    @defs = []
  end

  # `yield name` writes the definition; dependencies may be generated inside.
  def memo(key, proto)
    return @fn[key] if @fn.key?(key)

    name = "p#{@fn.size}"
    @fn[key] = name
    @protos << format(proto, name) + ';'
    @defs << yield(name)
    name
  end

  # FFI `Enum#from_native`: `@vk_map[val] || val`, where @vk_map is the
  # inverted symbol map (the last name of an aliased value wins).
  def enum(e)
    memo([:enum, e.object_id], 'static void %s(out_t &o, int64_t v)') do |n|
      cases = e.symbol_map.invert.map { |v, k| "  case #{v}LL: return put(o, #{c_str(k.to_s)});" }
      <<~CPP
        static void #{n}(out_t &o, int64_t v) {
          switch (v) {
        #{cases.join("\n")}
          default: dec(o, v);
          }
        }
      CPP
    end
  end

  # FFI `Bitmask#from_native` (every flag that overlaps the value, then
  # `val ^ flags` if not zero), with the ZEBitmask `@default` name for 0.
  def bitmask(b)
    memo([:bitmask, b.object_id], 'static void %s(out_t &o, int64_t val)') do |n|
      default = b.instance_variable_get(:@default)
      signed = b.instance_variable_get(:@signed)
      bits = b.native_type.size * 8
      flags = b.symbol_map.map do |k, v|
        "  if ((uint64_t)val & #{v}ULL) { sep(o, first); put(o, #{c_str(k.to_s)}); acc |= #{v}ULL; }"
      end
      <<~CPP
        static void #{n}(out_t &o, int64_t val) {
        #{default ? "  if (val == 0) return put(o, #{c_str("[ #{default} ]")});" : ''}
          put(o, "[ ");
          bool first = true;
          uint64_t acc = 0;
        #{flags.join("\n")}
        #{signed && bits < 64 ? "  val = (int64_t)((uint64_t)val & ((1ULL << #{bits}) - 1));" : ''}
          int64_t rem = val ^ (int64_t)acc;
          if (rem) { sep(o, first); dec(o, rem); }
          put(o, " ]");
        }
      CPP
    end
  end

  # Statements that print the value of FFI type `t` stored at `ptr`.
  def value(t, ptr)
    case t
    when FFI::Type::Mapped
      conv = t.converter
      ct = INT_TYPES.fetch(t.native_type)
      f = conv.is_a?(FFI::Bitmask) ? bitmask(conv) : enum(conv)
      "#{f}(o, (int64_t)rd<#{ct}>(#{ptr}));"
    when FFI::StructByValue
      "#{struct(t.struct_class)}(o, #{ptr});"
    when FFI::Type::Array
      et = t.elem_type
      if et == FFI::Type::INT8 # FFI CharArray#to_s: the bytes up to the first NUL
        "put_cstr(o, (const char *)(#{ptr}), #{t.length});"
      else # Struct::InlineArray#to_s
        elems = (0...t.length).map { |i| value(et, "#{ptr} + #{i * et.size}") }
        "put(o, \"[ \"); #{elems.join(' put(o, ", "); ')} put(o, \" ]\");"
      end
    when FFI::Type::Builtin
      if t == FFI::Type::POINTER then "ruby_hex(o, rd<uint64_t>(#{ptr}));"
      elsif t == FFI::Type::FLOAT32 then "ruby_float(o, rd<float>(#{ptr}));"
      elsif t == FFI::Type::FLOAT64 then "ruby_float(o, rd<double>(#{ptr}));"
      elsif t == FFI::Type::BOOL then "put(o, rd<uint8_t>(#{ptr}) ? \"true\" : \"false\");"
      else "dec(o, rd<#{INT_TYPES.fetch(t)}>(#{ptr}));"
      end
    else
      raise "unsupported FFI type #{t.inspect}"
    end
  end

  def field(klass, name)
    klass.layout.fields.find { |f| f.name == name } or raise "#{klass}: no field #{name}"
  end

  # `#{struct}` in Ruby: the to_s of the class (generic ZEStruct, or a prepended module).
  def struct(klass)
    memo([:struct, klass], 'static void %s(out_t &o, const unsigned char *p)') do |n|
      mods = klass.ancestors.take_while { |a| a != klass }.map { |m| m.name.split('::').last }
      # Handle#to_s on a struct without `data` raises in Ruby (THAPI #559): print it as a plain struct.
      mods = [] if mods == ['Handle'] && !klass.layout.members.include?(:data)
      raise "#{klass}: own to_s is not supported" if klass.instance_methods(false).include?(:to_s)

      body =
        case mods
        when ['Handle']
          f = field(klass, :data)
          "put(o, \"{ data: \\\"\"); for (int i = 0; i < #{f.type.length}; i++) " \
            "fmt::format_to(std::back_inserter(o), \"\\\\x{:02x}\", p[#{f.offset} + i]); put(o, \"\\\" }\");"
        when ['UUID']
          "put(o, \"{ id: \"); uuid(o, p + #{field(klass, :id).offset}); put(o, \" }\");"
        when ['KUUID']
          "put(o, \"{ kid: \"); uuid(o, p + #{field(klass, :kid).offset}); " \
            "put(o, \", mid: \"); uuid(o, p + #{field(klass, :mid).offset}); put(o, \" }\");"
        when ['Version']
          "#{value(field(klass, :major).type, "p + #{field(klass, :major).offset}")} put(o, \".\"); " \
            "#{value(field(klass, :minor).type, "p + #{field(klass, :minor).offset}")}"
        when []
          parts = klass.layout.fields.map do |f|
            "put(o, #{c_str("#{f.name}: ")}); #{value(f.type, "p + #{f.offset}")}"
          end
          "put(o, \"{ \"); #{parts.join(' put(o, ", "); ')} put(o, \" }\");"
        else
          raise "#{klass}: unsupported to_s modules #{mods}"
        end
      "static void #{n}(out_t &o, const unsigned char *p) { #{body} }\n"
    end
  end

  # A Version struct built from an integer: `from_native(i)` then `to_s`.
  def version_from_int(klass)
    memo([:version, klass], 'static void %s(out_t &o, int64_t i)') do |n|
      "static void #{n}(out_t &o, int64_t i) { dec(o, (uint16_t)(i >> 16)); put(o, \".\"); dec(o, (uint16_t)(i & 0xffff)); }\n"
    end
  end
end

P = Printers.new

# An integer of the trace field `fc`, from the C argument `a`: truncated to the
# field's width and signedness, as the Ruby printer read it.
def int_expr(fc, a)
  bits = fc[:field_value_range] || 64
  if fc[:type] == 'integer_signed'
    "sext(u64(#{a}), #{bits})"
  else
    "(u64(#{a}) & #{bits == 64 ? '~0ULL' : "((1ULL << #{bits}) - 1)"})"
  end
end

# Statements that append the rendering of one payload member (Ruby `render_field`).
def render(m, args)
  name = m[:name]
  fc = m[:field_class]
  a = args[name]
  be = m.dig(:metadata, :be_class)
  be = Object.const_get(be) if be
  head = "put(o, #{c_str("#{name}: ")});"
  body =
    case fc[:type]
    when 'integer_signed', 'integer_unsigned'
      v = int_expr(fc, a)
      if be.is_a?(FFI::Bitmask) then "#{P.bitmask(be)}(o, (int64_t)#{v});"
      elsif be.is_a?(FFI::Enum) then "#{P.enum(be)}(o, (int64_t)#{v});"
      elsif be.is_a?(Class) then "#{P.version_from_int(be)}(o, (int64_t)#{v});"
      elsif be then raise "#{name}: unsupported be_class #{be}"
      elsif fc[:preferred_display_base] == 16
        raise "#{name}: signed hexadecimal is not supported" if fc[:type] == 'integer_signed'

        "ruby_hex(o, #{v});"
      else "dec(o, #{fc[:type] == 'integer_signed' ? '(int64_t)' : '(uint64_t)'}#{v});"
      end
    when 'double', 'single'
      "ruby_float(o, (double)#{a});"
    when 'string'
      raise "#{name}: string with be_class is not supported" if be

      "ruby_inspect(o, #{a} ? (const char *)#{a} : \"\");"
    when 'blob_dynamic', 'blob_static'
      ptr, len =
        if fc[:type] == 'blob_static' then ["(const unsigned char *)&#{a}", "sizeof(#{a})"]
        else ["(const unsigned char *)#{a}", args.fetch("_#{name}_length")]
        end
      if be.nil?
        "ruby_inspect(o, (const char *)#{ptr}, #{len});"
      elsif name.end_with?('_vals') # an array of structs, split on the struct size
        "struct_vals(o, #{ptr}, #{len}, #{be.size}, #{P.struct(be)});"
      else
        "if (#{len} > 0) struct_one(o, #{ptr}, #{len}, #{be.size}, #{P.struct(be)});"
      end
    when 'array_dynamic', 'array_static'
      e = fc[:element_field_class]
      raise "#{name}: unsupported array element #{e[:type]}" unless %w[integer_signed integer_unsigned].include?(e[:type])

      n = fc[:type] == 'array_static' ? fc[:length] : args.fetch("_#{name}_length")
      v = int_expr(e, "#{a}[i]")
      if e[:preferred_display_base] == 16 # `[ #{...collect { "0x%016x" }.join(", ")} ]`
        "put(o, \"[ \"); for (size_t i = 0; i < (size_t)#{n}; i++) { if (i) put(o, \", \"); ruby_hex(o, #{v}); } put(o, \" ]\");"
      else # Array#to_s
        cast = e[:type] == 'integer_signed' ? '(int64_t)' : '(uint64_t)'
        "put(o, \"[\"); for (size_t i = 0; i < (size_t)#{n}; i++) { if (i) put(o, \", \"); dec(o, #{cast}#{v}); } put(o, \"]\");"
      end
    else
      raise "#{name}: unsupported field type #{fc[:type]}"
    end
  "#{head} #{body}"
end

model = YAML.load_file(model_path)
callbacks = []
registrations = []
model[:stream_classes].each do |sc|
  ctx = (sc.dig(:event_common_context_field_class, :members) || [])
  sc[:event_classes].each_with_index do |e, ei|
    members = e[:payload_field_class]&.[](:members).to_a
    args = members.each_with_index.to_h { |m, i| [m[:name], "a#{i}"] }
    params = ['void *btx_handle', 'void *usr_data', 'int64_t ts', 'const char *hostname'] +
             ctx.map { |c| "#{c[:field_class][:cast_type]} #{c[:name]}" } +
             members.each_with_index.map { |m, i| "#{m[:field_class][:cast_type]} a#{i}" }
    ctx_code = ctx.each_with_index.map do |c, i|
      "put(o, #{c_str("#{i.zero? ? ' - ' : ', '}#{c[:name]}: ")}); dec(o, #{c[:name]});"
    end
    fields = members.reject { |m| length_field?(m[:name]) }.map { |m| render(m, args) }
    cb = "cb_#{ei}"
    callbacks << <<~CPP
      static void #{cb}(#{params.join(', ')}) {
        auto *d = static_cast<data_t *>(usr_data);
        out_t &o = d->out;
        o.clear();
        ruby_time(o, ts);
        if (d->context) { put(o, " - "); put(o, hostname ? hostname : ""); #{ctx_code.join(' ')} }
        put(o, #{c_str(" - #{e[:name]}: { ")});
        #{fields.join("\n  put(o, \", \");\n  ")}
        put(o, " }");
        o.push_back('\\0');
        btx_push_message_thapi_pretty_line(btx_handle, ts, o.data());
      }
    CPP
    registrations << "  btx_register_callbacks_#{e[:name].gsub(/[^0-9A-Za-z-]/, '_')}(btx_handle, &#{cb});"
  end
end

puts <<~CPP
  // Generated by #{File.basename(__FILE__)} from #{File.basename(model_path)} and #{File.basename(lib_path)}.
  // C `_Bool` (some cast_types) is C++ `bool`, with the same ABI.
  #define _Bool bool
  #include <metababel/metababel.h>

  #include "thapi_pretty.hpp"

  #include <type_traits>
  #include <vector>

  using namespace thapi_pretty;

  struct data_t {
    out_t out;
    bool context = false;
  };

  template <class T> static inline T rd(const unsigned char *p) {
    T v;
    std::memcpy(&v, p, sizeof v);
    return v;
  }
  template <class T> static inline uint64_t u64(T v) {
    if constexpr (std::is_pointer_v<T>) return (uint64_t)(uintptr_t)v;
    else return (uint64_t)v;
  }
  static inline int64_t sext(uint64_t v, int bits) {
    return bits == 64 ? (int64_t)v : (int64_t)(v << (64 - bits)) >> (64 - bits);
  }
  template <class T> static inline void dec(out_t &o, T v) { fmt::format_to(std::back_inserter(o), "{}", v); }
  static inline void dec(out_t &o, int8_t v) { dec(o, (int)v); }
  static inline void dec(out_t &o, uint8_t v) { dec(o, (unsigned)v); }
  static inline void sep(out_t &o, bool &first) { if (!first) put(o, ", "); first = false; }
  static inline void put_cstr(out_t &o, const char *s, size_t max) { put(o, std::string_view(s, strnlen(s, max))); }
  static inline void uuid(out_t &o, const unsigned char *a) { // bytes printed back to front
    fmt::format_to(std::back_inserter(o), "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                   a[15], a[14], a[13], a[12], a[11], a[10], a[9], a[8], a[7], a[6], a[5], a[4], a[3], a[2], a[1], a[0]);
  }
  // Ruby reads a packed struct from `FFI::MemoryPointer.from_string(bytes)`: the
  // bytes plus a NUL; we zero-pad to the struct size.
  static void struct_one(out_t &o, const unsigned char *p, size_t len, size_t sz,
                         void (*f)(out_t &, const unsigned char *)) {
    std::vector<unsigned char> b(sz, 0);
    std::memcpy(b.data(), p, len < sz ? len : sz);
    f(o, b.data());
  }
  static void struct_vals(out_t &o, const unsigned char *p, size_t len, size_t sz,
                          void (*f)(out_t &, const unsigned char *)) {
    size_t n = len > 0 ? (len + 1) / sz : 0;
    std::vector<unsigned char> b(n * sz + 1, 0);
    std::memcpy(b.data(), p, len < n * sz ? len : n * sz);
    put(o, "[ ");
    for (size_t i = 0; i < n; i++) { if (i) put(o, ", "); f(o, b.data() + i * sz); }
    put(o, " ]");
  }

CPP
puts P.protos
puts
puts P.defs
puts callbacks
puts <<~CPP

  static void btx_initialize_component(void **usr_data) { *usr_data = new data_t; }
  static void btx_finalize_component(void *usr_data) { delete static_cast<data_t *>(usr_data); }
  static void btx_read_params(void *usr_data, btx_params_t *usr_params) {
    static_cast<data_t *>(usr_data)->context = usr_params->context;
  }

  void btx_register_usr_callbacks(void *btx_handle) {
    btx_register_callbacks_initialize_component(btx_handle, &btx_initialize_component);
    btx_register_callbacks_finalize_component(btx_handle, &btx_finalize_component);
    btx_register_callbacks_read_params(btx_handle, &btx_read_params);
  #{registrations.join("\n")}
  }
CPP
