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
  // TODO: Document each fixup
  enum Fixups {
    fixup_32 = FirstTargetFixupKind,

    fixup_7_pcrel,
    fixup_13_pcrel,

    fixup_16,
    fixup_16_pm,

    fixup_lo8_ldi,
    fixup_hi8_ldi,
    fixup_hh8_ldi,

    fixup_lo8_ldi_neg,
    fixup_hi8_ldi_neg,
    fixup_hh8_ldi_neg,

    fixup_lo8_ldi_pm,
    fixup_hi8_ldi_pm,
    fixup_hh8_ldi_pm,

    fixup_lo8_ldi_pm_neg,
    fixup_hi8_ldi_pm_neg,
    fixup_hh8_ldi_pm_neg,

    fixup_call,
    fixup_ldi,

    fixup_6,
    fixup_6_adiw,

    fixup_ms8_ldi,
    fixup_ms8_ldi_neg,

    fixup_lo8_ldi_gs,
    fixup_hi8_ldi_gs,

    fixup_8,
    fixup_8_lo8,
    fixup_8_hi8,
    fixup_8_hlo8,

    fixup_sym_diff,
    fixup_16_ldst,

    fixup_lds_sts_16,

    fixup_port6,
    fixup_port5,

    // Marker
    LastTargetFixupKind,
    NumTargetFixupKinds = LastTargetFixupKind - FirstTargetFixupKind
  };
} // namespace AVR
} // namespace llvm


#endif
