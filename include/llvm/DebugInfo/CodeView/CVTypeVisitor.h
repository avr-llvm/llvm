//===- CVTypeVisitor.h ------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_CODEVIEW_CVTYPEVISITOR_H
#define LLVM_DEBUGINFO_CODEVIEW_CVTYPEVISITOR_H

#include "llvm/DebugInfo/CodeView/CodeView.h"
#include "llvm/DebugInfo/CodeView/RecordIterator.h"
#include "llvm/DebugInfo/CodeView/TypeIndex.h"
#include "llvm/DebugInfo/CodeView/TypeRecord.h"
#include "llvm/DebugInfo/CodeView/TypeStream.h"
#include "llvm/Support/ErrorOr.h"

namespace llvm {
namespace codeview {

template <typename Derived>
class CVTypeVisitor {
public:
  CVTypeVisitor() {}

  bool hadError() const { return HadError; }

  template <typename T>
  bool consumeObject(ArrayRef<uint8_t> &Data, const T *&Res) {
    if (Data.size() < sizeof(*Res)) {
      HadError = true;
      return false;
    }
    Res = reinterpret_cast<const T *>(Data.data());
    Data = Data.drop_front(sizeof(*Res));
    return true;
  }

  /// Actions to take on known types. By default, they do nothing. Visit methods
  /// for member records take the FieldData by non-const reference and are
  /// expected to consume the trailing bytes used by the field.
  /// FIXME: Make the visitor interpret the trailing bytes so that clients don't
  /// need to.
#define TYPE_RECORD(EnumName, EnumVal, Name)                                   \
  void visit##Name(TypeLeafKind LeafType, Name##Record &Record) {}
#define TYPE_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#define MEMBER_RECORD(EnumName, EnumVal, Name)                                 \
  void visit##Name(TypeLeafKind LeafType, Name##Record &Record) {}
#define MEMBER_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#include "TypeRecords.def"

  void visitTypeRecord(const TypeIterator::Record &Record) {
    ArrayRef<uint8_t> LeafData = Record.Data;
    ArrayRef<uint8_t> RecordData = LeafData;
    auto *DerivedThis = static_cast<Derived *>(this);
    DerivedThis->visitTypeBegin(Record.Type, RecordData);
    switch (Record.Type) {
    default:
      DerivedThis->visitUnknownType(Record.Type);
      break;
    case LF_FIELDLIST:
      DerivedThis->visitFieldList(Record.Type, LeafData);
      break;
#define TYPE_RECORD(EnumName, EnumVal, Name)                                   \
  case EnumName: {                                                             \
    TypeRecordKind RK = static_cast<TypeRecordKind>(EnumName);                 \
    auto Result = Name##Record::deserialize(RK, LeafData);                     \
    if (Result.getError())                                                     \
      return parseError();                                                     \
    DerivedThis->visit##Name(Record.Type, *Result);                            \
    break;                                                                     \
  }
#define TYPE_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)                  \
  TYPE_RECORD(EnumVal, EnumVal, AliasName)
#define MEMBER_RECORD(EnumName, EnumVal, Name)
#include "TypeRecords.def"
      }
      DerivedThis->visitTypeEnd(Record.Type, RecordData);
  }

  /// Visits the type records in Data. Sets the error flag on parse failures.
  void visitTypeStream(ArrayRef<uint8_t> Data) {
    for (const auto &I : makeTypeRange(Data, &HadError)) {
      visitTypeRecord(I);
      if (hadError())
        break;
    }
  }

  /// Action to take on unknown types. By default, they are ignored.
  void visitUnknownType(TypeLeafKind Leaf) {}

  /// Paired begin/end actions for all types. Receives all record data,
  /// including the fixed-length record prefix.
  void visitTypeBegin(TypeLeafKind Leaf, ArrayRef<uint8_t> RecordData) {}
  void visitTypeEnd(TypeLeafKind Leaf, ArrayRef<uint8_t> RecordData) {}

  static ArrayRef<uint8_t> skipPadding(ArrayRef<uint8_t> Data) {
    if (Data.empty())
      return Data;
    uint8_t Leaf = Data.front();
    if (Leaf < LF_PAD0)
      return Data;
    // Leaf is greater than 0xf0. We should advance by the number of bytes in
    // the low 4 bits.
    return Data.drop_front(Leaf & 0x0F);
  }

  /// Visits individual member records of a field list record. Member records do
  /// not describe their own length, and need special handling.
  void visitFieldList(TypeLeafKind Leaf, ArrayRef<uint8_t> FieldData) {
    auto *DerivedThis = static_cast<Derived *>(this);
    while (!FieldData.empty()) {
      const ulittle16_t *LeafPtr;
      if (!CVTypeVisitor::consumeObject(FieldData, LeafPtr))
        return;
      TypeLeafKind Leaf = TypeLeafKind(unsigned(*LeafPtr));
      switch (Leaf) {
      default:
        // Field list records do not describe their own length, so we cannot
        // continue parsing past an unknown member type.
        DerivedThis->visitUnknownMember(Leaf);
        return parseError();
#define MEMBER_RECORD(EnumName, EnumVal, Name)                                 \
  case EnumName: {                                                             \
    TypeRecordKind RK = static_cast<TypeRecordKind>(EnumName);                 \
    auto Result = Name##Record::deserialize(RK, FieldData);                    \
    if (Result.getError())                                                     \
      return parseError();                                                     \
    DerivedThis->visit##Name(Leaf, *Result);                                   \
    break;                                                                     \
  }
#define MEMBER_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)                \
  MEMBER_RECORD(EnumVal, EnumVal, AliasName)
#include "TypeRecords.def"
      }
      FieldData = skipPadding(FieldData);
    }
  }

  /// Action to take on unknown members. By default, they are ignored. Member
  /// record parsing cannot recover from an unknown member record, so this
  /// method is only called at most once per field list record.
  void visitUnknownMember(TypeLeafKind Leaf) {}

  /// Helper for returning from a void function when the stream is corrupted.
  void parseError() { HadError = true; }

private:
  /// Whether a type stream parsing error was encountered.
  bool HadError = false;
};

} // end namespace codeview
} // end namespace llvm

#endif // LLVM_DEBUGINFO_CODEVIEW_CVTYPEVISITOR_H
