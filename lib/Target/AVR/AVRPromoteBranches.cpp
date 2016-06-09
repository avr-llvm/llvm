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
#include "AVRInstrInfo.h"
#include "AVRTargetMachine.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Support/MathExtras.h"


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

static bool ProcessBranch(MachineInstr &MI,
                          int64_t Min,
                          int64_t Max,
                          unsigned WiderOpcode,
                          bool &Modified) {
  return false;
}

static bool ProcessRelativeBranch(MachineInstr &MI,
                                  unsigned TargetWidth,
                                  unsigned WiderOpcode,
                                  bool &Modified) {
  return ProcessBranch(MI, minIntN(TargetWidth),
                       maxIntN(TargetWidth), WiderOpcode, Modified);
}

static bool ProcessAbsoluteBranch(MachineInstr &MI,
                                  unsigned TargetWidth,
                                  unsigned WiderOpcode,
                                  bool &Modified) {
  return ProcessBranch(MI, 0, maxUIntN(TargetWidth),
                       WiderOpcode, Modified);
}

bool AVRPromoteBranches::runOnMachineFunction(MachineFunction &MF) {
  void((this->TM));

  bool Modified = false;

  for (MachineBasicBlock &BB : MF) {
    for (MachineInstr &MI : BB) {
      switch (MI.getOpcode()) {
      case AVR::RJMPk:
        ProcessRelativeBranch(MI, 12, AVR::JMPk, Modified); break;
      case AVR::RCALLk:
        ProcessAbsoluteBranch(MI, 12, AVR::CALLk, Modified); break;
      case AVR::BREQk:
      case AVR::BRNEk:
      case AVR::BRSHk:
      case AVR::BRLOk:
      case AVR::BRMIk:
      case AVR::BRPLk:
      case AVR::BRGEk:
      case AVR::BRLTk:
        ProcessRelativeBranch(MI, 7, AVR::RJMPk, Modified); break;
      default: break;
      }
    }
  }

  return false;
}

FunctionPass *llvm::createAVRPromoteBranchesPass(AVRTargetMachine &TM) {
  return new AVRPromoteBranches(TM);
}

