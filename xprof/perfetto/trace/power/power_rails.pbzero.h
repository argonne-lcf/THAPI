// Autogenerated by the ProtoZero compiler plugin. DO NOT EDIT.

#ifndef PERFETTO_PROTOS_PROTOS_PERFETTO_TRACE_POWER_POWER_RAILS_PROTO_H_
#define PERFETTO_PROTOS_PROTOS_PERFETTO_TRACE_POWER_POWER_RAILS_PROTO_H_

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

class PowerRails_EnergyData;
class PowerRails_RailDescriptor;

class PowerRails_Decoder : public ::protozero::TypedProtoDecoder</*MAX_FIELD_ID=*/2, /*HAS_NONPACKED_REPEATED_FIELDS=*/true> {
 public:
  PowerRails_Decoder(const uint8_t* data, size_t len) : TypedProtoDecoder(data, len) {}
  explicit PowerRails_Decoder(const std::string& raw) : TypedProtoDecoder(reinterpret_cast<const uint8_t*>(raw.data()), raw.size()) {}
  explicit PowerRails_Decoder(const ::protozero::ConstBytes& raw) : TypedProtoDecoder(raw.data, raw.size) {}
  bool has_rail_descriptor() const { return at<1>().valid(); }
  ::protozero::RepeatedFieldIterator<::protozero::ConstBytes> rail_descriptor() const { return GetRepeated<::protozero::ConstBytes>(1); }
  bool has_energy_data() const { return at<2>().valid(); }
  ::protozero::RepeatedFieldIterator<::protozero::ConstBytes> energy_data() const { return GetRepeated<::protozero::ConstBytes>(2); }
};

class PowerRails : public ::protozero::Message {
 public:
  using Decoder = PowerRails_Decoder;
  enum : int32_t {
    kRailDescriptorFieldNumber = 1,
    kEnergyDataFieldNumber = 2,
  };
  static constexpr const char* GetName() { return ".perfetto.protos.PowerRails"; }

  using RailDescriptor = ::perfetto::protos::pbzero::PowerRails_RailDescriptor;
  using EnergyData = ::perfetto::protos::pbzero::PowerRails_EnergyData;

  using FieldMetadata_RailDescriptor =
    ::protozero::proto_utils::FieldMetadata<
      1,
      ::protozero::proto_utils::RepetitionType::kRepeatedNotPacked,
      ::protozero::proto_utils::ProtoSchemaType::kMessage,
      PowerRails_RailDescriptor,
      PowerRails>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_RailDescriptor kRailDescriptor() { return {}; }
  template <typename T = PowerRails_RailDescriptor> T* add_rail_descriptor() {
    return BeginNestedMessage<T>(1);
  }


  using FieldMetadata_EnergyData =
    ::protozero::proto_utils::FieldMetadata<
      2,
      ::protozero::proto_utils::RepetitionType::kRepeatedNotPacked,
      ::protozero::proto_utils::ProtoSchemaType::kMessage,
      PowerRails_EnergyData,
      PowerRails>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_EnergyData kEnergyData() { return {}; }
  template <typename T = PowerRails_EnergyData> T* add_energy_data() {
    return BeginNestedMessage<T>(2);
  }

};

class PowerRails_EnergyData_Decoder : public ::protozero::TypedProtoDecoder</*MAX_FIELD_ID=*/3, /*HAS_NONPACKED_REPEATED_FIELDS=*/false> {
 public:
  PowerRails_EnergyData_Decoder(const uint8_t* data, size_t len) : TypedProtoDecoder(data, len) {}
  explicit PowerRails_EnergyData_Decoder(const std::string& raw) : TypedProtoDecoder(reinterpret_cast<const uint8_t*>(raw.data()), raw.size()) {}
  explicit PowerRails_EnergyData_Decoder(const ::protozero::ConstBytes& raw) : TypedProtoDecoder(raw.data, raw.size) {}
  bool has_index() const { return at<1>().valid(); }
  uint32_t index() const { return at<1>().as_uint32(); }
  bool has_timestamp_ms() const { return at<2>().valid(); }
  uint64_t timestamp_ms() const { return at<2>().as_uint64(); }
  bool has_energy() const { return at<3>().valid(); }
  uint64_t energy() const { return at<3>().as_uint64(); }
};

class PowerRails_EnergyData : public ::protozero::Message {
 public:
  using Decoder = PowerRails_EnergyData_Decoder;
  enum : int32_t {
    kIndexFieldNumber = 1,
    kTimestampMsFieldNumber = 2,
    kEnergyFieldNumber = 3,
  };
  static constexpr const char* GetName() { return ".perfetto.protos.PowerRails.EnergyData"; }


  using FieldMetadata_Index =
    ::protozero::proto_utils::FieldMetadata<
      1,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint32,
      uint32_t,
      PowerRails_EnergyData>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Index kIndex() { return {}; }
  void set_index(uint32_t value) {
    static constexpr uint32_t field_id = FieldMetadata_Index::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint32>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_TimestampMs =
    ::protozero::proto_utils::FieldMetadata<
      2,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint64,
      uint64_t,
      PowerRails_EnergyData>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_TimestampMs kTimestampMs() { return {}; }
  void set_timestamp_ms(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_TimestampMs::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_Energy =
    ::protozero::proto_utils::FieldMetadata<
      3,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint64,
      uint64_t,
      PowerRails_EnergyData>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Energy kEnergy() { return {}; }
  void set_energy(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_Energy::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint64>
        ::Append(*this, field_id, value);
  }
};

class PowerRails_RailDescriptor_Decoder : public ::protozero::TypedProtoDecoder</*MAX_FIELD_ID=*/4, /*HAS_NONPACKED_REPEATED_FIELDS=*/false> {
 public:
  PowerRails_RailDescriptor_Decoder(const uint8_t* data, size_t len) : TypedProtoDecoder(data, len) {}
  explicit PowerRails_RailDescriptor_Decoder(const std::string& raw) : TypedProtoDecoder(reinterpret_cast<const uint8_t*>(raw.data()), raw.size()) {}
  explicit PowerRails_RailDescriptor_Decoder(const ::protozero::ConstBytes& raw) : TypedProtoDecoder(raw.data, raw.size) {}
  bool has_index() const { return at<1>().valid(); }
  uint32_t index() const { return at<1>().as_uint32(); }
  bool has_rail_name() const { return at<2>().valid(); }
  ::protozero::ConstChars rail_name() const { return at<2>().as_string(); }
  bool has_subsys_name() const { return at<3>().valid(); }
  ::protozero::ConstChars subsys_name() const { return at<3>().as_string(); }
  bool has_sampling_rate() const { return at<4>().valid(); }
  uint32_t sampling_rate() const { return at<4>().as_uint32(); }
};

class PowerRails_RailDescriptor : public ::protozero::Message {
 public:
  using Decoder = PowerRails_RailDescriptor_Decoder;
  enum : int32_t {
    kIndexFieldNumber = 1,
    kRailNameFieldNumber = 2,
    kSubsysNameFieldNumber = 3,
    kSamplingRateFieldNumber = 4,
  };
  static constexpr const char* GetName() { return ".perfetto.protos.PowerRails.RailDescriptor"; }


  using FieldMetadata_Index =
    ::protozero::proto_utils::FieldMetadata<
      1,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint32,
      uint32_t,
      PowerRails_RailDescriptor>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Index kIndex() { return {}; }
  void set_index(uint32_t value) {
    static constexpr uint32_t field_id = FieldMetadata_Index::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint32>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_RailName =
    ::protozero::proto_utils::FieldMetadata<
      2,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kString,
      std::string,
      PowerRails_RailDescriptor>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_RailName kRailName() { return {}; }
  void set_rail_name(const char* data, size_t size) {
    AppendBytes(FieldMetadata_RailName::kFieldId, data, size);
  }
  void set_rail_name(std::string value) {
    static constexpr uint32_t field_id = FieldMetadata_RailName::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kString>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_SubsysName =
    ::protozero::proto_utils::FieldMetadata<
      3,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kString,
      std::string,
      PowerRails_RailDescriptor>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_SubsysName kSubsysName() { return {}; }
  void set_subsys_name(const char* data, size_t size) {
    AppendBytes(FieldMetadata_SubsysName::kFieldId, data, size);
  }
  void set_subsys_name(std::string value) {
    static constexpr uint32_t field_id = FieldMetadata_SubsysName::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kString>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_SamplingRate =
    ::protozero::proto_utils::FieldMetadata<
      4,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint32,
      uint32_t,
      PowerRails_RailDescriptor>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_SamplingRate kSamplingRate() { return {}; }
  void set_sampling_rate(uint32_t value) {
    static constexpr uint32_t field_id = FieldMetadata_SamplingRate::kFieldId;
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
