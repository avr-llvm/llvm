//===- llvm/CodeGen/GlobalISel/InstructionSelect.cpp - InstructionSelect ---==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the InstructionSelect class.
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/GlobalISel/InstructionSelect.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/Twine.h"
#include "llvm/CodeGen/GlobalISel/InstructionSelector.h"
#include "llvm/CodeGen/GlobalISel/LegalizerInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetSubtargetInfo.h"

#define DEBUG_TYPE "instruction-select"

using namespace llvm;

char InstructionSelect::ID = 0;
INITIALIZE_PASS_BEGIN(InstructionSelect, DEBUG_TYPE,
                      "Select target instructions out of generic instructions",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
INITIALIZE_PASS_END(InstructionSelect, DEBUG_TYPE,
                    "Select target instructions out of generic instructions",
                    false, false)

InstructionSelect::InstructionSelect() : MachineFunctionPass(ID) {
  initializeInstructionSelectPass(*PassRegistry::getPassRegistry());
}

void InstructionSelect::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  MachineFunctionPass::getAnalysisUsage(AU);
}

static void reportSelectionError(const MachineInstr &MI, const Twine &Message) {
  const MachineFunction &MF = *MI.getParent()->getParent();
  std::string ErrStorage;
  raw_string_ostream Err(ErrStorage);
  Err << Message << ":\nIn function: " << MF.getName() << '\n' << MI << '\n';
  report_fatal_error(Err.str());
}

bool InstructionSelect::runOnMachineFunction(MachineFunction &MF) {
  // If the ISel pipeline failed, do not bother running that pass.
  if (MF.getProperties().hasProperty(
          MachineFunctionProperties::Property::FailedISel))
    return false;

  DEBUG(dbgs() << "Selecting function: " << MF.getName() << '\n');

  const TargetPassConfig &TPC = getAnalysis<TargetPassConfig>();
  const InstructionSelector *ISel = MF.getSubtarget().getInstructionSelector();
  assert(ISel && "Cannot work without InstructionSelector");

  // FIXME: freezeReservedRegs is now done in IRTranslator, but there are many
  // other MF/MFI fields we need to initialize.

#ifndef NDEBUG
  // Check that our input is fully legal: we require the function to have the
  // Legalized property, so it should be.
  // FIXME: This should be in the MachineVerifier, but it can't use the
  // LegalizerInfo as it's currently in the separate GlobalISel library.
  // The RegBankSelected property is already checked in the verifier. Note
  // that it has the same layering problem, but we only use inline methods so
  // end up not needing to link against the GlobalISel library.
  const MachineRegisterInfo &MRI = MF.getRegInfo();
  if (const LegalizerInfo *MLI = MF.getSubtarget().getLegalizerInfo())
    for (const MachineBasicBlock &MBB : MF)
      for (const MachineInstr &MI : MBB)
        if (isPreISelGenericOpcode(MI.getOpcode()) && !MLI->isLegal(MI, MRI))
          reportSelectionError(MI, "Instruction is not legal");

#endif
  // FIXME: We could introduce new blocks and will need to fix the outer loop.
  // Until then, keep track of the number of blocks to assert that we don't.
  const size_t NumBlocks = MF.size();

  bool Failed = false;
  for (MachineBasicBlock *MBB : post_order(&MF)) {
    if (MBB->empty())
      continue;

    // Select instructions in reverse block order. We permit erasing so have
    // to resort to manually iterating and recognizing the begin (rend) case.
    bool ReachedBegin = false;
    for (auto MII = std::prev(MBB->end()), Begin = MBB->begin();
         !ReachedBegin;) {
      // Keep track of the insertion range for debug printing.
      const auto AfterIt = std::next(MII);

      // Select this instruction.
      MachineInstr &MI = *MII;

      // And have our iterator point to the next instruction, if there is one.
      if (MII == Begin)
        ReachedBegin = true;
      else
        --MII;

      DEBUG(dbgs() << "Selecting: \n  " << MI);

      if (!ISel->select(MI)) {
        if (TPC.isGlobalISelAbortEnabled())
          reportSelectionError(MI, "Cannot select");
        Failed = true;
        break;
      }

      // Dump the range of instructions that MI expanded into.
      DEBUG({
        auto InsertedBegin = ReachedBegin ? MBB->begin() : std::next(MII);
        dbgs() << "Into:\n";
        for (auto &InsertedMI : make_range(InsertedBegin, AfterIt))
          dbgs() << "  " << InsertedMI;
        dbgs() << '\n';
      });
    }
  }

  if (!TPC.isGlobalISelAbortEnabled() && (Failed || MF.size() == NumBlocks)) {
    MF.getProperties().set(MachineFunctionProperties::Property::FailedISel);
    return false;
  }
  assert(MF.size() == NumBlocks && "Inserting blocks is not supported yet");

  // Now that selection is complete, there are no more generic vregs.
  // FIXME: We're still discussing what to do with the vreg->size map:
  // it's somewhat redundant (with the def MIs type size), but having to
  // examine MIs is also awkward.  Another alternative is to track the type on
  // the vreg instead, but that's not ideal either, because it's saying that
  // vregs have types, which they really don't. But then again, LLT is just
  // a size and a "shape": it's probably the same information as regbank info.
  MF.getRegInfo().clearVirtRegTypes();

  // FIXME: Should we accurately track changes?
  return true;
}
