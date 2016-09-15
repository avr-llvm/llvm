//===- CVTypeVisitor.cpp ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/CodeView/CVTypeVisitor.h"

#include "llvm/DebugInfo/CodeView/CodeViewError.h"
#include "llvm/DebugInfo/MSF/ByteStream.h"

using namespace llvm;
using namespace llvm::codeview;

template <typename T>
static Error takeObject(ArrayRef<uint8_t> &Data, const T *&Res) {
  if (Data.size() < sizeof(*Res))
    return llvm::make_error<CodeViewError>(cv_error_code::insufficient_buffer);
  Res = reinterpret_cast<const T *>(Data.data());
  Data = Data.drop_front(sizeof(*Res));
  return Error::success();
}

static Error skipPadding(ArrayRef<uint8_t> &Data) {
  if (Data.empty())
    return Error::success();
  uint8_t Leaf = Data.front();
  if (Leaf < LF_PAD0)
    return Error::success();
  // Leaf is greater than 0xf0. We should advance by the number of bytes in
  // the low 4 bits.
  unsigned BytesToAdvance = Leaf & 0x0F;
  if (Data.size() < BytesToAdvance) {
    return llvm::make_error<CodeViewError>(cv_error_code::corrupt_record,
                                           "Invalid padding bytes!");
  }
  Data = Data.drop_front(BytesToAdvance);
  return Error::success();
}

template <typename T>
static Expected<CVMemberRecord> deserializeMemberRecord(ArrayRef<uint8_t> &Data,
                                                        TypeLeafKind Kind) {
  ArrayRef<uint8_t> OldData = Data;
  TypeRecordKind RK = static_cast<TypeRecordKind>(Kind);
  auto ExpectedRecord = T::deserialize(RK, Data);
  if (!ExpectedRecord)
    return ExpectedRecord.takeError();
  assert(Data.size() < OldData.size());
  if (auto EC = skipPadding(Data))
    return std::move(EC);

  CVMemberRecord CVMR;
  CVMR.Kind = Kind;
  CVMR.Data = OldData.drop_back(Data.size());
  return CVMR;
}

CVTypeVisitor::CVTypeVisitor(TypeVisitorCallbacks &Callbacks)
    : Callbacks(Callbacks) {}

template <typename T>
static Error visitKnownRecord(CVType &Record, TypeVisitorCallbacks &Callbacks) {
  TypeRecordKind RK = static_cast<TypeRecordKind>(Record.Type);
  T KnownRecord(RK);
  if (auto EC = Callbacks.visitKnownRecord(Record, KnownRecord))
    return EC;
  return Error::success();
}

template <typename T>
static Error visitKnownMember(CVMemberRecord &Record,
                              TypeVisitorCallbacks &Callbacks) {
  TypeRecordKind RK = static_cast<TypeRecordKind>(Record.Kind);
  T KnownRecord(RK);
  if (auto EC = Callbacks.visitKnownMember(Record, KnownRecord))
    return EC;
  return Error::success();
}

Error CVTypeVisitor::visitTypeRecord(CVType &Record) {
  if (auto EC = Callbacks.visitTypeBegin(Record))
    return EC;

  switch (Record.Type) {
  default:
    if (auto EC = Callbacks.visitUnknownType(Record))
      return EC;
    break;
#define TYPE_RECORD(EnumName, EnumVal, Name)                                   \
  case EnumName: {                                                             \
    if (auto EC = visitKnownRecord<Name##Record>(Record, Callbacks))           \
      return EC;                                                               \
    break;                                                                     \
  }
#define TYPE_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)                  \
  TYPE_RECORD(EnumVal, EnumVal, AliasName)
#define MEMBER_RECORD(EnumName, EnumVal, Name)
#define MEMBER_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#include "llvm/DebugInfo/CodeView/TypeRecords.def"
  }

  if (auto EC = Callbacks.visitTypeEnd(Record))
    return EC;

  return Error::success();
}

Error CVTypeVisitor::visitMemberRecord(CVMemberRecord &Record) {
  if (auto EC = Callbacks.visitMemberBegin(Record))
    return EC;

  switch (Record.Kind) {
  default:
    if (auto EC = Callbacks.visitUnknownMember(Record))
      return EC;
    break;
#define MEMBER_RECORD(EnumName, EnumVal, Name)                                 \
  case EnumName: {                                                             \
    if (auto EC = visitKnownMember<Name##Record>(Record, Callbacks))           \
      return EC;                                                               \
    break;                                                                     \
  }
#define MEMBER_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)                \
  MEMBER_RECORD(EnumVal, EnumVal, AliasName)
#define TYPE_RECORD(EnumName, EnumVal, Name)
#define TYPE_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#include "llvm/DebugInfo/CodeView/TypeRecords.def"
  }

  if (auto EC = Callbacks.visitMemberEnd(Record))
    return EC;

  return Error::success();
}

/// Visits the type records in Data. Sets the error flag on parse failures.
Error CVTypeVisitor::visitTypeStream(const CVTypeArray &Types) {
  for (auto I : Types) {
    if (auto EC = visitTypeRecord(I))
      return EC;
  }
  return Error::success();
}

template <typename MR>
static Error visitKnownMember(ArrayRef<uint8_t> &Data, TypeLeafKind Leaf,
                              TypeVisitorCallbacks &Callbacks) {
  auto ExpectedRecord = deserializeMemberRecord<MR>(Data, Leaf);
  if (!ExpectedRecord)
    return ExpectedRecord.takeError();
  CVMemberRecord &Record = *ExpectedRecord;
  if (auto EC = Callbacks.visitMemberBegin(Record))
    return EC;
  if (auto EC = visitKnownMember<MR>(Record, Callbacks))
    return EC;
  if (auto EC = Callbacks.visitMemberEnd(Record))
    return EC;
  return Error::success();
}

Error CVTypeVisitor::visitFieldListMemberStream(ArrayRef<uint8_t> Data) {
  while (!Data.empty()) {
    const support::ulittle16_t *LeafValue;
    if (auto EC = takeObject(Data, LeafValue))
      return EC;

    TypeLeafKind Leaf = static_cast<TypeLeafKind>(uint16_t(*LeafValue));
    CVType Record;
    switch (Leaf) {
    default:
      // Field list records do not describe their own length, so we cannot
      // continue parsing past a type that we don't know how to deserialize.
      return llvm::make_error<CodeViewError>(
          cv_error_code::unknown_member_record);
#define MEMBER_RECORD(EnumName, EnumVal, Name)                                 \
  case EnumName: {                                                             \
    if (auto EC = visitKnownMember<Name##Record>(Data, Leaf, Callbacks))       \
      return EC;                                                               \
    break;                                                                     \
  }
#define MEMBER_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)                \
  MEMBER_RECORD(EnumVal, EnumVal, AliasName)
#include "llvm/DebugInfo/CodeView/TypeRecords.def"
    }
  }
  return Error::success();
}
