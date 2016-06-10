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
#include "llvm/MC/MCSymbol.h"
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

static int64_t DistanceToSymbol(MachineInstr &MI, MCSymbol &Sym) {
  // TODO: handle relative targets
  return Sym.getOffset();
}

static int64_t DistanceToTarget(MachineInstr &MI,
                                MachineOperand &Target,
                                bool IsRelative) {
  if (Target.isImm()) {
    return Target.getImm();
  } else if (Target.isMBB()) {
    return DistanceToSymbol(MI, *Target.getMBB()->getSymbol());
  } else if (Target.isSymbol()) {
    return DistanceToSymbol(MI, *Target.getMCSymbol());
  } else {
    llvm_unreachable("don't know how to handle this branch target");
  }

  return 0;
}

static void ProcessBranch(MachineInstr &MI,
                          unsigned OpNo,
                          int64_t Min,
                          int64_t Max,
                          unsigned WiderOpcode,
                          bool &Modified,
                          bool IsRelative) {
  MachineOperand &Target = MI.getOperand(OpNo);

  auto Distance = DistanceToTarget(MI, Target, IsRelative);

  if (Distance < Min || Distance > Max) {
    MCInstrDesc Desc = MI.getDesc();

    Desc.Opcode = WiderOpcode;
    MI.setDesc(Desc);

    if (!Modified) Modified = true;
  }
}

static void ProcessRelativeBranch(MachineInstr &MI,
                                  unsigned OpNo,
                                  unsigned TargetWidth,
                                  unsigned WiderOpcode,
                                  bool &Modified) {
  ProcessBranch(MI, OpNo, minIntN(TargetWidth),
                maxIntN(TargetWidth), WiderOpcode, Modified, true);
}

static void ProcessAbsoluteBranch(MachineInstr &MI,
                                  unsigned OpNo,
                                  unsigned TargetWidth,
                                  unsigned WiderOpcode,
                                  bool &Modified) {
  ProcessBranch(MI, OpNo, 0, maxUIntN(TargetWidth),
                WiderOpcode, Modified, false);
}

bool AVRPromoteBranches::runOnMachineFunction(MachineFunction &MF) {
  void((this->TM));

  bool Modified = false;

  for (MachineBasicBlock &BB : MF) {
    for (MachineInstr &MI : BB) {
      switch (MI.getOpcode()) {
      case AVR::RJMPk:
        ProcessRelativeBranch(MI, 0, 12, AVR::JMPk, Modified); break;
      case AVR::RCALLk:
        ProcessAbsoluteBranch(MI, 0, 12, AVR::CALLk, Modified); break;
      case AVR::BREQk:
      case AVR::BRNEk:
      case AVR::BRSHk:
      case AVR::BRLOk:
      case AVR::BRMIk:
      case AVR::BRPLk:
      case AVR::BRGEk:
      case AVR::BRLTk:
        ProcessRelativeBranch(MI, 0, 7, AVR::RJMPk, Modified); break;
      default: break;
      }
    }
  }

  return false;
}

FunctionPass *llvm::createAVRPromoteBranchesPass(AVRTargetMachine &TM) {
  return new AVRPromoteBranches(TM);
}

