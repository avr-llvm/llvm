//===- LoopRotation.h - Loop Rotation -------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides the interface for the Loop Rotation pass.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_SCALAR_LOOPROTATION_H
#define LLVM_TRANSFORMS_SCALAR_LOOPROTATION_H

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

/// A simple loop rotation transformation.
class LoopRotatePass : public PassInfoMixin<LoopRotatePass> {
  unsigned MaxHeaderSize;
public:
  LoopRotatePass();
  LoopRotatePass(unsigned MaxHeaderSize) : MaxHeaderSize(MaxHeaderSize) {}
  PreservedAnalyses run(Loop &L, AnalysisManager<Loop> &AM);
};
}

#endif // LLVM_TRANSFORMS_SCALAR_LOOPROTATION_H
