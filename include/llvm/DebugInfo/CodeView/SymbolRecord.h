//===- SymbolRecord.h -------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_CODEVIEW_SYMBOLRECORD_H
#define LLVM_DEBUGINFO_CODEVIEW_SYMBOLRECORD_H

#include "llvm/DebugInfo/CodeView/CodeView.h"
#include "llvm/DebugInfo/CodeView/RecordIterator.h"
#include "llvm/DebugInfo/CodeView/TypeIndex.h"
#include "llvm/Support/Endian.h"

namespace llvm {
namespace codeview {

using llvm::support::ulittle16_t;
using llvm::support::ulittle32_t;
using llvm::support::little32_t;

// S_GPROC32, S_LPROC32, S_GPROC32_ID, S_LPROC32_ID, S_LPROC32_DPC or
// S_LPROC32_DPC_ID
struct ProcSym {
  ulittle32_t PtrParent;
  ulittle32_t PtrEnd;
  ulittle32_t PtrNext;
  ulittle32_t CodeSize;
  ulittle32_t DbgStart;
  ulittle32_t DbgEnd;
  TypeIndex FunctionType;
  ulittle32_t CodeOffset;
  ulittle16_t Segment;
  uint8_t Flags; // ProcSymFlags enum
  // Name: The null-terminated name follows.
};

// S_INLINESITE
struct InlineSiteSym {
  ulittle32_t PtrParent;
  ulittle32_t PtrEnd;
  TypeIndex Inlinee;
  // BinaryAnnotations
};

// S_LOCAL
struct LocalSym {
  TypeIndex Type;
  ulittle16_t Flags; // LocalSymFlags enum
  // Name: The null-terminated name follows.
};

struct LocalVariableAddrRange {
  ulittle32_t OffsetStart;
  ulittle16_t ISectStart;
  ulittle16_t Range;
};

struct LocalVariableAddrGap {
  ulittle16_t GapStartOffset;
  ulittle16_t Range;
};

enum : uint16_t { MaxDefRange = 0xf000 };

// S_DEFRANGE
struct DefRangeSym {
  ulittle32_t Program;
  LocalVariableAddrRange Range;
  // LocalVariableAddrGap Gaps[];
};

// S_DEFRANGE_SUBFIELD
struct DefRangeSubfieldSym {
  ulittle32_t Program;
  ulittle16_t OffsetInParent;
  LocalVariableAddrRange Range;
  // LocalVariableAddrGap Gaps[];
};

// S_DEFRANGE_REGISTER
struct DefRangeRegisterSym {
  ulittle16_t Register;
  ulittle16_t MayHaveNoName;
  LocalVariableAddrRange Range;
  // LocalVariableAddrGap Gaps[];
};

// S_DEFRANGE_SUBFIELD_REGISTER
struct DefRangeSubfieldRegisterSym {
  ulittle16_t Register; // Register to which the variable is relative
  ulittle16_t MayHaveNoName;
  ulittle32_t OffsetInParent;
  LocalVariableAddrRange Range;
  // LocalVariableAddrGap Gaps[];
};

// S_DEFRANGE_FRAMEPOINTER_REL
struct DefRangeFramePointerRelSym {
  little32_t Offset; // Offset from the frame pointer register
  LocalVariableAddrRange Range;
  // LocalVariableAddrGap Gaps[];
};

// S_DEFRANGE_FRAMEPOINTER_REL_FULL_SCOPE
struct DefRangeFramePointerRelFullScopeSym {
  little32_t Offset; // Offset from the frame pointer register
};

// S_DEFRANGE_REGISTER_REL
struct DefRangeRegisterRelSym {
  ulittle16_t BaseRegister;
  ulittle16_t Flags;
  little32_t BasePointerOffset;
  LocalVariableAddrRange Range;
  // LocalVariableAddrGap Gaps[];

  bool hasSpilledUDTMember() const { return Flags & 1; }
  uint16_t offsetInParent() const { return Flags >> 4; }
};

// S_BLOCK32
struct BlockSym {
  ulittle32_t PtrParent;
  ulittle32_t PtrEnd;
  ulittle32_t CodeSize;
  ulittle32_t CodeOffset;
  ulittle16_t Segment;
  // Name: The null-terminated name follows.
};

// S_LABEL32
struct LabelSym {
  ulittle32_t CodeOffset;
  ulittle16_t Segment;
  uint8_t Flags; // CV_PROCFLAGS
  // Name: The null-terminated name follows.
};

// S_OBJNAME
struct ObjNameSym {
  ulittle32_t Signature;
  // Name: The null-terminated name follows.
};

// S_COMPILE3
struct CompileSym3 {
  ulittle32_t flags; // CompileSym3Flags enum
  uint8_t getLanguage() const { return flags & 0xff; }
  ulittle16_t Machine; // CPUType
  ulittle16_t VersionFrontendMajor;
  ulittle16_t VersionFrontendMinor;
  ulittle16_t VersionFrontendBuild;
  ulittle16_t VersionFrontendQFE;
  ulittle16_t VersionBackendMajor;
  ulittle16_t VersionBackendMinor;
  ulittle16_t VersionBackendBuild;
  ulittle16_t VersionBackendQFE;
  // VersionString: The null-terminated version string follows.
};

// S_FRAMEPROC
struct FrameProcSym {
  ulittle32_t TotalFrameBytes;
  ulittle32_t PaddingFrameBytes;
  ulittle32_t OffsetToPadding;
  ulittle32_t BytesOfCalleeSavedRegisters;
  ulittle32_t OffsetOfExceptionHandler;
  ulittle16_t SectionIdOfExceptionHandler;
  ulittle32_t Flags;
};

// S_CALLSITEINFO
struct CallSiteInfoSym {
  ulittle32_t CodeOffset;
  ulittle16_t Segment;
  ulittle16_t Reserved;
  TypeIndex Type;
};

// S_HEAPALLOCSITE
struct HeapAllocationSiteSym {
  ulittle32_t CodeOffset;
  ulittle16_t Segment;
  ulittle16_t CallInstructionSize;
  TypeIndex Type;
};

// S_FRAMECOOKIE
struct FrameCookieSym {
  ulittle32_t CodeOffset;
  ulittle16_t Register;
  ulittle32_t CookieKind;
};

// S_UDT, S_COBOLUDT
struct UDTSym {
  TypeIndex Type; // Type of the UDT
  // Name: The null-terminated name follows.
};

// S_BUILDINFO
struct BuildInfoSym {
  ulittle32_t BuildId;
};

// S_BPREL32
struct BPRelativeSym {
  little32_t Offset;  // Offset from the base pointer register
  TypeIndex Type;     // Type of the variable
  // Name: The null-terminated name follows.
};

// S_REGREL32
struct RegRelativeSym {
  ulittle32_t Offset;   // Offset from the register
  TypeIndex Type;       // Type of the variable
  ulittle16_t Register; // Register to which the variable is relative
  // Name: The null-terminated name follows.
};

// S_CONSTANT, S_MANCONSTANT
struct ConstantSym {
  TypeIndex Type;
  // Value: The value of the constant.
  // Name: The null-terminated name follows.
};

// S_LDATA32, S_GDATA32, S_LMANDATA, S_GMANDATA
struct DataSym {
  TypeIndex Type;
  ulittle32_t DataOffset;
  ulittle16_t Segment;
  // Name: The null-terminated name follows.
};

// S_LTHREAD32, S_GTHREAD32
struct ThreadLocalDataSym {
  TypeIndex Type;
  ulittle32_t DataOffset;
  ulittle16_t Segment;
  // Name: The null-terminated name follows.
};

typedef RecordIterator<SymbolRecordKind> SymbolIterator;

inline iterator_range<SymbolIterator> makeSymbolRange(ArrayRef<uint8_t> Data) {
  return make_range(SymbolIterator(Data, nullptr), SymbolIterator());
}

} // namespace codeview
} // namespace llvm

#endif
