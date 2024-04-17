// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: Payload.proto

#include "Payload.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
namespace myPayload {
class PayloadDefaultTypeInternal {
 public:
  ::PROTOBUF_NAMESPACE_ID::internal::ExplicitlyConstructed<Payload> _instance;
} _Payload_default_instance_;
}  // namespace myPayload
static void InitDefaultsscc_info_Payload_Payload_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::myPayload::_Payload_default_instance_;
    new (ptr) ::myPayload::Payload();
    ::PROTOBUF_NAMESPACE_ID::internal::OnShutdownDestroyMessage(ptr);
  }
}

::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<0> scc_info_Payload_Payload_2eproto =
    {{ATOMIC_VAR_INIT(::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase::kUninitialized), 0, 0, InitDefaultsscc_info_Payload_Payload_2eproto}, {}};

static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_Payload_2eproto[1];
static const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* file_level_enum_descriptors_Payload_2eproto[1];
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_Payload_2eproto = nullptr;

const ::PROTOBUF_NAMESPACE_ID::uint32 TableStruct_Payload_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::myPayload::Payload, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::myPayload::Payload, payloadtype_),
  PROTOBUF_FIELD_OFFSET(::myPayload::Payload, content_),
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, sizeof(::myPayload::Payload)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::myPayload::_Payload_default_instance_),
};

const char descriptor_table_protodef_Payload_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\rPayload.proto\022\tmyPayload\"G\n\007Payload\022+\n"
  "\013payloadtype\030\001 \001(\0162\026.myPayload.PayloadTy"
  "pe\022\017\n\007content\030\002 \001(\t*C\n\013PayloadType\022\017\n\013SE"
  "RVER_PING\020\000\022\022\n\016SERVER_MESSAGE\020\001\022\017\n\013ALL_M"
  "ESSAGE\020\002b\006proto3"
  ;
static const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable*const descriptor_table_Payload_2eproto_deps[1] = {
};
static ::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase*const descriptor_table_Payload_2eproto_sccs[1] = {
  &scc_info_Payload_Payload_2eproto.base,
};
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_Payload_2eproto_once;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_Payload_2eproto = {
  false, false, descriptor_table_protodef_Payload_2eproto, "Payload.proto", 176,
  &descriptor_table_Payload_2eproto_once, descriptor_table_Payload_2eproto_sccs, descriptor_table_Payload_2eproto_deps, 1, 0,
  schemas, file_default_instances, TableStruct_Payload_2eproto::offsets,
  file_level_metadata_Payload_2eproto, 1, file_level_enum_descriptors_Payload_2eproto, file_level_service_descriptors_Payload_2eproto,
};

// Force running AddDescriptors() at dynamic initialization time.
static bool dynamic_init_dummy_Payload_2eproto = (static_cast<void>(::PROTOBUF_NAMESPACE_ID::internal::AddDescriptors(&descriptor_table_Payload_2eproto)), true);
namespace myPayload {
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* PayloadType_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_Payload_2eproto);
  return file_level_enum_descriptors_Payload_2eproto[0];
}
bool PayloadType_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
    case 2:
      return true;
    default:
      return false;
  }
}


// ===================================================================

class Payload::_Internal {
 public:
};

Payload::Payload(::PROTOBUF_NAMESPACE_ID::Arena* arena)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena) {
  SharedCtor();
  RegisterArenaDtor(arena);
  // @@protoc_insertion_point(arena_constructor:myPayload.Payload)
}
Payload::Payload(const Payload& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  content_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  if (!from._internal_content().empty()) {
    content_.Set(::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::EmptyDefault{}, from._internal_content(), 
      GetArena());
  }
  payloadtype_ = from.payloadtype_;
  // @@protoc_insertion_point(copy_constructor:myPayload.Payload)
}

void Payload::SharedCtor() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&scc_info_Payload_Payload_2eproto.base);
  content_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  payloadtype_ = 0;
}

Payload::~Payload() {
  // @@protoc_insertion_point(destructor:myPayload.Payload)
  SharedDtor();
  _internal_metadata_.Delete<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

void Payload::SharedDtor() {
  GOOGLE_DCHECK(GetArena() == nullptr);
  content_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}

void Payload::ArenaDtor(void* object) {
  Payload* _this = reinterpret_cast< Payload* >(object);
  (void)_this;
}
void Payload::RegisterArenaDtor(::PROTOBUF_NAMESPACE_ID::Arena*) {
}
void Payload::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const Payload& Payload::default_instance() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&::scc_info_Payload_Payload_2eproto.base);
  return *internal_default_instance();
}


void Payload::Clear() {
// @@protoc_insertion_point(message_clear_start:myPayload.Payload)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  content_.ClearToEmpty();
  payloadtype_ = 0;
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* Payload::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
    switch (tag >> 3) {
      // .myPayload.PayloadType payloadtype = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 8)) {
          ::PROTOBUF_NAMESPACE_ID::uint64 val = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
          _internal_set_payloadtype(static_cast<::myPayload::PayloadType>(val));
        } else goto handle_unusual;
        continue;
      // string content = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 18)) {
          auto str = _internal_mutable_content();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(::PROTOBUF_NAMESPACE_ID::internal::VerifyUTF8(str, "myPayload.Payload.content"));
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag,
            _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
            ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* Payload::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:myPayload.Payload)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // .myPayload.PayloadType payloadtype = 1;
  if (this->payloadtype() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteEnumToArray(
      1, this->_internal_payloadtype(), target);
  }

  // string content = 2;
  if (this->content().size() > 0) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_content().data(), static_cast<int>(this->_internal_content().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "myPayload.Payload.content");
    target = stream->WriteStringMaybeAliased(
        2, this->_internal_content(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:myPayload.Payload)
  return target;
}

size_t Payload::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:myPayload.Payload)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string content = 2;
  if (this->content().size() > 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_content());
  }

  // .myPayload.PayloadType payloadtype = 1;
  if (this->payloadtype() != 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::EnumSize(this->_internal_payloadtype());
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void Payload::MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:myPayload.Payload)
  GOOGLE_DCHECK_NE(&from, this);
  const Payload* source =
      ::PROTOBUF_NAMESPACE_ID::DynamicCastToGenerated<Payload>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:myPayload.Payload)
    ::PROTOBUF_NAMESPACE_ID::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:myPayload.Payload)
    MergeFrom(*source);
  }
}

void Payload::MergeFrom(const Payload& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:myPayload.Payload)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.content().size() > 0) {
    _internal_set_content(from._internal_content());
  }
  if (from.payloadtype() != 0) {
    _internal_set_payloadtype(from._internal_payloadtype());
  }
}

void Payload::CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:myPayload.Payload)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void Payload::CopyFrom(const Payload& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:myPayload.Payload)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Payload::IsInitialized() const {
  return true;
}

void Payload::InternalSwap(Payload* other) {
  using std::swap;
  _internal_metadata_.Swap<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(&other->_internal_metadata_);
  content_.Swap(&other->content_, &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), GetArena());
  swap(payloadtype_, other->payloadtype_);
}

::PROTOBUF_NAMESPACE_ID::Metadata Payload::GetMetadata() const {
  return GetMetadataStatic();
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace myPayload
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::myPayload::Payload* Arena::CreateMaybeMessage< ::myPayload::Payload >(Arena* arena) {
  return Arena::CreateMessageInternal< ::myPayload::Payload >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
