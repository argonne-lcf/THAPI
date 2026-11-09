#!/usr/bin/env ruby
# Usage: SRC_DIR=... gen_cxx_pretty.rb LIBRARY_BASE.rb BTX_MODEL.yaml OUT_CALLBACKS.cpp OUT_FORMATTERS.hpp
#   e.g. SRC_DIR=$(srcdir) gen_cxx_pretty.rb $(srcdir)/gen_ze_library_base.rb btx_ze_model.yaml \
#          btx_zepretty_callbacks.cpp btx_zepretty_formatters.hpp
#
# OUT_FORMATTERS.hpp: an fmt::formatter for each C struct and enum the events of
# BTX_MODEL.yaml carry, printing them as the Ruby pretty printer does.
# OUT_CALLBACKS.cpp: the callbacks of a metababel FILTER that formats each event
# of BTX_MODEL.yaml that way and pushes the text as a `thapi_pretty:line` event.
#
# LIBRARY_BASE.rb defines the backend's API model (API, NAMING). The C++ code
# prints most values by their C type (overloads of `pv`); the API model says
# only what the C type does not: the order of the struct members, which
# integer members are flags, and which structs print in a special way. Enum
# values are never evaluated here: the generated code uses the enumerator
# names, so the compiler gives the values.
require 'yaml'

base_path, model_path, out_cpp, out_hpp = ARGV
require File.expand_path(base_path)

def c_str(s) = '"' + s.gsub('\\', '\\\\\\\\').gsub('"', '\"') + '"'

# How a backend's Ruby FFI library prints its types (gen_<backend>_library.rb).
RULES = {
  'ZE' => {
    # A `_version_t` enum is a { minor, major } struct printed "major.minor".
    version: ->(name) { name.end_with?('_version_t') },
    # The modules gen_ze_library.rb prepends by class name. Handle#to_s on a
    # struct without `data` raises in Ruby (THAPI #559): print it as a plain struct.
    special: lambda { |name, members|
      k = NAMING.class_name(name)
      if k.match('UUID') then k.match('ZEKernelUUID') ? :kuuid : :uuid
      elsif k.match(/Handle\z/) then members.any? { |m| m.name == 'data' } ? :handle : nil
      elsif name == 'zet_typed_value_t' then :typed_value # the to_s gen_ze_library.rb meant (THAPI #561)
      end
    }
  }
}.freeze
RULE = RULES.fetch(NAMING.module_name)

# ---- What the C++ code needs to know, from the API model --------------------
EnumY = Struct.new(:cname, :names)                         # printed by name
MaskY = Struct.new(:cname, :names)                         # cname: the `_flag_t` enum
VersionY = Struct.new(:cname)                              # "major.minor"
StructY = Struct.new(:cname, :members, :special, :union)   # members: [name, kind]
AnonY = Struct.new(:members, :union)                       # a layout declared inline
# kind: :auto (printed by C type), a MaskY (an integer holding flags), or an AnonY.

class Frontend
  attr_reader :memo

  def initialize
    @memo = {}
    @typedefs = API.types.to_h { |t| [t.name, t] }
  end

  # The description of the be_class of a model member, from its C type.
  def be_type(cast_type)
    type_named(cast_type.sub(/\s*\*\z/, '').strip)
  end

  def type_named(name)
    return @memo[name] if @memo.key?(name)

    if API.bitfield?(name) then mask(name)
    elsif API.struct_names.include?(name) then struct(name)
    elsif (t = @typedefs[name]) && t.type.is_a?(YAMLCAst::Enum)
      @memo[name] = RULE[:version].call(name) ? VersionY.new(name) : EnumY.new(name, enum_names(t.type))
    elsif (t = @typedefs[name]) && t.type.is_a?(YAMLCAst::Union) then struct(name, union: true)
    end
  end

  def enum_names(type) = API.enum(type).members.map(&:name)

  # A flags type: `ze_x_flags_t` (an integer) or `ze_x_flag_t` (the enum).
  def mask(name)
    flag = name.sub(/_flags_t\z/, '_flag_t')
    @memo[name] = @memo[flag] ||= MaskY.new(flag, enum_names(@typedefs.fetch(flag).type))
  end

  def struct(name, union: false)
    decl = @typedefs.fetch(name)
    defn = union ? API.union(decl.type) : API.struct(decl.type)
    s = @memo[name] = StructY.new(name, nil, RULE[:special].call(name, defn.members), union)
    s.members = members(defn, name)
    s
  end

  def members(defn, where)
    defn.members.map do |m|
      raise "#{where}: bit-field member #{m.name} is not supported" if m.num_bits
      raise "#{where}: unnamed member is not supported" unless m.name

      [m.name, kind(m.type, "#{where}.#{m.name}")]
    end
  end

  def kind(type, where)
    case type
    when YAMLCAst::CustomType
      t = type_named(type.name)
      t.is_a?(MaskY) ? t : :auto # structs and enums print by their C type
    when YAMLCAst::Struct, YAMLCAst::Union
      union = type.is_a?(YAMLCAst::Union)
      return :auto if type.name && type_named(type.name)

      AnonY.new(members(union ? API.union(type) : API.struct(type), where), union)
    when YAMLCAst::Array
      e = type.type
      raise "#{where}: array of flags is not supported" if e.is_a?(YAMLCAst::CustomType) && API.bitfield?(e.name)

      type_named(e.name) if e.is_a?(YAMLCAst::CustomType) # make the element printable
      :auto
    else :auto
    end
  end
end

# ---- Emitter ----------------------------------------------------------------
class Emitter
  attr_reader :protos, :defs, :formatters

  def initialize(ns)
    @ns = ns
    @fn = {}
    @protos = []
    @defs = []
    @formatters = []
  end

  def formatter(c, arg, call)
    @formatters << "template <> struct fmt::formatter<#{c}> : thapi_pretty::no_spec {\n  " \
                   "auto format(#{arg}, format_context &ctx) const { auto o = ctx.out(); #{call}; return o; }\n};"
  end

  # The name of the function that prints `t`; generated on first use.
  def fn(t)
    return @fn[t.cname] if @fn.key?(t.cname)

    name = "p#{@fn.size}"
    @fn[t.cname] = name
    case t
    when EnumY then enum(name, t)
    when MaskY then mask(name, t)
    when VersionY then version(name, t)
    when StructY then struct(name, t)
    end
    name
  end

  # FFI `Enum#from_native`: `@vk_map[val] || val`. @vk_map is the inverted
  # symbol map, so the last name of an aliased value wins: test in reverse.
  # A member holding the enum is read as an FFI :int (int32_t).
  def enum(n, t)
    tests = t.names.reverse.map { |k| "  if (v == (int64_t)#{k}) { put(o, #{c_str(k)}); return; }" }
    @protos << "inline void #{n}(it_t o, int64_t v);" << "inline void pv(it_t o, #{t.cname} e);"
    @defs << "inline void #{n}(it_t o, int64_t v) {\n#{tests.join("\n")}\n  dec(o, v);\n}\n" \
             "inline void pv(it_t o, #{t.cname} e) { #{n}(o, (int64_t)(int32_t)e); }\n"
    formatter(t.cname, "#{t.cname} e", "#{@ns}::pv(o, e)")
  end

  # FFI `Bitmask#from_native` on gen_library_base.rb's classification: a
  # positive power of two is a flag, 0 the default name (the last one), anything
  # else a composite FFI does not keep. Every flag that overlaps the value is
  # listed, then `val ^ flags` if not zero. FFI reads the bitmask as int32_t.
  def mask(n, t)
    flags = t.names.map do |k|
      "  if constexpr (flag(#{k})) if ((uint64_t)val & (uint64_t)#{k}) { sep(o, first); put(o, #{c_str(k)}); acc |= (uint64_t)#{k}; }"
    end
    dflt = t.names.reverse.map { |k| "(int64_t)#{k} == 0 ? #{c_str("[ #{k} ]")} : " }.join
    @protos << "inline void #{n}(it_t o, int64_t val);" << "inline void pv(it_t o, #{t.cname} e);"
    @defs << "inline void #{n}(it_t o, int64_t val) {\n  constexpr const char *dflt = #{dflt}nullptr;\n" \
             "  if (dflt && val == 0) { put(o, dflt); return; }\n  put(o, \"[ \");\n  bool first = true;\n  uint64_t acc = 0;\n" \
             "#{flags.join("\n")}\n  val = (int64_t)((uint64_t)val & 0xffffffffULL);\n" \
             "  int64_t rem = val ^ (int64_t)acc;\n  if (rem) { sep(o, first); dec(o, rem); }\n  put(o, \" ]\");\n}\n" \
             "inline void pv(it_t o, #{t.cname} e) { #{n}(o, (int64_t)(int32_t)e); }\n"
  end

  def version(n, t)
    @protos << "inline void #{n}(it_t o, int64_t v);" << "inline void pv(it_t o, #{t.cname} e);"
    @defs << "inline void #{n}(it_t o, int64_t v) { dec(o, (uint16_t)(v >> 16)); put(o, \".\"); dec(o, (uint16_t)(v & 0xffff)); }\n" \
             "inline void pv(it_t o, #{t.cname} e) { #{n}(o, (int64_t)(uint32_t)e); }\n"
    formatter(t.cname, "#{t.cname} e", "#{@ns}::pv(o, e)")
  end

  # Statements printing the members of `s` (an lvalue): "{ a: 1, b: 2 }".
  def members(ms, s)
    parts = ms.map do |m, k|
      v = "#{s}.#{m}"
      code = case k
             when MaskY then "#{fn(k)}(o, (int64_t)(int32_t)#{v});"
             when AnonY then members(k.members, v)
             else "pv(o, #{v});"
             end
      "put(o, #{c_str("#{m}: ")}); #{code}"
    end
    "put(o, \"{ \"); #{parts.join(' put(o, ", "); ')} put(o, \" }\");"
  end

  def struct(n, t)
    c = t.cname
    ms = t.members.to_h
    t.members.each { |_, k| fn(k) if k.is_a?(MaskY) }
    body =
      case t.special
      when :handle then 'put(o, "{ data: \""); for (unsigned char x : s.data) fmt::format_to(o, "\\\\x{:02x}", x); put(o, "\" }");'
      when :uuid then 'put(o, "{ id: "); uuid(o, (const uint8_t *)s.id); put(o, " }");'
      when :kuuid
        'put(o, "{ kid: "); uuid(o, (const uint8_t *)s.kid); put(o, ", mid: "); uuid(o, (const uint8_t *)s.mid); put(o, " }");'
      when :typed_value
        raise "#{c}: unexpected members" unless ms.key?('type') && ms.key?('value')

        'switch (s.type) { case ZET_VALUE_TYPE_UINT32: pv(o, s.value.ui32); break; case ZET_VALUE_TYPE_UINT64: pv(o, s.value.ui64); break; ' \
          'case ZET_VALUE_TYPE_FLOAT32: pv(o, s.value.fp32); break; case ZET_VALUE_TYPE_FLOAT64: pv(o, s.value.fp64); break; ' \
          'case ZET_VALUE_TYPE_BOOL8: pv(o, s.value.b8); break; default: pv(o, s.value.ui64); }'
      else members(t.members, 's')
      end
    @protos << "inline void #{n}(it_t o, const #{c} &s);" << "inline void pv(it_t o, const #{c} &s);"
    @defs << "inline void #{n}(it_t o, const #{c} &s) { #{body} }\ninline void pv(it_t o, const #{c} &s) { #{n}(o, s); }\n"
    formatter(c, "const #{c} &s", "#{@ns}::pv(o, s)")
  end
end

# ---- Callbacks ----------------------------------------------------------------
def length_field?(name) = name.start_with?('_') && name.end_with?('_length')

# An integer of the trace field `fc`, from the C argument `a`: truncated to the
# field's width and signedness, as the Ruby printer read it.
def int_expr(fc, a)
  w = fc[:field_value_range] || 64
  return "sext(u64(#{a}), #{w})" if fc[:type] == 'integer_signed'

  "(u64(#{a}) & #{w == 64 ? '~0ULL' : "((1ULL << #{w}) - 1)"})"
end

# Statements appending one payload member (Ruby `render_field`).
def render(em, fe, m, args)
  name = m[:name]
  fc = m[:field_class]
  a = args[name]
  t = m.dig(:metadata, :be_class) && (fe.be_type(fc[:cast_type]) || raise("#{name}: no C type for #{fc[:cast_type]}"))
  body =
    case fc[:type]
    when 'integer_signed', 'integer_unsigned'
      v = int_expr(fc, a)
      if [EnumY, MaskY, VersionY].include?(t.class) then "#{em.fn(t)}(o, (int64_t)#{v});"
      elsif t then raise "#{name}: unsupported be_class for an integer"
      elsif fc[:preferred_display_base] == 16
        raise "#{name}: signed hexadecimal is not supported" if fc[:type] == 'integer_signed'

        "ruby_hex(o, #{v});"
      else "dec(o, #{fc[:type] == 'integer_signed' ? '(int64_t)' : '(uint64_t)'}#{v});"
      end
    when 'double', 'single' then "ruby_float(o, (double)#{a});"
    when 'string'
      raise "#{name}: string with be_class is not supported" if t

      "ruby_inspect(o, #{a} ? (const char *)#{a} : \"\", #{a} ? std::strlen((const char *)#{a}) : 0);"
    when 'blob_dynamic', 'blob_static'
      ptr, len =
        if fc[:type] == 'blob_static' then ["(const unsigned char *)&#{a}", "sizeof(#{a})"]
        else ["(const unsigned char *)#{a}", args.fetch("_#{name}_length")]
        end
      if t.nil? then "ruby_inspect(o, (const char *)#{ptr}, #{len});"
      elsif !t.is_a?(StructY) then raise "#{name}: a blob of #{t.class} is not supported"
      elsif name.end_with?('_vals') # an array of structs, split on the struct size
        "struct_vals<#{t.cname}>(o, #{ptr}, #{len}, #{em.fn(t)});"
      else "if (#{len} > 0) struct_one<#{t.cname}>(o, #{ptr}, #{len}, #{em.fn(t)});"
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
    else raise "#{name}: unsupported field type #{fc[:type]}"
    end
  "put(o, #{c_str("#{name}: ")}); #{body}"
end

model = YAML.load_file(model_path)
fe = Frontend.new
backend = model[:stream_classes].first[:name].sub(/\Athapi_/, '')
ns = "thapi_pretty::#{backend}"
em = Emitter.new(ns)
callbacks = []
registrations = []
model[:stream_classes].each do |sc|
  ctx = sc.dig(:event_common_context_field_class, :members) || []
  sc[:event_classes].each_with_index do |e, ei|
    members = e[:payload_field_class]&.[](:members).to_a
    args = members.each_with_index.to_h { |m, i| [m[:name], "a#{i}"] }
    params = ['void *btx_handle', 'void *usr_data', 'int64_t ts', 'const char *hostname'] +
             ctx.map { |c| "#{c[:field_class][:cast_type]} #{c[:name]}" } +
             members.each_with_index.map { |m, i| "#{m[:field_class][:cast_type]} a#{i}" }
    ctx_code = ctx.each_with_index.map { |c, i| "put(o, #{c_str("#{i.zero? ? ' - ' : ', '}#{c[:name]}: ")}); dec(o, #{c[:name]});" }
    fields = members.reject { |m| length_field?(m[:name]) }.map { |m| render(em, fe, m, args) }
    cb = "cb_#{ei}"
    callbacks << <<~CPP
      static void #{cb}(#{params.join(', ')}) {
        auto *d = static_cast<data_t *>(usr_data);
        d->out.clear();
        it_t o(d->out);
        ruby_time(o, ts);
        if (d->context) { put(o, " - "); put(o, hostname ? hostname : ""); #{ctx_code.join(' ')} }
        put(o, #{c_str(" - #{e[:name]}: { ")});
        #{fields.join("\n  put(o, \", \");\n  ")}
        put(o, " }");
        d->out.push_back('\\0');
        btx_push_message_thapi_pretty_line(btx_handle, ts, d->out.data());
      }
    CPP
    registrations << "  btx_register_callbacks_#{e[:name].gsub(/[^0-9A-Za-z-]/, '_')}(btx_handle, &#{cb});"
  end
end

# A printer for every type reached, so each member printed by its C type finds its `pv`.
fe.memo.values.compact.uniq.each { |t| em.fn(t) }

src = "#{File.basename(model_path)} and the #{NAMING.module_name} API model"
File.write(out_hpp, <<~HPP)
  // Generated by #{File.basename(__FILE__)} from #{src}.
  // fmt::formatter for the #{backend} structs and enums, printed as the Ruby
  // pretty printer prints them. Include after the #{backend} headers.
  #pragma once

  #include "thapi_pretty.hpp"

  #include <type_traits>

  namespace #{ns} {
  using thapi_pretty::it_t;
  using thapi_pretty::put;
  using thapi_pretty::dec;
  using thapi_pretty::ruby_float;
  using thapi_pretty::ruby_hex;
  using thapi_pretty::char_array;
  using thapi_pretty::uuid;
  inline void sep(it_t o, bool &first) { if (!first) put(o, ", "); first = false; }
  // A positive power of two: a flag (gen_library_base.rb classify_bitfield_members).
  template <class E> constexpr bool flag(E e) { int64_t v = (int64_t)e; return v > 0 && (v & (v - 1)) == 0; }

  #{em.protos.join("\n")}

  // A value printed by its C type, as FFI reads and prints it.
  template <class T> inline void pv(it_t o, const T &v) {
    if constexpr (std::is_same_v<T, bool>) put(o, v ? "true" : "false");
    else if constexpr (std::is_same_v<T, char> || std::is_same_v<T, signed char>) dec(o, (int)v);
    else if constexpr (std::is_same_v<T, unsigned char>) dec(o, (unsigned)v);
    else if constexpr (std::is_floating_point_v<T>) ruby_float(o, (double)v);
    else if constexpr (std::is_pointer_v<T> || std::is_null_pointer_v<T>) ruby_hex(o, (uint64_t)(uintptr_t)v);
    else if constexpr (std::is_integral_v<T>) dec(o, v);
    else static_assert(!sizeof(T), "no printer for this type");
  }
  // An inline array: FFI prints a :char (int8) array as the string up to its first NUL.
  template <class E, size_t N> inline void pv(it_t o, const E (&a)[N]) {
    if constexpr (std::is_same_v<E, char> || std::is_same_v<E, signed char>) char_array(o, (const char *)a, N);
    else { put(o, "[ "); for (size_t i = 0; i < N; i++) { if (i) put(o, ", "); pv(o, a[i]); } put(o, " ]"); }
  }

  #{em.defs.join("\n")}
  } // namespace #{ns}

  #{em.formatters.join("\n")}
HPP

File.write(out_cpp, <<~CPP)
  // Generated by #{File.basename(__FILE__)} from #{src}.
  // C `_Bool` (some cast_types) is C++ `bool`, with the same ABI.
  #define _Bool bool
  #include <metababel/metababel.h>

  #include "#{File.basename(out_hpp)}"

  using namespace #{ns};
  using thapi_pretty::ruby_time;
  using thapi_pretty::ruby_inspect;

  struct data_t {
    thapi_pretty::out_t out;
    bool context = false;
  };

  template <class T> static inline uint64_t u64(T v) {
    if constexpr (std::is_pointer_v<T>) return (uint64_t)(uintptr_t)v;
    else return (uint64_t)v;
  }
  static inline int64_t sext(uint64_t v, int bits) {
    return bits == 64 ? (int64_t)v : (int64_t)(v << (64 - bits)) >> (64 - bits);
  }
  // Ruby reads a packed struct from `FFI::MemoryPointer.from_string(bytes)`: the
  // bytes plus a NUL; we zero-pad to the struct size.
  template <class T> static void struct_one(it_t o, const unsigned char *p, size_t len, void (*f)(it_t, const T &)) {
    T s;
    std::memset((void *)&s, 0, sizeof s);
    std::memcpy((void *)&s, p, len < sizeof s ? len : sizeof s);
    f(o, s);
  }
  template <class T> static void struct_vals(it_t o, const unsigned char *p, size_t len, void (*f)(it_t, const T &)) {
    size_t n = len > 0 ? (len + 1) / sizeof(T) : 0;
    put(o, "[ ");
    for (size_t i = 0; i < n; i++) {
      if (i) put(o, ", ");
      size_t off = i * sizeof(T);
      struct_one<T>(o, p + off, off < len ? len - off : 0, f);
    }
    put(o, " ]");
  }

  #{callbacks.join("\n")}
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
