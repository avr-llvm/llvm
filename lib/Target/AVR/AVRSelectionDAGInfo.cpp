//===-- AVRSelectionDAGInfo.cpp - AVR SelectionDAG Info -------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the AVRSelectionDAGInfo class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "avr-selectiondag-info"

#include "AVRSelectionDAGInfo.h"

namespace llvm {

AVRSelectionDAGInfo::AVRSelectionDAGInfo(const DataLayout &DL) :
  TargetSelectionDAGInfo(&DL) {}

} // end of namespace llvm
