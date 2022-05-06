// Autogenerated by the ProtoZero compiler plugin. DO NOT EDIT.

#ifndef PERFETTO_PROTOS_PROTOS_PERFETTO_TRACE_TRACK_EVENT_LOG_MESSAGE_PROTO_H_
#define PERFETTO_PROTOS_PROTOS_PERFETTO_TRACE_TRACK_EVENT_LOG_MESSAGE_PROTO_H_

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


class LogMessageBody_Decoder : public ::protozero::TypedProtoDecoder</*MAX_FIELD_ID=*/2, /*HAS_NONPACKED_REPEATED_FIELDS=*/false> {
 public:
  LogMessageBody_Decoder(const uint8_t* data, size_t len) : TypedProtoDecoder(data, len) {}
  explicit LogMessageBody_Decoder(const std::string& raw) : TypedProtoDecoder(reinterpret_cast<const uint8_t*>(raw.data()), raw.size()) {}
  explicit LogMessageBody_Decoder(const ::protozero::ConstBytes& raw) : TypedProtoDecoder(raw.data, raw.size) {}
  bool has_iid() const { return at<1>().valid(); }
  uint64_t iid() const { return at<1>().as_uint64(); }
  bool has_body() const { return at<2>().valid(); }
  ::protozero::ConstChars body() const { return at<2>().as_string(); }
};

class LogMessageBody : public ::protozero::Message {
 public:
  using Decoder = LogMessageBody_Decoder;
  enum : int32_t {
    kIidFieldNumber = 1,
    kBodyFieldNumber = 2,
  };
  static constexpr const char* GetName() { return ".perfetto.protos.LogMessageBody"; }


  using FieldMetadata_Iid =
    ::protozero::proto_utils::FieldMetadata<
      1,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint64,
      uint64_t,
      LogMessageBody>;

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

  using FieldMetadata_Body =
    ::protozero::proto_utils::FieldMetadata<
      2,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kString,
      std::string,
      LogMessageBody>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Body kBody() { return {}; }
  void set_body(const char* data, size_t size) {
    AppendBytes(FieldMetadata_Body::kFieldId, data, size);
  }
  void set_body(std::string value) {
    static constexpr uint32_t field_id = FieldMetadata_Body::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kString>
        ::Append(*this, field_id, value);
  }
};

class LogMessage_Decoder : public ::protozero::TypedProtoDecoder</*MAX_FIELD_ID=*/2, /*HAS_NONPACKED_REPEATED_FIELDS=*/false> {
 public:
  LogMessage_Decoder(const uint8_t* data, size_t len) : TypedProtoDecoder(data, len) {}
  explicit LogMessage_Decoder(const std::string& raw) : TypedProtoDecoder(reinterpret_cast<const uint8_t*>(raw.data()), raw.size()) {}
  explicit LogMessage_Decoder(const ::protozero::ConstBytes& raw) : TypedProtoDecoder(raw.data, raw.size) {}
  bool has_source_location_iid() const { return at<1>().valid(); }
  uint64_t source_location_iid() const { return at<1>().as_uint64(); }
  bool has_body_iid() const { return at<2>().valid(); }
  uint64_t body_iid() const { return at<2>().as_uint64(); }
};

class LogMessage : public ::protozero::Message {
 public:
  using Decoder = LogMessage_Decoder;
  enum : int32_t {
    kSourceLocationIidFieldNumber = 1,
    kBodyIidFieldNumber = 2,
  };
  static constexpr const char* GetName() { return ".perfetto.protos.LogMessage"; }


  using FieldMetadata_SourceLocationIid =
    ::protozero::proto_utils::FieldMetadata<
      1,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint64,
      uint64_t,
      LogMessage>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_SourceLocationIid kSourceLocationIid() { return {}; }
  void set_source_location_iid(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_SourceLocationIid::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_BodyIid =
    ::protozero::proto_utils::FieldMetadata<
      2,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint64,
      uint64_t,
      LogMessage>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_BodyIid kBodyIid() { return {}; }
  void set_body_iid(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_BodyIid::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint64>
        ::Append(*this, field_id, value);
  }
};

} // Namespace.
} // Namespace.
} // Namespace.
#endif  // Include guard.
