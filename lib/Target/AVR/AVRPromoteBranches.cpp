//===------------------------ AVRPromoteBranches.cpp ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
// This pass promotes small branches with targets which are too far away
// into bigger branches with a landing pad inbetween.

#include "AVR.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "AVRTargetMachine.h"

using namespace llvm;

#define DEBUG_TYPE "avr-promote-branches"

namespace {
  /// Promotes small branches which point to blocks
  /// that are too far away into branch instructions
  /// which can point to a wide range of addresses.
  class AVRPromoteBranches : public MachineFunctionPass {
  public:
    static char ID;

    AVRPromoteBranches(AVRTargetMachine &TM) :
      MachineFunctionPass(ID), TM(TM) { }

    bool runOnMachineFunction(MachineFunction &F) override;

    const char *getPassName() const override {
      return "AVR Promote Branches";
    }

  private:
    AVRTargetMachine &TM;
  };

  char AVRPromoteBranches::ID = 0;
} // end anonymous namespace

bool AVRPromoteBranches::runOnMachineFunction(MachineFunction &MF) {
  void((this->TM));
  return false;
}

FunctionPass *llvm::createAVRPromoteBranchesPass(AVRTargetMachine &TM) {
  return new AVRPromoteBranches(TM);
}

