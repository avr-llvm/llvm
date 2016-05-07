//===-- SIWholeQuadMode.cpp - enter and suspend whole quad mode -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief This pass adds instructions to enable whole quad mode for pixel
/// shaders.
///
/// Whole quad mode is required for derivative computations, but it interferes
/// with shader side effects (stores and atomics). This pass is run on the
/// scheduled machine IR but before register coalescing, so that machine SSA is
/// available for analysis. It ensures that WQM is enabled when necessary, but
/// disabled around stores and atomics.
///
/// When necessary, this pass creates a function prolog
///
///   S_MOV_B64 LiveMask, EXEC
///   S_WQM_B64 EXEC, EXEC
///
/// to enter WQM at the top of the function and surrounds blocks of Exact
/// instructions by
///
///   S_AND_SAVEEXEC_B64 Tmp, LiveMask
///   ...
///   S_MOV_B64 EXEC, Tmp
///
/// In order to avoid excessive switching during sequences of Exact
/// instructions, the pass first analyzes which instructions must be run in WQM
/// (aka which instructions produce values that lead to derivative
/// computations).
///
/// Basic blocks are always exited in WQM as long as some successor needs WQM.
///
/// There is room for improvement given better control flow analysis:
///
///  (1) at the top level (outside of control flow statements, and as long as
///      kill hasn't been used), one SGPR can be saved by recovering WQM from
///      the LiveMask (this is implemented for the entry block).
///
///  (2) when entire regions (e.g. if-else blocks or entire loops) only
///      consist of exact and don't-care instructions, the switch only has to
///      be done at the entry and exit points rather than potentially in each
///      block of the region.
///
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "AMDGPUSubtarget.h"
#include "SIInstrInfo.h"
#include "SIMachineFunctionInfo.h"
#include "llvm/CodeGen/MachineDominanceFrontier.h"
#include "llvm/CodeGen/MachineDominators.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Constants.h"

using namespace llvm;

#define DEBUG_TYPE "si-wqm"

namespace {

enum {
  StateWQM = 0x1,
  StateExact = 0x2,
};

struct InstrInfo {
  char Needs = 0;
  char OutNeeds = 0;
};

struct BlockInfo {
  char Needs = 0;
  char InNeeds = 0;
  char OutNeeds = 0;
};

struct WorkItem {
  const MachineBasicBlock *MBB = nullptr;
  const MachineInstr *MI = nullptr;

  WorkItem() {}
  WorkItem(const MachineBasicBlock *MBB) : MBB(MBB) {}
  WorkItem(const MachineInstr *MI) : MI(MI) {}
};

class SIWholeQuadMode : public MachineFunctionPass {
private:
  const SIInstrInfo *TII;
  const SIRegisterInfo *TRI;
  MachineRegisterInfo *MRI;

  DenseMap<const MachineInstr *, InstrInfo> Instructions;
  DenseMap<const MachineBasicBlock *, BlockInfo> Blocks;
  SmallVector<const MachineInstr *, 2> ExecExports;
  SmallVector<MachineInstr *, 1> LiveMaskQueries;

  char scanInstructions(MachineFunction &MF, std::vector<WorkItem>& Worklist);
  void propagateInstruction(const MachineInstr &MI, std::vector<WorkItem>& Worklist);
  void propagateBlock(const MachineBasicBlock &MBB, std::vector<WorkItem>& Worklist);
  char analyzeFunction(MachineFunction &MF);

  void toExact(MachineBasicBlock &MBB, MachineBasicBlock::iterator Before,
               unsigned SaveWQM, unsigned LiveMaskReg);
  void toWQM(MachineBasicBlock &MBB, MachineBasicBlock::iterator Before,
             unsigned SavedWQM);
  void processBlock(MachineBasicBlock &MBB, unsigned LiveMaskReg, bool isEntry);

  void lowerLiveMaskQueries(unsigned LiveMaskReg);

public:
  static char ID;

  SIWholeQuadMode() :
    MachineFunctionPass(ID) { }

  bool runOnMachineFunction(MachineFunction &MF) override;

  const char *getPassName() const override {
    return "SI Whole Quad Mode";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
};

} // End anonymous namespace

char SIWholeQuadMode::ID = 0;

INITIALIZE_PASS_BEGIN(SIWholeQuadMode, DEBUG_TYPE,
                      "SI Whole Quad Mode", false, false)
INITIALIZE_PASS_END(SIWholeQuadMode, DEBUG_TYPE,
                      "SI Whole Quad Mode", false, false)

char &llvm::SIWholeQuadModeID = SIWholeQuadMode::ID;

FunctionPass *llvm::createSIWholeQuadModePass() {
  return new SIWholeQuadMode;
}

// Scan instructions to determine which ones require an Exact execmask and
// which ones seed WQM requirements.
char SIWholeQuadMode::scanInstructions(MachineFunction &MF,
                                       std::vector<WorkItem> &Worklist) {
  char GlobalFlags = 0;

  for (auto BI = MF.begin(), BE = MF.end(); BI != BE; ++BI) {
    MachineBasicBlock &MBB = *BI;

    for (auto II = MBB.begin(), IE = MBB.end(); II != IE; ++II) {
      MachineInstr &MI = *II;
      unsigned Opcode = MI.getOpcode();
      char Flags;

      if (TII->isWQM(Opcode) || TII->isDS(Opcode)) {
        Flags = StateWQM;
      } else if (TII->get(Opcode).mayStore() &&
                 (MI.getDesc().TSFlags & SIInstrFlags::VM_CNT)) {
        Flags = StateExact;
      } else {
        // Handle export instructions with the exec mask valid flag set
        if (Opcode == AMDGPU::EXP) {
          if (MI.getOperand(4).getImm() != 0)
            ExecExports.push_back(&MI);
        } else if (Opcode == AMDGPU::SI_PS_LIVE) {
          LiveMaskQueries.push_back(&MI);
        }

        continue;
      }

      Instructions[&MI].Needs = Flags;
      Worklist.push_back(&MI);
      GlobalFlags |= Flags;
    }
  }

  return GlobalFlags;
}

void SIWholeQuadMode::propagateInstruction(const MachineInstr &MI,
                                           std::vector<WorkItem>& Worklist) {
  const MachineBasicBlock &MBB = *MI.getParent();
  InstrInfo II = Instructions[&MI]; // take a copy to prevent dangling references
  BlockInfo &BI = Blocks[&MBB];

  // Control flow-type instructions that are followed by WQM computations
  // must themselves be in WQM.
  if ((II.OutNeeds & StateWQM) && !(II.Needs & StateWQM) &&
      (MI.isBranch() || MI.isTerminator() || MI.getOpcode() == AMDGPU::SI_KILL)) {
    Instructions[&MI].Needs = StateWQM;
    II.Needs = StateWQM;
  }

  // Propagate to block level
  BI.Needs |= II.Needs;
  if ((BI.InNeeds | II.Needs) != BI.InNeeds) {
    BI.InNeeds |= II.Needs;
    Worklist.push_back(&MBB);
  }

  // Propagate backwards within block
  if (const MachineInstr *PrevMI = MI.getPrevNode()) {
    char InNeeds = II.Needs | II.OutNeeds;
    if (!PrevMI->isPHI()) {
      InstrInfo &PrevII = Instructions[PrevMI];
      if ((PrevII.OutNeeds | InNeeds) != PrevII.OutNeeds) {
        PrevII.OutNeeds |= InNeeds;
        Worklist.push_back(PrevMI);
      }
    }
  }

  // Propagate WQM flag to instruction inputs
  assert(II.Needs != (StateWQM | StateExact));
  if (II.Needs != StateWQM)
    return;

  for (const MachineOperand &Use : MI.uses()) {
    if (!Use.isReg() || !Use.isUse())
      continue;

    // At this point, physical registers appear as inputs or outputs
    // and following them makes no sense (and would in fact be incorrect
    // when the same VGPR is used as both an output and an input that leads
    // to a NeedsWQM instruction).
    //
    // Note: VCC appears e.g. in 64-bit addition with carry - theoretically we
    // have to trace this, in practice it happens for 64-bit computations like
    // pointers where both dwords are followed already anyway.
    if (!TargetRegisterInfo::isVirtualRegister(Use.getReg()))
      continue;

    for (const MachineOperand &Def : MRI->def_operands(Use.getReg())) {
      const MachineInstr *DefMI = Def.getParent();
      InstrInfo &DefII = Instructions[DefMI];

      // Obviously skip if DefMI is already flagged as NeedWQM.
      //
      // The instruction might also be flagged as NeedExact. This happens when
      // the result of an atomic is used in a WQM computation. In this case,
      // the atomic must not run for helper pixels and the WQM result is
      // undefined.
      if (DefII.Needs != 0)
        continue;

      DefII.Needs = StateWQM;
      Worklist.push_back(DefMI);
    }
  }
}

void SIWholeQuadMode::propagateBlock(const MachineBasicBlock &MBB,
                                     std::vector<WorkItem>& Worklist) {
  BlockInfo BI = Blocks[&MBB]; // take a copy to prevent dangling references

  // Propagate through instructions
  if (!MBB.empty()) {
    const MachineInstr *LastMI = &*MBB.rbegin();
    InstrInfo &LastII = Instructions[LastMI];
    if ((LastII.OutNeeds | BI.OutNeeds) != LastII.OutNeeds) {
      LastII.OutNeeds |= BI.OutNeeds;
      Worklist.push_back(LastMI);
    }
  }

  // Predecessor blocks must provide for our WQM/Exact needs.
  for (const MachineBasicBlock *Pred : MBB.predecessors()) {
    BlockInfo &PredBI = Blocks[Pred];
    if ((PredBI.OutNeeds | BI.InNeeds) == PredBI.OutNeeds)
      continue;

    PredBI.OutNeeds |= BI.InNeeds;
    PredBI.InNeeds |= BI.InNeeds;
    Worklist.push_back(Pred);
  }

  // All successors must be prepared to accept the same set of WQM/Exact
  // data.
  for (const MachineBasicBlock *Succ : MBB.successors()) {
    BlockInfo &SuccBI = Blocks[Succ];
    if ((SuccBI.InNeeds | BI.OutNeeds) == SuccBI.InNeeds)
      continue;

    SuccBI.InNeeds |= BI.OutNeeds;
    Worklist.push_back(Succ);
  }
}

char SIWholeQuadMode::analyzeFunction(MachineFunction &MF) {
  std::vector<WorkItem> Worklist;
  char GlobalFlags = scanInstructions(MF, Worklist);

  while (!Worklist.empty()) {
    WorkItem WI = Worklist.back();
    Worklist.pop_back();

    if (WI.MI)
      propagateInstruction(*WI.MI, Worklist);
    else
      propagateBlock(*WI.MBB, Worklist);
  }

  return GlobalFlags;
}

void SIWholeQuadMode::toExact(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator Before,
                              unsigned SaveWQM, unsigned LiveMaskReg) {
  if (SaveWQM) {
    BuildMI(MBB, Before, DebugLoc(), TII->get(AMDGPU::S_AND_SAVEEXEC_B64),
            SaveWQM)
        .addReg(LiveMaskReg);
  } else {
    BuildMI(MBB, Before, DebugLoc(), TII->get(AMDGPU::S_AND_B64),
            AMDGPU::EXEC)
        .addReg(AMDGPU::EXEC)
        .addReg(LiveMaskReg);
  }
}

void SIWholeQuadMode::toWQM(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator Before,
                            unsigned SavedWQM) {
  if (SavedWQM) {
    BuildMI(MBB, Before, DebugLoc(), TII->get(AMDGPU::COPY), AMDGPU::EXEC)
        .addReg(SavedWQM);
  } else {
    BuildMI(MBB, Before, DebugLoc(), TII->get(AMDGPU::S_WQM_B64),
            AMDGPU::EXEC)
        .addReg(AMDGPU::EXEC);
  }
}

void SIWholeQuadMode::processBlock(MachineBasicBlock &MBB, unsigned LiveMaskReg,
                                   bool isEntry) {
  auto BII = Blocks.find(&MBB);
  if (BII == Blocks.end())
    return;

  const BlockInfo &BI = BII->second;

  if (!(BI.InNeeds & StateWQM))
    return;

  // This is a non-entry block that is WQM throughout, so no need to do
  // anything.
  if (!isEntry && !(BI.Needs & StateExact) && BI.OutNeeds != StateExact)
    return;

  unsigned SavedWQMReg = 0;
  bool WQMFromExec = isEntry;
  char State = isEntry ? StateExact : StateWQM;

  auto II = MBB.getFirstNonPHI(), IE = MBB.end();
  while (II != IE) {
    MachineInstr &MI = *II;
    ++II;

    // Skip instructions that are not affected by EXEC
    if (MI.getDesc().TSFlags & (SIInstrFlags::SALU | SIInstrFlags::SMRD) &&
        !MI.isBranch() && !MI.isTerminator())
      continue;

    // Generic instructions such as COPY will either disappear by register
    // coalescing or be lowered to SALU or VALU instructions.
    if (TargetInstrInfo::isGenericOpcode(MI.getOpcode())) {
      if (MI.getNumExplicitOperands() >= 1) {
        const MachineOperand &Op = MI.getOperand(0);
        if (Op.isReg()) {
          if (TRI->isSGPRReg(*MRI, Op.getReg())) {
            // SGPR instructions are not affected by EXEC
            continue;
          }
        }
      }
    }

    char Needs = 0;
    char OutNeeds = 0;
    auto InstrInfoIt = Instructions.find(&MI);
    if (InstrInfoIt != Instructions.end()) {
      Needs = InstrInfoIt->second.Needs;
      OutNeeds = InstrInfoIt->second.OutNeeds;

      // Make sure to switch to Exact mode before the end of the block when
      // Exact and only Exact is needed further downstream.
      if (OutNeeds == StateExact && (MI.isBranch() || MI.isTerminator())) {
        assert(Needs == 0);
        Needs = StateExact;
      }
    }

    // State switching
    if (Needs && State != Needs) {
      if (Needs == StateExact) {
        assert(!SavedWQMReg);

        if (!WQMFromExec && (OutNeeds & StateWQM))
          SavedWQMReg = MRI->createVirtualRegister(&AMDGPU::SReg_64RegClass);

        toExact(MBB, &MI, SavedWQMReg, LiveMaskReg);
      } else {
        assert(WQMFromExec == (SavedWQMReg == 0));
        toWQM(MBB, &MI, SavedWQMReg);
        SavedWQMReg = 0;
      }

      State = Needs;
    }

    if (MI.getOpcode() == AMDGPU::SI_KILL)
      WQMFromExec = false;
  }

  if ((BI.OutNeeds & StateWQM) && State != StateWQM) {
    assert(WQMFromExec == (SavedWQMReg == 0));
    toWQM(MBB, MBB.end(), SavedWQMReg);
  } else if (BI.OutNeeds == StateExact && State != StateExact) {
    toExact(MBB, MBB.end(), 0, LiveMaskReg);
  }
}

void SIWholeQuadMode::lowerLiveMaskQueries(unsigned LiveMaskReg) {
  for (MachineInstr *MI : LiveMaskQueries) {
    DebugLoc DL = MI->getDebugLoc();
    unsigned Dest = MI->getOperand(0).getReg();
    BuildMI(*MI->getParent(), MI, DL, TII->get(AMDGPU::COPY), Dest)
        .addReg(LiveMaskReg);
    MI->eraseFromParent();
  }
}

bool SIWholeQuadMode::runOnMachineFunction(MachineFunction &MF) {
  if (MF.getFunction()->getCallingConv() != CallingConv::AMDGPU_PS)
    return false;

  Instructions.clear();
  Blocks.clear();
  ExecExports.clear();
  LiveMaskQueries.clear();

  TII = static_cast<const SIInstrInfo *>(MF.getSubtarget().getInstrInfo());
  TRI = static_cast<const SIRegisterInfo *>(MF.getSubtarget().getRegisterInfo());
  MRI = &MF.getRegInfo();

  char GlobalFlags = analyzeFunction(MF);
  if (!(GlobalFlags & StateWQM)) {
    lowerLiveMaskQueries(AMDGPU::EXEC);
    return !LiveMaskQueries.empty();
  }

  // Store a copy of the original live mask when required
  MachineBasicBlock &Entry = MF.front();
  MachineInstr *EntryMI = Entry.getFirstNonPHI();
  unsigned LiveMaskReg = 0;

  if (GlobalFlags & StateExact || !LiveMaskQueries.empty()) {
    LiveMaskReg = MRI->createVirtualRegister(&AMDGPU::SReg_64RegClass);
    BuildMI(Entry, EntryMI, DebugLoc(), TII->get(AMDGPU::COPY), LiveMaskReg)
        .addReg(AMDGPU::EXEC);
  }

  if (GlobalFlags == StateWQM) {
    // For a shader that needs only WQM, we can just set it once.
    BuildMI(Entry, EntryMI, DebugLoc(), TII->get(AMDGPU::S_WQM_B64),
            AMDGPU::EXEC).addReg(AMDGPU::EXEC);

    lowerLiveMaskQueries(LiveMaskReg);
    // EntryMI may become invalid here
    return true;
  }

  lowerLiveMaskQueries(LiveMaskReg);
  EntryMI = nullptr;

  // Handle the general case
  for (const auto &BII : Blocks)
    processBlock(const_cast<MachineBasicBlock &>(*BII.first), LiveMaskReg,
                 BII.first == &*MF.begin());

  return true;
}
