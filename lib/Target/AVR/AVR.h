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

/**
 * \mainpage AVR-LLVM Documentation
 *
 * AVR-LLVM adds support for the Atmel AVR microcontroller
 * architecture to LLVM.
 *
 * This enables the use of AVR in different compiler projects.
 *
 * AVR-specific code can be found inside `lib/Target/AVR`.
 *
 * Links:
 * * [GitHub](https://github.com/avr-llvm/llvm)
 * * [Wiki](https://github.com/avr-llvm/llvm/wiki)
 * * [Mailing lists](http://lists.avr-llvm.org/mailman/listinfo)
 */

#ifndef LLVM_AVR_H
# define LLVM_AVR_H

# include "AVRConfig.h"

# include "llvm/Target/TargetMachine.h"
# include "llvm/CodeGen/SelectionDAGNodes.h"

# include "MCTargetDesc/AVRMCTargetDesc.h"

namespace llvm {

class AVRTargetMachine;
class FunctionPass;

FunctionPass *createAVRISelDag(AVRTargetMachine &TM,
                               CodeGenOpt::Level OptLevel);
FunctionPass *createAVRExpandPseudoPass();
FunctionPass *createAVRFrameAnalyzerPass();
FunctionPass *createAVRDynAllocaSRPass();
FunctionPass *createAVRBranchSelectionPass();


namespace AVR {

enum AddressSpace {
  DataMemory,
  ProgramMemory
};

template <typename T>
bool
isProgramMemoryAddress(T * V) {
  return cast<PointerType>(V->getType())->getAddressSpace() == ProgramMemory;
}

inline
bool
isProgramMemoryAccess(MemSDNode const* N) {
  auto V = N->getMemOperand()->getValue();

  return (V != nullptr) ? isProgramMemoryAddress(V) : false;
}

}  // end of namespace AVR

} // end namespace llvm

#endif // LLVM_AVR_H
