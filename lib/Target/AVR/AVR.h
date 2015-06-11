//===-- AVR.h - Top-level interface for AVR representation ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// AVR back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_AVR_H
# define LLVM_AVR_H

# include "MCTargetDesc/AVRMCTargetDesc.h"
# include "llvm/Target/TargetMachine.h"

namespace llvm {

class AVRTargetMachine;
class FunctionPass;

FunctionPass *createAVRISelDag(AVRTargetMachine &TM,
                               CodeGenOpt::Level OptLevel);
FunctionPass *createAVRExpandPseudoPass();
FunctionPass *createAVRFrameAnalyzerPass();
FunctionPass *createAVRDynAllocaSRPass();
FunctionPass *createAVRBranchSelectionPass();

} // end namespace llvm

#endif // LLVM_AVR_H
