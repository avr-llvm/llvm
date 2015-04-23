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

#ifndef __INCLUDE_AVRSELECTIONDAGINFO_H__
#define __INCLUDE_AVRSELECTIONDAGINFO_H__

#include "llvm/Target/TargetSelectionDAGInfo.h"

namespace llvm
{

class AVRTargetMachine;

class AVRSelectionDAGInfo : public TargetSelectionDAGInfo
{
public:
  explicit AVRSelectionDAGInfo(const AVRTargetMachine &TM);
};

} // end namespace llvm

#endif //__INCLUDE_AVRSELECTIONDAGINFO_H__
