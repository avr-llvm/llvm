//===-- AVRSelectionDAGInfo.h - AVR SelectionDAG Info -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the AVR subclass for TargetSelectionDAGInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_AVR_SELECTION_DAG_INFO_H
# define LLVM_AVR_SELECTION_DAG_INFO_H

# include "AVRConfig.h"

# include "llvm/Target/TargetSelectionDAGInfo.h"

namespace llvm {

class AVRTargetMachine;

class AVRSelectionDAGInfo : public TargetSelectionDAGInfo {
public:
  explicit AVRSelectionDAGInfo(const DataLayout &DL);
};

} // end namespace llvm

#endif // LLVM_AVR_SELECTION_DAG_INFO_H
