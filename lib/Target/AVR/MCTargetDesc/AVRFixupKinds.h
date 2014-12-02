//===-- MipsFixupKinds.h - AVR Specific Fixup Entries ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR_MCTARGETDESC_AVRFIXUPKINDS_H
#define LLVM_LIB_TARGET_AVR_MCTARGETDESC_AVRFIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace AVR {
  // Although most of the current fixup types reflect a unique relocation
  // one can have multiple fixup types for a given relocation and thus need
  // to be uniquely named.
  //
  // This table *must* be in the same order of
  // MCFixupKindInfo Infos[AVR::NumTargetFixupKinds]
  // in MipsAsmBackend.cpp.
  //
  enum Fixups {
    // Branch fixups resulting in R_MIPS_16.
    fixup_brcond = FirstTargetFixupKind,
    fixup_rel_condbr_7,

    // Marker
    LastTargetFixupKind,
    NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
  };
} // namespace AVR
} // namespace llvm


#endif
