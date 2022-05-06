// Autogenerated by the ProtoZero compiler plugin. DO NOT EDIT.

#ifndef PERFETTO_PROTOS_PROTOS_PERFETTO_TRACE_TRACK_EVENT_SOURCE_LOCATION_PROTO_H_
#define PERFETTO_PROTOS_PROTOS_PERFETTO_TRACE_TRACK_EVENT_SOURCE_LOCATION_PROTO_H_

#include <stddef.h>
#include <stdint.h>

#include "perfetto/protozero/field_writer.h"
#include "perfetto/protozero/message.h"
#include "perfetto/protozero/packed_repeated_fields.h"
#include "perfetto/protozero/proto_decoder.h"
#include "perfetto/protozero/proto_utils.h"

namespace perfetto {
namespace protos {
namespace pbzero {


class SourceLocation_Decoder : public ::protozero::TypedProtoDecoder</*MAX_FIELD_ID=*/4, /*HAS_NONPACKED_REPEATED_FIELDS=*/false> {
 public:
  SourceLocation_Decoder(const uint8_t* data, size_t len) : TypedProtoDecoder(data, len) {}
  explicit SourceLocation_Decoder(const std::string& raw) : TypedProtoDecoder(reinterpret_cast<const uint8_t*>(raw.data()), raw.size()) {}
  explicit SourceLocation_Decoder(const ::protozero::ConstBytes& raw) : TypedProtoDecoder(raw.data, raw.size) {}
  bool has_iid() const { return at<1>().valid(); }
  uint64_t iid() const { return at<1>().as_uint64(); }
  bool has_file_name() const { return at<2>().valid(); }
  ::protozero::ConstChars file_name() const { return at<2>().as_string(); }
  bool has_function_name() const { return at<3>().valid(); }
  ::protozero::ConstChars function_name() const { return at<3>().as_string(); }
  bool has_line_number() const { return at<4>().valid(); }
  uint32_t line_number() const { return at<4>().as_uint32(); }
};

class SourceLocation : public ::protozero::Message {
 public:
  using Decoder = SourceLocation_Decoder;
  enum : int32_t {
    kIidFieldNumber = 1,
    kFileNameFieldNumber = 2,
    kFunctionNameFieldNumber = 3,
    kLineNumberFieldNumber = 4,
  };
  static constexpr const char* GetName() { return ".perfetto.protos.SourceLocation"; }


  using FieldMetadata_Iid =
    ::protozero::proto_utils::FieldMetadata<
      1,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint64,
      uint64_t,
      SourceLocation>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Iid kIid() { return {}; }
  void set_iid(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_Iid::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_FileName =
    ::protozero::proto_utils::FieldMetadata<
      2,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kString,
      std::string,
      SourceLocation>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_FileName kFileName() { return {}; }
  void set_file_name(const char* data, size_t size) {
    AppendBytes(FieldMetadata_FileName::kFieldId, data, size);
  }
  void set_file_name(std::string value) {
    static constexpr uint32_t field_id = FieldMetadata_FileName::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kString>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_FunctionName =
    ::protozero::proto_utils::FieldMetadata<
      3,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kString,
      std::string,
      SourceLocation>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_FunctionName kFunctionName() { return {}; }
  void set_function_name(const char* data, size_t size) {
    AppendBytes(FieldMetadata_FunctionName::kFieldId, data, size);
  }
  void set_function_name(std::string value) {
    static constexpr uint32_t field_id = FieldMetadata_FunctionName::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kString>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_LineNumber =
    ::protozero::proto_utils::FieldMetadata<
      4,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint32,
      uint32_t,
      SourceLocation>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_LineNumber kLineNumber() { return {}; }
  void set_line_number(uint32_t value) {
    static constexpr uint32_t field_id = FieldMetadata_LineNumber::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint32>
        ::Append(*this, field_id, value);
  }
};

} // Namespace.
} // Namespace.
} // Namespace.
#endif  // Include guard.
