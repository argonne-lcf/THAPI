#!/usr/bin/env ruby
# Usage: gen_cxx_pretty.rb BTX_MODEL.yaml FFI_LIBRARY.rb OUT_CALLBACKS.cpp OUT_FORMATTERS.hpp
#
# OUT_FORMATTERS.hpp: an fmt::formatter for each C struct and enum the events of
# BTX_MODEL.yaml carry, printing them as the Ruby pretty printer does, plus
# static_asserts that the C layout is the one the printer reads.
# OUT_CALLBACKS.cpp: the callbacks of a metababel FILTER that formats each event
# of BTX_MODEL.yaml that way and pushes the text as a `thapi_pretty:line` event.
#
# Two steps: a frontend describes the types (Num, Ptr, EnumT, MaskT, StructT,
# ArrT below); the emitter writes C++ from that description. The frontend reads
# the FFI library (e.g. ze_library.rb): enum names, bitmask flags, struct layouts.
require 'yaml'
require 'ffi'

model_path, lib_path, out_cpp, out_hpp = ARGV

# ---- Type description ------------------------------------------------------
Num = Struct.new(:ctype)                                # int8_t ... uint64_t, float, double, bool
Ptr = Struct.new(:unused)                               # printed "0x%016x"
EnumT = Struct.new(:cnames, :native, :pairs)            # pairs: [value, name], one per value
MaskT = Struct.new(:native, :flags, :default)           # flags: [name, mask], in order
StructT = Struct.new(:cname, :size, :members, :special) # members: [name, offset, type]
ArrT = Struct.new(:elem, :length)                       # elem Num('char'): printed as a C string
PTR = Ptr.new

def signed?(ctype) = !ctype.start_with?('u')
def bits(ctype) = ctype[/\d+/].to_i

# ---- Frontend: FFI library -------------------------------------------------
class FFIFrontend
  INT = {
    FFI::Type::INT8 => 'int8_t', FFI::Type::UINT8 => 'uint8_t', FFI::Type::INT16 => 'int16_t',
    FFI::Type::UINT16 => 'uint16_t', FFI::Type::INT32 => 'int32_t', FFI::Type::UINT32 => 'uint32_t',
    FFI::Type::INT64 => 'int64_t', FFI::Type::UINT64 => 'uint64_t', FFI::Type::LONG => 'int64_t',
    FFI::Type::ULONG => 'uint64_t', FFI::Type::LONG_LONG => 'int64_t', FFI::Type::ULONG_LONG => 'uint64_t'
  }.freeze

  def initialize(lib_path)
    $LOAD_PATH.unshift(File.dirname(File.expand_path(lib_path)))
    require File.expand_path(lib_path)
    @struct_cname = {}
    @enum_cnames = Hash.new { |h, k| h[k] = [] }
    ObjectSpace.each_object(Module) do |m|
      td = m.instance_variable_defined?(:@ffi_typedefs) && m.instance_variable_get(:@ffi_typedefs)
      next unless td

      td.each do |k, t|
        if t.is_a?(FFI::StructByValue) then @struct_cname[t.struct_class] = k.to_s
        elsif t.is_a?(FFI::Type::Mapped) then @enum_cnames[t.converter] |= [k.to_s]
        end
      end
    end
    @memo = {}
  end

  # The description of the be_class of a model member.
  def be_class(name)
    obj = Object.const_get(name)
    case obj
    when FFI::Bitmask then mask(obj, INT.fetch(obj.native_type))
    when FFI::Enum then enum(obj, INT.fetch(obj.native_type))
    when Class then struct(obj)
    else raise "unsupported be_class #{name}"
    end
  end

  def type(t)
    case t
    when FFI::Type::Mapped
      c = t.converter
      c.is_a?(FFI::Bitmask) ? mask(c, INT.fetch(t.native_type)) : enum(c, INT.fetch(t.native_type))
    when FFI::StructByValue then struct(t.struct_class)
    when FFI::Type::Array # FFI prints a :char (int8) array as a string, other arrays as [ a, b ]
      ArrT.new(t.elem_type == FFI::Type::INT8 ? Num.new('char') : type(t.elem_type), t.length)
    when FFI::Type::Builtin
      if t == FFI::Type::POINTER then PTR
      elsif t == FFI::Type::FLOAT32 then Num.new('float')
      elsif t == FFI::Type::FLOAT64 then Num.new('double')
      elsif t == FFI::Type::BOOL then Num.new('bool')
      else Num.new(INT.fetch(t))
      end
    else raise "unsupported FFI type #{t.inspect}"
    end
  end

  # FFI `Enum#from_native`: `@vk_map[val] || val`; @vk_map is the inverted
  # symbol map, so the last name of an aliased value wins.
  def enum(e, native)
    @memo[[e, native]] ||= EnumT.new(@enum_cnames[e], native, e.symbol_map.invert.map { |v, k| [v, k.to_s] })
  end

  # FFI `Bitmask#from_native`, with the ZEBitmask `@default` name for 0.
  def mask(b, native)
    @memo[[b, native]] ||= MaskT.new(native, b.symbol_map.map { |k, v| [k.to_s, v] },
                                     b.instance_variable_get(:@default)&.to_s)
  end

  def struct(klass)
    return @memo[klass] if @memo.key?(klass)
    raise "#{klass}: own to_s is not supported" if klass.instance_methods(false).include?(:to_s)

    mods = klass.ancestors.take_while { |a| a != klass }.map { |m| m.name.split('::').last }
    # Handle#to_s on a struct without `data` raises in Ruby (THAPI #559): print it as a plain struct.
    special = case mods
              when [] then nil
              when ['Handle'] then klass.layout.members.include?(:data) ? :handle : nil
              when ['UUID'] then :uuid
              when ['KUUID'] then :kuuid
              when ['Version'] then :version
              else raise "#{klass}: unsupported to_s modules #{mods}"
              end
    s = @memo[klass] = StructT.new(@struct_cname.fetch(klass), klass.size, [], special)
    s.members = klass.layout.fields.map { |f| [f.name.to_s, f.offset, type(f.type)] }
    s
  end
end

# ---- Emitter ---------------------------------------------------------------
def c_str(s) = '"' + s.gsub('\\', '\\\\\\\\').gsub('"', '\"') + '"'

class Emitter
  attr_reader :protos, :defs, :formatters, :asserts

  def initialize(ns)
    @ns = ns
    @fn = {}
    @protos = []
    @defs = []
    @formatters = []
    @asserts = []
  end

  # The name of the function that prints `t`; generated on first use.
  def fn(t)
    return @fn[t.object_id] if @fn.key?(t.object_id)

    name = "p#{@fn.size}"
    @fn[t.object_id] = name
    case t
    when EnumT then enum(name, t)
    when MaskT then mask(name, t)
    when StructT then struct(name, t)
    else raise "no function for #{t}"
    end
    name
  end

  def enum(n, t)
    @protos << "inline void #{n}(it_t o, int64_t v);"
    cases = t.pairs.map { |v, k| "  case #{v}LL: put(o, #{c_str(k)}); return;" }
    @defs << "inline void #{n}(it_t o, int64_t v) {\n  switch (v) {\n#{cases.join("\n")}\n  default: dec(o, v);\n  }\n}\n"
    t.cnames.each do |c|
      @formatters << "template <> struct fmt::formatter<#{c}> : thapi_pretty::no_spec {\n  " \
                     "auto format(#{c} v, format_context &ctx) const { auto o = ctx.out(); " \
                     "#{@ns}::#{n}(o, (int64_t)(#{t.native})v); return o; }\n};"
    end
  end

  def mask(n, t)
    @protos << "inline void #{n}(it_t o, int64_t val);"
    flags = t.flags.map { |k, v| "  if ((uint64_t)val & #{v}ULL) { sep(o, first); put(o, #{c_str(k)}); acc |= #{v}ULL; }" }
    wrap = signed?(t.native) && bits(t.native) < 64 ? "  val = (int64_t)((uint64_t)val & ((1ULL << #{bits(t.native)}) - 1));\n" : ''
    dflt = t.default ? "  if (val == 0) { put(o, #{c_str("[ #{t.default} ]")}); return; }\n" : ''
    @defs << "inline void #{n}(it_t o, int64_t val) {\n#{dflt}  put(o, \"[ \");\n  bool first = true;\n  uint64_t acc = 0;\n" \
             "#{flags.join("\n")}\n#{wrap}  int64_t rem = val ^ (int64_t)acc;\n  if (rem) { sep(o, first); dec(o, rem); }\n" \
             "  put(o, \" ]\");\n}\n"
  end

  # Statements printing the value `e` (a C++ lvalue) of type `t`.
  def value(t, e, depth = 0)
    case t
    when Num
      case t.ctype
      when 'float', 'double' then "ruby_float(o, (double)(#{e}));"
      when 'bool' then "put(o, (#{e}) ? \"true\" : \"false\");"
      else "dec(o, (#{t.ctype})(#{e}));"
      end
    when Ptr then "ruby_hex(o, (uint64_t)(uintptr_t)(#{e}));"
    when EnumT, MaskT then "#{fn(t)}(o, (int64_t)(#{t.native})(#{e}));"
    when StructT then "#{fn(t)}(o, #{e});"
    when ArrT
      if t.elem == Num.new('char') then "char_array(o, (const char *)(#{e}), #{t.length});"
      else # Struct::InlineArray#to_s
        i = "i#{depth}"
        "put(o, \"[ \"); for (int #{i} = 0; #{i} < #{t.length}; #{i}++) { if (#{i}) put(o, \", \"); " \
          "#{value(t.elem, "(#{e})[#{i}]", depth + 1)} } put(o, \" ]\");"
      end
    end
  end

  def member(t, name) = t.members.find { |m, _, _| m == name } || raise("#{t.cname}: no member #{name}")

  def struct(n, t)
    c = t.cname
    @protos << "inline void #{n}(it_t o, const #{c} &s);"
    raw = 'const unsigned char *b = (const unsigned char *)&s;'
    body =
      case t.special
      when :handle # "\x%02x" for every byte of `data`
        _, off, a = member(t, 'data')
        "#{raw} put(o, \"{ data: \\\"\"); for (int i = 0; i < #{a.length}; i++) " \
          "fmt::format_to(o, \"\\\\x{:02x}\", b[#{off} + i]); put(o, \"\\\" }\");"
      when :uuid
        "#{raw} put(o, \"{ id: \"); uuid(o, b + #{member(t, 'id')[1]}); put(o, \" }\");"
      when :kuuid
        "#{raw} put(o, \"{ kid: \"); uuid(o, b + #{member(t, 'kid')[1]}); " \
          "put(o, \", mid: \"); uuid(o, b + #{member(t, 'mid')[1]}); put(o, \" }\");"
      when :version # a C enum in the headers, a { minor, major } struct in FFI
        _, mo, mt = member(t, 'major')
        _, no, nt = member(t, 'minor')
        "#{raw} #{mt.ctype} major; #{nt.ctype} minor; std::memcpy(&major, b + #{mo}, sizeof major); " \
          "std::memcpy(&minor, b + #{no}, sizeof minor); dec(o, major); put(o, \".\"); dec(o, minor);"
      else
        parts = t.members.map { |m, _, mt| "put(o, #{c_str("#{m}: ")}); #{value(mt, "s.#{m}")}" }
        "put(o, \"{ \"); #{parts.join(' put(o, ", "); ')} put(o, \" }\");"
      end
    @defs << "inline void #{n}(it_t o, const #{c} &s) { #{body} }\n"
    @formatters << "template <> struct fmt::formatter<#{c}> : thapi_pretty::no_spec {\n  " \
                   "auto format(const #{c} &s, format_context &ctx) const { auto o = ctx.out(); #{@ns}::#{n}(o, s); return o; }\n};"
    @asserts << "static_assert(sizeof(#{c}) == #{t.size}, \"#{c}: size differs from the FFI layout\");"
    return if t.special == :version

    t.members.each do |m, off, _|
      @asserts << "static_assert(offsetof(#{c}, #{m}) == #{off}, \"#{c}.#{m}: offset differs from the FFI layout\");"
    end
  end

  # A Version struct built from an integer: FFI `from_native(i)` then `to_s`.
  def version_from_int(t)
    mt = member(t, 'major')[2]
    nt = member(t, 'minor')[2]
    "dec(o, (#{mt.ctype})(v >> 16)); put(o, \".\"); dec(o, (#{nt.ctype})(v & 0xffff));"
  end
end

# ---- Callbacks -------------------------------------------------------------
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
  be = m.dig(:metadata, :be_class)
  t = be && fe.be_class(be)
  body =
    case fc[:type]
    when 'integer_signed', 'integer_unsigned'
      v = int_expr(fc, a)
      if t.is_a?(EnumT) || t.is_a?(MaskT) then "#{em.fn(t)}(o, (int64_t)#{v});"
      elsif t.is_a?(StructT) && t.special == :version then "{ int64_t v = (int64_t)#{v}; #{em.version_from_int(t)} }"
      elsif t then raise "#{name}: unsupported be_class #{be}"
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
fe = FFIFrontend.new(lib_path)
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

src = "#{File.basename(model_path)} and #{File.basename(lib_path)}"
File.write(out_hpp, <<~HPP)
  // Generated by #{File.basename(__FILE__)} from #{src}.
  // fmt::formatter for the #{backend} structs and enums, printed as the Ruby
  // pretty printer prints them. Include after the #{backend} headers.
  #pragma once

  #include "thapi_pretty.hpp"

  #include <cstddef>

  namespace #{ns} {
  using thapi_pretty::it_t;
  using thapi_pretty::put;
  using thapi_pretty::dec;
  using thapi_pretty::ruby_float;
  using thapi_pretty::ruby_hex;
  using thapi_pretty::char_array;
  using thapi_pretty::uuid;
  inline void sep(it_t o, bool &first) { if (!first) put(o, ", "); first = false; }

  #{em.protos.join("\n")}

  #{em.defs.join("\n")}
  } // namespace #{ns}

  #{em.formatters.join("\n")}

  #{em.asserts.join("\n")}
HPP

File.write(out_cpp, <<~CPP)
  // Generated by #{File.basename(__FILE__)} from #{src}.
  // C `_Bool` (some cast_types) is C++ `bool`, with the same ABI.
  #define _Bool bool
  #include <metababel/metababel.h>

  #include "#{File.basename(out_hpp)}"

  #include <type_traits>

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
    std::memset(&s, 0, sizeof s);
    std::memcpy(&s, p, len < sizeof s ? len : sizeof s);
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
