// Autogenerated by the ProtoZero compiler plugin. DO NOT EDIT.

#ifndef PERFETTO_PROTOS_PROTOS_PERFETTO_TRACE_GPU_VULKAN_MEMORY_EVENT_PROTO_H_
#define PERFETTO_PROTOS_PROTOS_PERFETTO_TRACE_GPU_VULKAN_MEMORY_EVENT_PROTO_H_

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

class VulkanMemoryEventAnnotation;
enum VulkanMemoryEvent_AllocationScope : int32_t;
enum VulkanMemoryEvent_Operation : int32_t;
enum VulkanMemoryEvent_Source : int32_t;

enum VulkanMemoryEvent_Source : int32_t {
  VulkanMemoryEvent_Source_SOURCE_UNSPECIFIED = 0,
  VulkanMemoryEvent_Source_SOURCE_DRIVER = 1,
  VulkanMemoryEvent_Source_SOURCE_DEVICE = 2,
  VulkanMemoryEvent_Source_SOURCE_DEVICE_MEMORY = 3,
  VulkanMemoryEvent_Source_SOURCE_BUFFER = 4,
  VulkanMemoryEvent_Source_SOURCE_IMAGE = 5,
};

const VulkanMemoryEvent_Source VulkanMemoryEvent_Source_MIN = VulkanMemoryEvent_Source_SOURCE_UNSPECIFIED;
const VulkanMemoryEvent_Source VulkanMemoryEvent_Source_MAX = VulkanMemoryEvent_Source_SOURCE_IMAGE;

enum VulkanMemoryEvent_Operation : int32_t {
  VulkanMemoryEvent_Operation_OP_UNSPECIFIED = 0,
  VulkanMemoryEvent_Operation_OP_CREATE = 1,
  VulkanMemoryEvent_Operation_OP_DESTROY = 2,
  VulkanMemoryEvent_Operation_OP_BIND = 3,
  VulkanMemoryEvent_Operation_OP_DESTROY_BOUND = 4,
  VulkanMemoryEvent_Operation_OP_ANNOTATIONS = 5,
};

const VulkanMemoryEvent_Operation VulkanMemoryEvent_Operation_MIN = VulkanMemoryEvent_Operation_OP_UNSPECIFIED;
const VulkanMemoryEvent_Operation VulkanMemoryEvent_Operation_MAX = VulkanMemoryEvent_Operation_OP_ANNOTATIONS;

enum VulkanMemoryEvent_AllocationScope : int32_t {
  VulkanMemoryEvent_AllocationScope_SCOPE_UNSPECIFIED = 0,
  VulkanMemoryEvent_AllocationScope_SCOPE_COMMAND = 1,
  VulkanMemoryEvent_AllocationScope_SCOPE_OBJECT = 2,
  VulkanMemoryEvent_AllocationScope_SCOPE_CACHE = 3,
  VulkanMemoryEvent_AllocationScope_SCOPE_DEVICE = 4,
  VulkanMemoryEvent_AllocationScope_SCOPE_INSTANCE = 5,
};

const VulkanMemoryEvent_AllocationScope VulkanMemoryEvent_AllocationScope_MIN = VulkanMemoryEvent_AllocationScope_SCOPE_UNSPECIFIED;
const VulkanMemoryEvent_AllocationScope VulkanMemoryEvent_AllocationScope_MAX = VulkanMemoryEvent_AllocationScope_SCOPE_INSTANCE;

class VulkanMemoryEvent_Decoder : public ::protozero::TypedProtoDecoder</*MAX_FIELD_ID=*/20, /*HAS_NONPACKED_REPEATED_FIELDS=*/true> {
 public:
  VulkanMemoryEvent_Decoder(const uint8_t* data, size_t len) : TypedProtoDecoder(data, len) {}
  explicit VulkanMemoryEvent_Decoder(const std::string& raw) : TypedProtoDecoder(reinterpret_cast<const uint8_t*>(raw.data()), raw.size()) {}
  explicit VulkanMemoryEvent_Decoder(const ::protozero::ConstBytes& raw) : TypedProtoDecoder(raw.data, raw.size) {}
  bool has_source() const { return at<1>().valid(); }
  int32_t source() const { return at<1>().as_int32(); }
  bool has_operation() const { return at<2>().valid(); }
  int32_t operation() const { return at<2>().as_int32(); }
  bool has_timestamp() const { return at<3>().valid(); }
  int64_t timestamp() const { return at<3>().as_int64(); }
  bool has_pid() const { return at<4>().valid(); }
  uint32_t pid() const { return at<4>().as_uint32(); }
  bool has_memory_address() const { return at<5>().valid(); }
  uint64_t memory_address() const { return at<5>().as_uint64(); }
  bool has_memory_size() const { return at<6>().valid(); }
  uint64_t memory_size() const { return at<6>().as_uint64(); }
  bool has_caller_iid() const { return at<7>().valid(); }
  uint64_t caller_iid() const { return at<7>().as_uint64(); }
  bool has_allocation_scope() const { return at<8>().valid(); }
  int32_t allocation_scope() const { return at<8>().as_int32(); }
  bool has_annotations() const { return at<9>().valid(); }
  ::protozero::RepeatedFieldIterator<::protozero::ConstBytes> annotations() const { return GetRepeated<::protozero::ConstBytes>(9); }
  bool has_device() const { return at<16>().valid(); }
  uint64_t device() const { return at<16>().as_uint64(); }
  bool has_device_memory() const { return at<17>().valid(); }
  uint64_t device_memory() const { return at<17>().as_uint64(); }
  bool has_memory_type() const { return at<18>().valid(); }
  uint32_t memory_type() const { return at<18>().as_uint32(); }
  bool has_heap() const { return at<19>().valid(); }
  uint32_t heap() const { return at<19>().as_uint32(); }
  bool has_object_handle() const { return at<20>().valid(); }
  uint64_t object_handle() const { return at<20>().as_uint64(); }
};

class VulkanMemoryEvent : public ::protozero::Message {
 public:
  using Decoder = VulkanMemoryEvent_Decoder;
  enum : int32_t {
    kSourceFieldNumber = 1,
    kOperationFieldNumber = 2,
    kTimestampFieldNumber = 3,
    kPidFieldNumber = 4,
    kMemoryAddressFieldNumber = 5,
    kMemorySizeFieldNumber = 6,
    kCallerIidFieldNumber = 7,
    kAllocationScopeFieldNumber = 8,
    kAnnotationsFieldNumber = 9,
    kDeviceFieldNumber = 16,
    kDeviceMemoryFieldNumber = 17,
    kMemoryTypeFieldNumber = 18,
    kHeapFieldNumber = 19,
    kObjectHandleFieldNumber = 20,
  };
  static constexpr const char* GetName() { return ".perfetto.protos.VulkanMemoryEvent"; }

  using Source = ::perfetto::protos::pbzero::VulkanMemoryEvent_Source;
  using Operation = ::perfetto::protos::pbzero::VulkanMemoryEvent_Operation;
  using AllocationScope = ::perfetto::protos::pbzero::VulkanMemoryEvent_AllocationScope;
  static const Source SOURCE_UNSPECIFIED = VulkanMemoryEvent_Source_SOURCE_UNSPECIFIED;
  static const Source SOURCE_DRIVER = VulkanMemoryEvent_Source_SOURCE_DRIVER;
  static const Source SOURCE_DEVICE = VulkanMemoryEvent_Source_SOURCE_DEVICE;
  static const Source SOURCE_DEVICE_MEMORY = VulkanMemoryEvent_Source_SOURCE_DEVICE_MEMORY;
  static const Source SOURCE_BUFFER = VulkanMemoryEvent_Source_SOURCE_BUFFER;
  static const Source SOURCE_IMAGE = VulkanMemoryEvent_Source_SOURCE_IMAGE;
  static const Operation OP_UNSPECIFIED = VulkanMemoryEvent_Operation_OP_UNSPECIFIED;
  static const Operation OP_CREATE = VulkanMemoryEvent_Operation_OP_CREATE;
  static const Operation OP_DESTROY = VulkanMemoryEvent_Operation_OP_DESTROY;
  static const Operation OP_BIND = VulkanMemoryEvent_Operation_OP_BIND;
  static const Operation OP_DESTROY_BOUND = VulkanMemoryEvent_Operation_OP_DESTROY_BOUND;
  static const Operation OP_ANNOTATIONS = VulkanMemoryEvent_Operation_OP_ANNOTATIONS;
  static const AllocationScope SCOPE_UNSPECIFIED = VulkanMemoryEvent_AllocationScope_SCOPE_UNSPECIFIED;
  static const AllocationScope SCOPE_COMMAND = VulkanMemoryEvent_AllocationScope_SCOPE_COMMAND;
  static const AllocationScope SCOPE_OBJECT = VulkanMemoryEvent_AllocationScope_SCOPE_OBJECT;
  static const AllocationScope SCOPE_CACHE = VulkanMemoryEvent_AllocationScope_SCOPE_CACHE;
  static const AllocationScope SCOPE_DEVICE = VulkanMemoryEvent_AllocationScope_SCOPE_DEVICE;
  static const AllocationScope SCOPE_INSTANCE = VulkanMemoryEvent_AllocationScope_SCOPE_INSTANCE;

  using FieldMetadata_Source =
    ::protozero::proto_utils::FieldMetadata<
      1,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kEnum,
      ::perfetto::protos::pbzero::VulkanMemoryEvent_Source,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Source kSource() { return {}; }
  void set_source(::perfetto::protos::pbzero::VulkanMemoryEvent_Source value) {
    static constexpr uint32_t field_id = FieldMetadata_Source::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kEnum>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_Operation =
    ::protozero::proto_utils::FieldMetadata<
      2,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kEnum,
      ::perfetto::protos::pbzero::VulkanMemoryEvent_Operation,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Operation kOperation() { return {}; }
  void set_operation(::perfetto::protos::pbzero::VulkanMemoryEvent_Operation value) {
    static constexpr uint32_t field_id = FieldMetadata_Operation::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kEnum>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_Timestamp =
    ::protozero::proto_utils::FieldMetadata<
      3,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kInt64,
      int64_t,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Timestamp kTimestamp() { return {}; }
  void set_timestamp(int64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_Timestamp::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kInt64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_Pid =
    ::protozero::proto_utils::FieldMetadata<
      4,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint32,
      uint32_t,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Pid kPid() { return {}; }
  void set_pid(uint32_t value) {
    static constexpr uint32_t field_id = FieldMetadata_Pid::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint32>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_MemoryAddress =
    ::protozero::proto_utils::FieldMetadata<
      5,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kFixed64,
      uint64_t,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_MemoryAddress kMemoryAddress() { return {}; }
  void set_memory_address(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_MemoryAddress::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kFixed64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_MemorySize =
    ::protozero::proto_utils::FieldMetadata<
      6,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint64,
      uint64_t,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_MemorySize kMemorySize() { return {}; }
  void set_memory_size(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_MemorySize::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_CallerIid =
    ::protozero::proto_utils::FieldMetadata<
      7,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint64,
      uint64_t,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_CallerIid kCallerIid() { return {}; }
  void set_caller_iid(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_CallerIid::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_AllocationScope =
    ::protozero::proto_utils::FieldMetadata<
      8,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kEnum,
      ::perfetto::protos::pbzero::VulkanMemoryEvent_AllocationScope,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_AllocationScope kAllocationScope() { return {}; }
  void set_allocation_scope(::perfetto::protos::pbzero::VulkanMemoryEvent_AllocationScope value) {
    static constexpr uint32_t field_id = FieldMetadata_AllocationScope::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kEnum>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_Annotations =
    ::protozero::proto_utils::FieldMetadata<
      9,
      ::protozero::proto_utils::RepetitionType::kRepeatedNotPacked,
      ::protozero::proto_utils::ProtoSchemaType::kMessage,
      VulkanMemoryEventAnnotation,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Annotations kAnnotations() { return {}; }
  template <typename T = VulkanMemoryEventAnnotation> T* add_annotations() {
    return BeginNestedMessage<T>(9);
  }


  using FieldMetadata_Device =
    ::protozero::proto_utils::FieldMetadata<
      16,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kFixed64,
      uint64_t,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Device kDevice() { return {}; }
  void set_device(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_Device::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kFixed64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_DeviceMemory =
    ::protozero::proto_utils::FieldMetadata<
      17,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kFixed64,
      uint64_t,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_DeviceMemory kDeviceMemory() { return {}; }
  void set_device_memory(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_DeviceMemory::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kFixed64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_MemoryType =
    ::protozero::proto_utils::FieldMetadata<
      18,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint32,
      uint32_t,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_MemoryType kMemoryType() { return {}; }
  void set_memory_type(uint32_t value) {
    static constexpr uint32_t field_id = FieldMetadata_MemoryType::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint32>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_Heap =
    ::protozero::proto_utils::FieldMetadata<
      19,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint32,
      uint32_t,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_Heap kHeap() { return {}; }
  void set_heap(uint32_t value) {
    static constexpr uint32_t field_id = FieldMetadata_Heap::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint32>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_ObjectHandle =
    ::protozero::proto_utils::FieldMetadata<
      20,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kFixed64,
      uint64_t,
      VulkanMemoryEvent>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_ObjectHandle kObjectHandle() { return {}; }
  void set_object_handle(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_ObjectHandle::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kFixed64>
        ::Append(*this, field_id, value);
  }
};

class VulkanMemoryEventAnnotation_Decoder : public ::protozero::TypedProtoDecoder</*MAX_FIELD_ID=*/4, /*HAS_NONPACKED_REPEATED_FIELDS=*/false> {
 public:
  VulkanMemoryEventAnnotation_Decoder(const uint8_t* data, size_t len) : TypedProtoDecoder(data, len) {}
  explicit VulkanMemoryEventAnnotation_Decoder(const std::string& raw) : TypedProtoDecoder(reinterpret_cast<const uint8_t*>(raw.data()), raw.size()) {}
  explicit VulkanMemoryEventAnnotation_Decoder(const ::protozero::ConstBytes& raw) : TypedProtoDecoder(raw.data, raw.size) {}
  bool has_key_iid() const { return at<1>().valid(); }
  uint64_t key_iid() const { return at<1>().as_uint64(); }
  bool has_int_value() const { return at<2>().valid(); }
  int64_t int_value() const { return at<2>().as_int64(); }
  bool has_double_value() const { return at<3>().valid(); }
  double double_value() const { return at<3>().as_double(); }
  bool has_string_iid() const { return at<4>().valid(); }
  uint64_t string_iid() const { return at<4>().as_uint64(); }
};

class VulkanMemoryEventAnnotation : public ::protozero::Message {
 public:
  using Decoder = VulkanMemoryEventAnnotation_Decoder;
  enum : int32_t {
    kKeyIidFieldNumber = 1,
    kIntValueFieldNumber = 2,
    kDoubleValueFieldNumber = 3,
    kStringIidFieldNumber = 4,
  };
  static constexpr const char* GetName() { return ".perfetto.protos.VulkanMemoryEventAnnotation"; }


  using FieldMetadata_KeyIid =
    ::protozero::proto_utils::FieldMetadata<
      1,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint64,
      uint64_t,
      VulkanMemoryEventAnnotation>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_KeyIid kKeyIid() { return {}; }
  void set_key_iid(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_KeyIid::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kUint64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_IntValue =
    ::protozero::proto_utils::FieldMetadata<
      2,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kInt64,
      int64_t,
      VulkanMemoryEventAnnotation>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_IntValue kIntValue() { return {}; }
  void set_int_value(int64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_IntValue::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kInt64>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_DoubleValue =
    ::protozero::proto_utils::FieldMetadata<
      3,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kDouble,
      double,
      VulkanMemoryEventAnnotation>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_DoubleValue kDoubleValue() { return {}; }
  void set_double_value(double value) {
    static constexpr uint32_t field_id = FieldMetadata_DoubleValue::kFieldId;
    // Call the appropriate protozero::Message::Append(field_id, ...)
    // method based on the type of the field.
    ::protozero::internal::FieldWriter<
      ::protozero::proto_utils::ProtoSchemaType::kDouble>
        ::Append(*this, field_id, value);
  }

  using FieldMetadata_StringIid =
    ::protozero::proto_utils::FieldMetadata<
      4,
      ::protozero::proto_utils::RepetitionType::kNotRepeated,
      ::protozero::proto_utils::ProtoSchemaType::kUint64,
      uint64_t,
      VulkanMemoryEventAnnotation>;

  // Ceci n'est pas une pipe.
  // This is actually a variable of FieldMetadataHelper<FieldMetadata<...>>
  // type (and users are expected to use it as such, hence kCamelCase name).
  // It is declared as a function to keep protozero bindings header-only as
  // inline constexpr variables are not available until C++17 (while inline
  // functions are).
  // TODO(altimin): Use inline variable instead after adopting C++17.
  static constexpr FieldMetadata_StringIid kStringIid() { return {}; }
  void set_string_iid(uint64_t value) {
    static constexpr uint32_t field_id = FieldMetadata_StringIid::kFieldId;
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
