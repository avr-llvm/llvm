//===- LibCallSemantics.h - Describe library semantics --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines interfaces that can be used to describe language specific
// runtime library interfaces (e.g. libc, libm, etc) to LLVM optimizers.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_LIBCALLSEMANTICS_H
#define LLVM_ANALYSIS_LIBCALLSEMANTICS_H

#include "llvm/Analysis/AliasAnalysis.h"

namespace llvm {
class InvokeInst;

  enum class EHPersonality {
    Unknown,
    GNU_Ada,
    GNU_C,
    GNU_CXX,
    GNU_ObjC,
    MSVC_X86SEH,
    MSVC_Win64SEH,
    MSVC_CXX,
  };

  /// \brief See if the given exception handling personality function is one
  /// that we understand.  If so, return a description of it; otherwise return
  /// Unknown.
  EHPersonality classifyEHPersonality(const Value *Pers);

  /// \brief Returns true if this personality function catches asynchronous
  /// exceptions.
  inline bool isAsynchronousEHPersonality(EHPersonality Pers) {
    // The two SEH personality functions can catch asynch exceptions. We assume
    // unknown personalities don't catch asynch exceptions.
    switch (Pers) {
    case EHPersonality::MSVC_X86SEH:
    case EHPersonality::MSVC_Win64SEH:
      return true;
    default: return false;
    }
    llvm_unreachable("invalid enum");
  }

  /// \brief Returns true if this is an MSVC personality function.
  inline bool isMSVCEHPersonality(EHPersonality Pers) {
    // The two SEH personality functions can catch asynch exceptions. We assume
    // unknown personalities don't catch asynch exceptions.
    switch (Pers) {
    case EHPersonality::MSVC_CXX:
    case EHPersonality::MSVC_X86SEH:
    case EHPersonality::MSVC_Win64SEH:
      return true;
    default: return false;
    }
    llvm_unreachable("invalid enum");
  }

  /// \brief Return true if this personality may be safely removed if there
  /// are no invoke instructions remaining in the current function.
  inline bool isNoOpWithoutInvoke(EHPersonality Pers) {
    switch (Pers) {
    case EHPersonality::Unknown:
      return false;
    // All known personalities currently have this behavior
    default: return true;
    }
    llvm_unreachable("invalid enum");
  }

  bool canSimplifyInvokeNoUnwind(const Function *F);

} // end namespace llvm

#endif
