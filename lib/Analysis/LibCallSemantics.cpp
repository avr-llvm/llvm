//===- LibCallSemantics.cpp - Describe library semantics ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements interfaces that can be used to describe language
// specific runtime library interfaces (e.g. libc, libm, etc) to LLVM
// optimizers.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/LibCallSemantics.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/IR/Function.h"
using namespace llvm;

/// See if the given exception handling personality function is one that we
/// understand.  If so, return a description of it; otherwise return Unknown.
EHPersonality llvm::classifyEHPersonality(const Value *Pers) {
  const Function *F = dyn_cast<Function>(Pers->stripPointerCasts());
  if (!F)
    return EHPersonality::Unknown;
  return StringSwitch<EHPersonality>(F->getName())
    .Case("__gnat_eh_personality", EHPersonality::GNU_Ada)
    .Case("__gxx_personality_v0",  EHPersonality::GNU_CXX)
    .Case("__gcc_personality_v0",  EHPersonality::GNU_C)
    .Case("__objc_personality_v0", EHPersonality::GNU_ObjC)
    .Case("_except_handler3",      EHPersonality::MSVC_X86SEH)
    .Case("_except_handler4",      EHPersonality::MSVC_X86SEH)
    .Case("__C_specific_handler",  EHPersonality::MSVC_Win64SEH)
    .Case("__CxxFrameHandler3",    EHPersonality::MSVC_CXX)
    .Default(EHPersonality::Unknown);
}

bool llvm::canSimplifyInvokeNoUnwind(const Function *F) {
  EHPersonality Personality = classifyEHPersonality(F->getPersonalityFn());
  // We can't simplify any invokes to nounwind functions if the personality
  // function wants to catch asynch exceptions.  The nounwind attribute only
  // implies that the function does not throw synchronous exceptions.
  return !isAsynchronousEHPersonality(Personality);
}
