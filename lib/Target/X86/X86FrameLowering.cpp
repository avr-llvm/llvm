//===-- X86FrameLowering.cpp - X86 Frame Information ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the X86 implementation of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#include "X86FrameLowering.h"
#include "X86InstrBuilder.h"
#include "X86InstrInfo.h"
#include "X86MachineFunctionInfo.h"
#include "X86Subtarget.h"
#include "X86TargetMachine.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/WinEHFuncInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/Debug.h"
#include <cstdlib>

using namespace llvm;

X86FrameLowering::X86FrameLowering(const X86Subtarget &STI,
                                   unsigned StackAlignOverride)
    : TargetFrameLowering(StackGrowsDown, StackAlignOverride,
                          STI.is64Bit() ? -8 : -4),
      STI(STI), TII(*STI.getInstrInfo()), TRI(STI.getRegisterInfo()) {
  // Cache a bunch of frame-related predicates for this subtarget.
  SlotSize = TRI->getSlotSize();
  Is64Bit = STI.is64Bit();
  IsLP64 = STI.isTarget64BitLP64();
  // standard x86_64 and NaCl use 64-bit frame/stack pointers, x32 - 32-bit.
  Uses64BitFramePtr = STI.isTarget64BitLP64() || STI.isTargetNaCl64();
  StackPtr = TRI->getStackRegister();
}

bool X86FrameLowering::hasReservedCallFrame(const MachineFunction &MF) const {
  return !MF.getFrameInfo()->hasVarSizedObjects() &&
         !MF.getInfo<X86MachineFunctionInfo>()->getHasPushSequences();
}

/// canSimplifyCallFramePseudos - If there is a reserved call frame, the
/// call frame pseudos can be simplified.  Having a FP, as in the default
/// implementation, is not sufficient here since we can't always use it.
/// Use a more nuanced condition.
bool
X86FrameLowering::canSimplifyCallFramePseudos(const MachineFunction &MF) const {
  return hasReservedCallFrame(MF) ||
         (hasFP(MF) && !TRI->needsStackRealignment(MF)) ||
         TRI->hasBasePointer(MF);
}

// needsFrameIndexResolution - Do we need to perform FI resolution for
// this function. Normally, this is required only when the function
// has any stack objects. However, FI resolution actually has another job,
// not apparent from the title - it resolves callframesetup/destroy 
// that were not simplified earlier.
// So, this is required for x86 functions that have push sequences even
// when there are no stack objects.
bool
X86FrameLowering::needsFrameIndexResolution(const MachineFunction &MF) const {
  return MF.getFrameInfo()->hasStackObjects() ||
         MF.getInfo<X86MachineFunctionInfo>()->getHasPushSequences();
}

/// hasFP - Return true if the specified function should have a dedicated frame
/// pointer register.  This is true if the function has variable sized allocas
/// or if frame pointer elimination is disabled.
bool X86FrameLowering::hasFP(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  const MachineModuleInfo &MMI = MF.getMMI();

  return (MF.getTarget().Options.DisableFramePointerElim(MF) ||
          TRI->needsStackRealignment(MF) ||
          MFI->hasVarSizedObjects() ||
          MFI->isFrameAddressTaken() || MFI->hasOpaqueSPAdjustment() ||
          MF.getInfo<X86MachineFunctionInfo>()->getForceFramePointer() ||
          MMI.callsUnwindInit() || MMI.hasEHFunclets() || MMI.callsEHReturn() ||
          MFI->hasStackMap() || MFI->hasPatchPoint());
}

static unsigned getSUBriOpcode(unsigned IsLP64, int64_t Imm) {
  if (IsLP64) {
    if (isInt<8>(Imm))
      return X86::SUB64ri8;
    return X86::SUB64ri32;
  } else {
    if (isInt<8>(Imm))
      return X86::SUB32ri8;
    return X86::SUB32ri;
  }
}

static unsigned getADDriOpcode(unsigned IsLP64, int64_t Imm) {
  if (IsLP64) {
    if (isInt<8>(Imm))
      return X86::ADD64ri8;
    return X86::ADD64ri32;
  } else {
    if (isInt<8>(Imm))
      return X86::ADD32ri8;
    return X86::ADD32ri;
  }
}

static unsigned getSUBrrOpcode(unsigned isLP64) {
  return isLP64 ? X86::SUB64rr : X86::SUB32rr;
}

static unsigned getADDrrOpcode(unsigned isLP64) {
  return isLP64 ? X86::ADD64rr : X86::ADD32rr;
}

static unsigned getANDriOpcode(bool IsLP64, int64_t Imm) {
  if (IsLP64) {
    if (isInt<8>(Imm))
      return X86::AND64ri8;
    return X86::AND64ri32;
  }
  if (isInt<8>(Imm))
    return X86::AND32ri8;
  return X86::AND32ri;
}

static unsigned getLEArOpcode(unsigned IsLP64) {
  return IsLP64 ? X86::LEA64r : X86::LEA32r;
}

/// findDeadCallerSavedReg - Return a caller-saved register that isn't live
/// when it reaches the "return" instruction. We can then pop a stack object
/// to this register without worry about clobbering it.
static unsigned findDeadCallerSavedReg(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator &MBBI,
                                       const TargetRegisterInfo *TRI,
                                       bool Is64Bit) {
  const MachineFunction *MF = MBB.getParent();
  const Function *F = MF->getFunction();
  if (!F || MF->getMMI().callsEHReturn())
    return 0;

  static const uint16_t CallerSavedRegs32Bit[] = {
    X86::EAX, X86::EDX, X86::ECX, 0
  };

  static const uint16_t CallerSavedRegs64Bit[] = {
    X86::RAX, X86::RDX, X86::RCX, X86::RSI, X86::RDI,
    X86::R8,  X86::R9,  X86::R10, X86::R11, 0
  };

  unsigned Opc = MBBI->getOpcode();
  switch (Opc) {
  default: return 0;
  case X86::RETL:
  case X86::RETQ:
  case X86::RETIL:
  case X86::RETIQ:
  case X86::TCRETURNdi:
  case X86::TCRETURNri:
  case X86::TCRETURNmi:
  case X86::TCRETURNdi64:
  case X86::TCRETURNri64:
  case X86::TCRETURNmi64:
  case X86::EH_RETURN:
  case X86::EH_RETURN64: {
    SmallSet<uint16_t, 8> Uses;
    for (unsigned i = 0, e = MBBI->getNumOperands(); i != e; ++i) {
      MachineOperand &MO = MBBI->getOperand(i);
      if (!MO.isReg() || MO.isDef())
        continue;
      unsigned Reg = MO.getReg();
      if (!Reg)
        continue;
      for (MCRegAliasIterator AI(Reg, TRI, true); AI.isValid(); ++AI)
        Uses.insert(*AI);
    }

    const uint16_t *CS = Is64Bit ? CallerSavedRegs64Bit : CallerSavedRegs32Bit;
    for (; *CS; ++CS)
      if (!Uses.count(*CS))
        return *CS;
  }
  }

  return 0;
}

static bool isEAXLiveIn(MachineFunction &MF) {
  for (MachineRegisterInfo::livein_iterator II = MF.getRegInfo().livein_begin(),
       EE = MF.getRegInfo().livein_end(); II != EE; ++II) {
    unsigned Reg = II->first;

    if (Reg == X86::RAX || Reg == X86::EAX || Reg == X86::AX ||
        Reg == X86::AH || Reg == X86::AL)
      return true;
  }

  return false;
}

/// Check whether or not the terminators of \p MBB needs to read EFLAGS.
static bool terminatorsNeedFlagsAsInput(const MachineBasicBlock &MBB) {
  for (const MachineInstr &MI : MBB.terminators()) {
    bool BreakNext = false;
    for (const MachineOperand &MO : MI.operands()) {
      if (!MO.isReg())
        continue;
      unsigned Reg = MO.getReg();
      if (Reg != X86::EFLAGS)
        continue;

      // This terminator needs an eflag that is not defined
      // by a previous terminator.
      if (!MO.isDef())
        return true;
      BreakNext = true;
    }
    if (BreakNext)
      break;
  }
  return false;
}

/// emitSPUpdate - Emit a series of instructions to increment / decrement the
/// stack pointer by a constant value.
void X86FrameLowering::emitSPUpdate(MachineBasicBlock &MBB,
                                    MachineBasicBlock::iterator &MBBI,
                                    int64_t NumBytes, bool InEpilogue) const {
  bool isSub = NumBytes < 0;
  uint64_t Offset = isSub ? -NumBytes : NumBytes;

  uint64_t Chunk = (1LL << 31) - 1;
  DebugLoc DL = MBB.findDebugLoc(MBBI);

  while (Offset) {
    if (Offset > Chunk) {
      // Rather than emit a long series of instructions for large offsets,
      // load the offset into a register and do one sub/add
      unsigned Reg = 0;

      if (isSub && !isEAXLiveIn(*MBB.getParent()))
        Reg = (unsigned)(Is64Bit ? X86::RAX : X86::EAX);
      else
        Reg = findDeadCallerSavedReg(MBB, MBBI, TRI, Is64Bit);

      if (Reg) {
        unsigned Opc = Is64Bit ? X86::MOV64ri : X86::MOV32ri;
        BuildMI(MBB, MBBI, DL, TII.get(Opc), Reg)
          .addImm(Offset);
        Opc = isSub
          ? getSUBrrOpcode(Is64Bit)
          : getADDrrOpcode(Is64Bit);
        MachineInstr *MI = BuildMI(MBB, MBBI, DL, TII.get(Opc), StackPtr)
          .addReg(StackPtr)
          .addReg(Reg);
        MI->getOperand(3).setIsDead(); // The EFLAGS implicit def is dead.
        Offset = 0;
        continue;
      }
    }

    uint64_t ThisVal = std::min(Offset, Chunk);
    if (ThisVal == (Is64Bit ? 8 : 4)) {
      // Use push / pop instead.
      unsigned Reg = isSub
        ? (unsigned)(Is64Bit ? X86::RAX : X86::EAX)
        : findDeadCallerSavedReg(MBB, MBBI, TRI, Is64Bit);
      if (Reg) {
        unsigned Opc = isSub
          ? (Is64Bit ? X86::PUSH64r : X86::PUSH32r)
          : (Is64Bit ? X86::POP64r  : X86::POP32r);
        MachineInstr *MI = BuildMI(MBB, MBBI, DL, TII.get(Opc))
          .addReg(Reg, getDefRegState(!isSub) | getUndefRegState(isSub));
        if (isSub)
          MI->setFlag(MachineInstr::FrameSetup);
        else
          MI->setFlag(MachineInstr::FrameDestroy);
        Offset -= ThisVal;
        continue;
      }
    }

    MachineInstrBuilder MI = BuildStackAdjustment(
        MBB, MBBI, DL, isSub ? -ThisVal : ThisVal, InEpilogue);
    if (isSub)
      MI.setMIFlag(MachineInstr::FrameSetup);
    else
      MI.setMIFlag(MachineInstr::FrameDestroy);

    Offset -= ThisVal;
  }
}

MachineInstrBuilder X86FrameLowering::BuildStackAdjustment(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI, DebugLoc DL,
    int64_t Offset, bool InEpilogue) const {
  assert(Offset != 0 && "zero offset stack adjustment requested");

  // On Atom, using LEA to adjust SP is preferred, but using it in the epilogue
  // is tricky.
  bool UseLEA;
  if (!InEpilogue) {
    UseLEA = STI.useLeaForSP();
  } else {
    // If we can use LEA for SP but we shouldn't, check that none
    // of the terminators uses the eflags. Otherwise we will insert
    // a ADD that will redefine the eflags and break the condition.
    // Alternatively, we could move the ADD, but this may not be possible
    // and is an optimization anyway.
    UseLEA = canUseLEAForSPInEpilogue(*MBB.getParent());
    if (UseLEA && !STI.useLeaForSP())
      UseLEA = terminatorsNeedFlagsAsInput(MBB);
    // If that assert breaks, that means we do not do the right thing
    // in canUseAsEpilogue.
    assert((UseLEA || !terminatorsNeedFlagsAsInput(MBB)) &&
           "We shouldn't have allowed this insertion point");
  }

  MachineInstrBuilder MI;
  if (UseLEA) {
    MI = addRegOffset(BuildMI(MBB, MBBI, DL,
                              TII.get(getLEArOpcode(Uses64BitFramePtr)),
                              StackPtr),
                      StackPtr, false, Offset);
  } else {
    bool IsSub = Offset < 0;
    uint64_t AbsOffset = IsSub ? -Offset : Offset;
    unsigned Opc = IsSub ? getSUBriOpcode(Uses64BitFramePtr, AbsOffset)
                         : getADDriOpcode(Uses64BitFramePtr, AbsOffset);
    MI = BuildMI(MBB, MBBI, DL, TII.get(Opc), StackPtr)
             .addReg(StackPtr)
             .addImm(AbsOffset);
    MI->getOperand(3).setIsDead(); // The EFLAGS implicit def is dead.
  }
  return MI;
}

int X86FrameLowering::mergeSPUpdates(MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator &MBBI,
                                     bool doMergeWithPrevious) const {
  if ((doMergeWithPrevious && MBBI == MBB.begin()) ||
      (!doMergeWithPrevious && MBBI == MBB.end()))
    return 0;

  MachineBasicBlock::iterator PI = doMergeWithPrevious ? std::prev(MBBI) : MBBI;
  MachineBasicBlock::iterator NI = doMergeWithPrevious ? nullptr
                                                       : std::next(MBBI);
  unsigned Opc = PI->getOpcode();
  int Offset = 0;

  if ((Opc == X86::ADD64ri32 || Opc == X86::ADD64ri8 ||
       Opc == X86::ADD32ri || Opc == X86::ADD32ri8 ||
       Opc == X86::LEA32r || Opc == X86::LEA64_32r) &&
      PI->getOperand(0).getReg() == StackPtr){
    Offset += PI->getOperand(2).getImm();
    MBB.erase(PI);
    if (!doMergeWithPrevious) MBBI = NI;
  } else if ((Opc == X86::SUB64ri32 || Opc == X86::SUB64ri8 ||
              Opc == X86::SUB32ri || Opc == X86::SUB32ri8) &&
             PI->getOperand(0).getReg() == StackPtr) {
    Offset -= PI->getOperand(2).getImm();
    MBB.erase(PI);
    if (!doMergeWithPrevious) MBBI = NI;
  }

  return Offset;
}

void X86FrameLowering::BuildCFI(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MBBI, DebugLoc DL,
                                MCCFIInstruction CFIInst) const {
  MachineFunction &MF = *MBB.getParent();
  unsigned CFIIndex = MF.getMMI().addFrameInst(CFIInst);
  BuildMI(MBB, MBBI, DL, TII.get(TargetOpcode::CFI_INSTRUCTION))
      .addCFIIndex(CFIIndex);
}

void
X86FrameLowering::emitCalleeSavedFrameMoves(MachineBasicBlock &MBB,
                                            MachineBasicBlock::iterator MBBI,
                                            DebugLoc DL) const {
  MachineFunction &MF = *MBB.getParent();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  MachineModuleInfo &MMI = MF.getMMI();
  const MCRegisterInfo *MRI = MMI.getContext().getRegisterInfo();

  // Add callee saved registers to move list.
  const std::vector<CalleeSavedInfo> &CSI = MFI->getCalleeSavedInfo();
  if (CSI.empty()) return;

  // Calculate offsets.
  for (std::vector<CalleeSavedInfo>::const_iterator
         I = CSI.begin(), E = CSI.end(); I != E; ++I) {
    int64_t Offset = MFI->getObjectOffset(I->getFrameIdx());
    unsigned Reg = I->getReg();

    unsigned DwarfReg = MRI->getDwarfRegNum(Reg, true);
    BuildCFI(MBB, MBBI, DL,
             MCCFIInstruction::createOffset(nullptr, DwarfReg, Offset));
  }
}

/// usesTheStack - This function checks if any of the users of EFLAGS
/// copies the EFLAGS. We know that the code that lowers COPY of EFLAGS has
/// to use the stack, and if we don't adjust the stack we clobber the first
/// frame index.
/// See X86InstrInfo::copyPhysReg.
static bool usesTheStack(const MachineFunction &MF) {
  const MachineRegisterInfo &MRI = MF.getRegInfo();

  for (MachineRegisterInfo::reg_instr_iterator
       ri = MRI.reg_instr_begin(X86::EFLAGS), re = MRI.reg_instr_end();
       ri != re; ++ri)
    if (ri->isCopy())
      return true;

  return false;
}

void X86FrameLowering::emitStackProbeCall(MachineFunction &MF,
                                          MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MBBI,
                                          DebugLoc DL) const {
  bool IsLargeCodeModel = MF.getTarget().getCodeModel() == CodeModel::Large;

  unsigned CallOp;
  if (Is64Bit)
    CallOp = IsLargeCodeModel ? X86::CALL64r : X86::CALL64pcrel32;
  else
    CallOp = X86::CALLpcrel32;

  const char *Symbol;
  if (Is64Bit) {
    if (STI.isTargetCygMing()) {
      Symbol = "___chkstk_ms";
    } else {
      Symbol = "__chkstk";
    }
  } else if (STI.isTargetCygMing())
    Symbol = "_alloca";
  else
    Symbol = "_chkstk";

  MachineInstrBuilder CI;

  // All current stack probes take AX and SP as input, clobber flags, and
  // preserve all registers. x86_64 probes leave RSP unmodified.
  if (Is64Bit && MF.getTarget().getCodeModel() == CodeModel::Large) {
    // For the large code model, we have to call through a register. Use R11,
    // as it is scratch in all supported calling conventions.
    BuildMI(MBB, MBBI, DL, TII.get(X86::MOV64ri), X86::R11)
        .addExternalSymbol(Symbol);
    CI = BuildMI(MBB, MBBI, DL, TII.get(CallOp)).addReg(X86::R11);
  } else {
    CI = BuildMI(MBB, MBBI, DL, TII.get(CallOp)).addExternalSymbol(Symbol);
  }

  unsigned AX = Is64Bit ? X86::RAX : X86::EAX;
  unsigned SP = Is64Bit ? X86::RSP : X86::ESP;
  CI.addReg(AX, RegState::Implicit)
      .addReg(SP, RegState::Implicit)
      .addReg(AX, RegState::Define | RegState::Implicit)
      .addReg(SP, RegState::Define | RegState::Implicit)
      .addReg(X86::EFLAGS, RegState::Define | RegState::Implicit);

  if (Is64Bit) {
    // MSVC x64's __chkstk and cygwin/mingw's ___chkstk_ms do not adjust %rsp
    // themselves. It also does not clobber %rax so we can reuse it when
    // adjusting %rsp.
    BuildMI(MBB, MBBI, DL, TII.get(X86::SUB64rr), X86::RSP)
        .addReg(X86::RSP)
        .addReg(X86::RAX);
  }
}

static unsigned calculateSetFPREG(uint64_t SPAdjust) {
  // Win64 ABI has a less restrictive limitation of 240; 128 works equally well
  // and might require smaller successive adjustments.
  const uint64_t Win64MaxSEHOffset = 128;
  uint64_t SEHFrameOffset = std::min(SPAdjust, Win64MaxSEHOffset);
  // Win64 ABI requires 16-byte alignment for the UWOP_SET_FPREG opcode.
  return SEHFrameOffset & -16;
}

// If we're forcing a stack realignment we can't rely on just the frame
// info, we need to know the ABI stack alignment as well in case we
// have a call out.  Otherwise just make sure we have some alignment - we'll
// go with the minimum SlotSize.
uint64_t X86FrameLowering::calculateMaxStackAlign(const MachineFunction &MF) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  uint64_t MaxAlign = MFI->getMaxAlignment(); // Desired stack alignment.
  unsigned StackAlign = getStackAlignment();
  if (MF.getFunction()->hasFnAttribute("stackrealign")) {
    if (MFI->hasCalls())
      MaxAlign = (StackAlign > MaxAlign) ? StackAlign : MaxAlign;
    else if (MaxAlign < SlotSize)
      MaxAlign = SlotSize;
  }
  return MaxAlign;
}

void X86FrameLowering::BuildStackAlignAND(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MBBI,
                                          DebugLoc DL,
                                          uint64_t MaxAlign) const {
  uint64_t Val = -MaxAlign;
  MachineInstr *MI =
      BuildMI(MBB, MBBI, DL, TII.get(getANDriOpcode(Uses64BitFramePtr, Val)),
              StackPtr)
          .addReg(StackPtr)
          .addImm(Val)
          .setMIFlag(MachineInstr::FrameSetup);

  // The EFLAGS implicit def is dead.
  MI->getOperand(3).setIsDead();
}

/// emitPrologue - Push callee-saved registers onto the stack, which
/// automatically adjust the stack pointer. Adjust the stack pointer to allocate
/// space for local variables. Also emit labels used by the exception handler to
/// generate the exception handling frames.

/*
  Here's a gist of what gets emitted:

  ; Establish frame pointer, if needed
  [if needs FP]
      push  %rbp
      .cfi_def_cfa_offset 16
      .cfi_offset %rbp, -16
      .seh_pushreg %rpb
      mov  %rsp, %rbp
      .cfi_def_cfa_register %rbp

  ; Spill general-purpose registers
  [for all callee-saved GPRs]
      pushq %<reg>
      [if not needs FP]
         .cfi_def_cfa_offset (offset from RETADDR)
      .seh_pushreg %<reg>

  ; If the required stack alignment > default stack alignment
  ; rsp needs to be re-aligned.  This creates a "re-alignment gap"
  ; of unknown size in the stack frame.
  [if stack needs re-alignment]
      and  $MASK, %rsp

  ; Allocate space for locals
  [if target is Windows and allocated space > 4096 bytes]
      ; Windows needs special care for allocations larger
      ; than one page.
      mov $NNN, %rax
      call ___chkstk_ms/___chkstk
      sub  %rax, %rsp
  [else]
      sub  $NNN, %rsp

  [if needs FP]
      .seh_stackalloc (size of XMM spill slots)
      .seh_setframe %rbp, SEHFrameOffset ; = size of all spill slots
  [else]
      .seh_stackalloc NNN

  ; Spill XMMs
  ; Note, that while only Windows 64 ABI specifies XMMs as callee-preserved,
  ; they may get spilled on any platform, if the current function
  ; calls @llvm.eh.unwind.init
  [if needs FP]
      [for all callee-saved XMM registers]
          movaps  %<xmm reg>, -MMM(%rbp)
      [for all callee-saved XMM registers]
          .seh_savexmm %<xmm reg>, (-MMM + SEHFrameOffset)
              ; i.e. the offset relative to (%rbp - SEHFrameOffset)
  [else]
      [for all callee-saved XMM registers]
          movaps  %<xmm reg>, KKK(%rsp)
      [for all callee-saved XMM registers]
          .seh_savexmm %<xmm reg>, KKK

  .seh_endprologue

  [if needs base pointer]
      mov  %rsp, %rbx
      [if needs to restore base pointer]
          mov %rsp, -MMM(%rbp)

  ; Emit CFI info
  [if needs FP]
      [for all callee-saved registers]
          .cfi_offset %<reg>, (offset from %rbp)
  [else]
       .cfi_def_cfa_offset (offset from RETADDR)
      [for all callee-saved registers]
          .cfi_offset %<reg>, (offset from %rsp)

  Notes:
  - .seh directives are emitted only for Windows 64 ABI
  - .cfi directives are emitted for all other ABIs
  - for 32-bit code, substitute %e?? registers for %r??
*/

void X86FrameLowering::emitPrologue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const {
  assert(&STI == &MF.getSubtarget<X86Subtarget>() &&
         "MF used frame lowering for wrong subtarget");
  MachineBasicBlock::iterator MBBI = MBB.begin();
  MachineFrameInfo *MFI = MF.getFrameInfo();
  const Function *Fn = MF.getFunction();
  MachineModuleInfo &MMI = MF.getMMI();
  X86MachineFunctionInfo *X86FI = MF.getInfo<X86MachineFunctionInfo>();
  uint64_t MaxAlign = calculateMaxStackAlign(MF); // Desired stack alignment.
  uint64_t StackSize = MFI->getStackSize();    // Number of bytes to allocate.
  bool IsFunclet = MBB.isEHFuncletEntry();
  bool HasFP = hasFP(MF);
  bool IsWin64CC = STI.isCallingConvWin64(Fn->getCallingConv());
  bool IsWin64Prologue = MF.getTarget().getMCAsmInfo()->usesWindowsCFI();
  bool NeedsWinCFI = IsWin64Prologue && Fn->needsUnwindTableEntry();
  bool NeedsDwarfCFI =
      !IsWin64Prologue && (MMI.hasDebugInfo() || Fn->needsUnwindTableEntry());
  unsigned FramePtr = TRI->getFrameRegister(MF);
  const unsigned MachineFramePtr =
      STI.isTarget64BitILP32()
          ? getX86SubSuperRegister(FramePtr, MVT::i64, false)
          : FramePtr;
  unsigned BasePtr = TRI->getBaseRegister();
  DebugLoc DL;

  // Add RETADDR move area to callee saved frame size.
  int TailCallReturnAddrDelta = X86FI->getTCReturnAddrDelta();
  if (TailCallReturnAddrDelta && IsWin64Prologue)
    report_fatal_error("Can't handle guaranteed tail call under win64 yet");

  if (TailCallReturnAddrDelta < 0)
    X86FI->setCalleeSavedFrameSize(
      X86FI->getCalleeSavedFrameSize() - TailCallReturnAddrDelta);

  bool UseStackProbe = (STI.isOSWindows() && !STI.isTargetMachO());

  // The default stack probe size is 4096 if the function has no stackprobesize
  // attribute.
  unsigned StackProbeSize = 4096;
  if (Fn->hasFnAttribute("stack-probe-size"))
    Fn->getFnAttribute("stack-probe-size")
        .getValueAsString()
        .getAsInteger(0, StackProbeSize);

  // If this is x86-64 and the Red Zone is not disabled, if we are a leaf
  // function, and use up to 128 bytes of stack space, don't have a frame
  // pointer, calls, or dynamic alloca then we do not need to adjust the
  // stack pointer (we fit in the Red Zone). We also check that we don't
  // push and pop from the stack.
  if (Is64Bit && !Fn->hasFnAttribute(Attribute::NoRedZone) &&
      !TRI->needsStackRealignment(MF) &&
      !MFI->hasVarSizedObjects() && // No dynamic alloca.
      !MFI->adjustsStack() &&       // No calls.
      !IsWin64CC &&                 // Win64 has no Red Zone
      !usesTheStack(MF) &&          // Don't push and pop.
      !MF.shouldSplitStack()) {     // Regular stack
    uint64_t MinSize = X86FI->getCalleeSavedFrameSize();
    if (HasFP) MinSize += SlotSize;
    StackSize = std::max(MinSize, StackSize > 128 ? StackSize - 128 : 0);
    MFI->setStackSize(StackSize);
  }

  // Insert stack pointer adjustment for later moving of return addr.  Only
  // applies to tail call optimized functions where the callee argument stack
  // size is bigger than the callers.
  if (TailCallReturnAddrDelta < 0) {
    BuildStackAdjustment(MBB, MBBI, DL, TailCallReturnAddrDelta,
                         /*InEpilogue=*/false)
        .setMIFlag(MachineInstr::FrameSetup);
  }

  // Mapping for machine moves:
  //
  //   DST: VirtualFP AND
  //        SRC: VirtualFP              => DW_CFA_def_cfa_offset
  //        ELSE                        => DW_CFA_def_cfa
  //
  //   SRC: VirtualFP AND
  //        DST: Register               => DW_CFA_def_cfa_register
  //
  //   ELSE
  //        OFFSET < 0                  => DW_CFA_offset_extended_sf
  //        REG < 64                    => DW_CFA_offset + Reg
  //        ELSE                        => DW_CFA_offset_extended

  uint64_t NumBytes = 0;
  int stackGrowth = -SlotSize;

  unsigned RDX = Uses64BitFramePtr ? X86::RDX : X86::EDX;
  if (IsWin64Prologue && IsFunclet) {
    // Immediately spill RDX into the home slot. The runtime cares about this.
    // MOV64mr %rdx, 16(%rsp)
    unsigned MOVmr = Uses64BitFramePtr ? X86::MOV64mr : X86::MOV32mr;
    addRegOffset(BuildMI(MBB, MBBI, DL, TII.get(MOVmr)), StackPtr, true, 16)
        .addReg(RDX)
        .setMIFlag(MachineInstr::FrameSetup);
  }

  if (HasFP) {
    // Calculate required stack adjustment.
    uint64_t FrameSize = StackSize - SlotSize;
    // If required, include space for extra hidden slot for stashing base pointer.
    if (X86FI->getRestoreBasePointer())
      FrameSize += SlotSize;

    NumBytes = FrameSize - X86FI->getCalleeSavedFrameSize();

    // Callee-saved registers are pushed on stack before the stack is realigned.
    if (TRI->needsStackRealignment(MF) && !IsWin64Prologue)
      NumBytes = RoundUpToAlignment(NumBytes, MaxAlign);

    // Get the offset of the stack slot for the EBP register, which is
    // guaranteed to be the last slot by processFunctionBeforeFrameFinalized.
    // Update the frame offset adjustment.
    if (!IsFunclet)
      MFI->setOffsetAdjustment(-NumBytes);
    else
      assert(MFI->getOffsetAdjustment() == -(int)NumBytes &&
             "should calculate same local variable offset for funclets");

    // Save EBP/RBP into the appropriate stack slot.
    BuildMI(MBB, MBBI, DL, TII.get(Is64Bit ? X86::PUSH64r : X86::PUSH32r))
      .addReg(MachineFramePtr, RegState::Kill)
      .setMIFlag(MachineInstr::FrameSetup);

    if (NeedsDwarfCFI) {
      // Mark the place where EBP/RBP was saved.
      // Define the current CFA rule to use the provided offset.
      assert(StackSize);
      BuildCFI(MBB, MBBI, DL,
               MCCFIInstruction::createDefCfaOffset(nullptr, 2 * stackGrowth));

      // Change the rule for the FramePtr to be an "offset" rule.
      unsigned DwarfFramePtr = TRI->getDwarfRegNum(MachineFramePtr, true);
      BuildCFI(MBB, MBBI, DL, MCCFIInstruction::createOffset(
                                  nullptr, DwarfFramePtr, 2 * stackGrowth));
    }

    if (NeedsWinCFI) {
      BuildMI(MBB, MBBI, DL, TII.get(X86::SEH_PushReg))
          .addImm(FramePtr)
          .setMIFlag(MachineInstr::FrameSetup);
    }

    if (!IsWin64Prologue && !IsFunclet) {
      // Update EBP with the new base value.
      BuildMI(MBB, MBBI, DL,
              TII.get(Uses64BitFramePtr ? X86::MOV64rr : X86::MOV32rr),
              FramePtr)
          .addReg(StackPtr)
          .setMIFlag(MachineInstr::FrameSetup);

      if (NeedsDwarfCFI) {
        // Mark effective beginning of when frame pointer becomes valid.
        // Define the current CFA to use the EBP/RBP register.
        unsigned DwarfFramePtr = TRI->getDwarfRegNum(MachineFramePtr, true);
        BuildCFI(MBB, MBBI, DL, MCCFIInstruction::createDefCfaRegister(
                                    nullptr, DwarfFramePtr));
      }
    }

    // Mark the FramePtr as live-in in every block.
    for (MachineFunction::iterator I = MF.begin(), E = MF.end(); I != E; ++I)
      I->addLiveIn(MachineFramePtr);
  } else {
    assert(!IsFunclet && "funclets without FPs not yet implemented");
    NumBytes = StackSize - X86FI->getCalleeSavedFrameSize();
  }

  // For EH funclets, only allocate enough space for outgoing calls. Save the
  // NumBytes value that we would've used for the parent frame.
  unsigned ParentFrameNumBytes = NumBytes;
  if (IsFunclet)
    NumBytes = MFI->getMaxCallFrameSize();

  // Skip the callee-saved push instructions.
  bool PushedRegs = false;
  int StackOffset = 2 * stackGrowth;

  while (MBBI != MBB.end() &&
         MBBI->getFlag(MachineInstr::FrameSetup) &&
         (MBBI->getOpcode() == X86::PUSH32r ||
          MBBI->getOpcode() == X86::PUSH64r)) {
    PushedRegs = true;
    unsigned Reg = MBBI->getOperand(0).getReg();
    ++MBBI;

    if (!HasFP && NeedsDwarfCFI) {
      // Mark callee-saved push instruction.
      // Define the current CFA rule to use the provided offset.
      assert(StackSize);
      BuildCFI(MBB, MBBI, DL,
               MCCFIInstruction::createDefCfaOffset(nullptr, StackOffset));
      StackOffset += stackGrowth;
    }

    if (NeedsWinCFI) {
      BuildMI(MBB, MBBI, DL, TII.get(X86::SEH_PushReg)).addImm(Reg).setMIFlag(
          MachineInstr::FrameSetup);
    }
  }

  // Realign stack after we pushed callee-saved registers (so that we'll be
  // able to calculate their offsets from the frame pointer).
  // Don't do this for Win64, it needs to realign the stack after the prologue.
  if (!IsWin64Prologue && !IsFunclet && TRI->needsStackRealignment(MF)) {
    assert(HasFP && "There should be a frame pointer if stack is realigned.");
    BuildStackAlignAND(MBB, MBBI, DL, MaxAlign);
  }

  // If there is an SUB32ri of ESP immediately before this instruction, merge
  // the two. This can be the case when tail call elimination is enabled and
  // the callee has more arguments then the caller.
  NumBytes -= mergeSPUpdates(MBB, MBBI, true);

  // Adjust stack pointer: ESP -= numbytes.

  // Windows and cygwin/mingw require a prologue helper routine when allocating
  // more than 4K bytes on the stack.  Windows uses __chkstk and cygwin/mingw
  // uses __alloca.  __alloca and the 32-bit version of __chkstk will probe the
  // stack and adjust the stack pointer in one go.  The 64-bit version of
  // __chkstk is only responsible for probing the stack.  The 64-bit prologue is
  // responsible for adjusting the stack pointer.  Touching the stack at 4K
  // increments is necessary to ensure that the guard pages used by the OS
  // virtual memory manager are allocated in correct sequence.
  uint64_t AlignedNumBytes = NumBytes;
  if (IsWin64Prologue && !IsFunclet && TRI->needsStackRealignment(MF))
    AlignedNumBytes = RoundUpToAlignment(AlignedNumBytes, MaxAlign);
  if (AlignedNumBytes >= StackProbeSize && UseStackProbe) {
    // Check whether EAX is livein for this function.
    bool isEAXAlive = isEAXLiveIn(MF);

    if (isEAXAlive) {
      // Sanity check that EAX is not livein for this function.
      // It should not be, so throw an assert.
      assert(!Is64Bit && "EAX is livein in x64 case!");

      // Save EAX
      BuildMI(MBB, MBBI, DL, TII.get(X86::PUSH32r))
        .addReg(X86::EAX, RegState::Kill)
        .setMIFlag(MachineInstr::FrameSetup);
    }

    if (Is64Bit) {
      // Handle the 64-bit Windows ABI case where we need to call __chkstk.
      // Function prologue is responsible for adjusting the stack pointer.
      if (isUInt<32>(NumBytes)) {
        BuildMI(MBB, MBBI, DL, TII.get(X86::MOV32ri), X86::EAX)
            .addImm(NumBytes)
            .setMIFlag(MachineInstr::FrameSetup);
      } else if (isInt<32>(NumBytes)) {
        BuildMI(MBB, MBBI, DL, TII.get(X86::MOV64ri32), X86::RAX)
            .addImm(NumBytes)
            .setMIFlag(MachineInstr::FrameSetup);
      } else {
        BuildMI(MBB, MBBI, DL, TII.get(X86::MOV64ri), X86::RAX)
            .addImm(NumBytes)
            .setMIFlag(MachineInstr::FrameSetup);
      }
    } else {
      // Allocate NumBytes-4 bytes on stack in case of isEAXAlive.
      // We'll also use 4 already allocated bytes for EAX.
      BuildMI(MBB, MBBI, DL, TII.get(X86::MOV32ri), X86::EAX)
        .addImm(isEAXAlive ? NumBytes - 4 : NumBytes)
        .setMIFlag(MachineInstr::FrameSetup);
    }

    // Save a pointer to the MI where we set AX.
    MachineBasicBlock::iterator SetRAX = MBBI;
    --SetRAX;

    // Call __chkstk, __chkstk_ms, or __alloca.
    emitStackProbeCall(MF, MBB, MBBI, DL);

    // Apply the frame setup flag to all inserted instrs.
    for (; SetRAX != MBBI; ++SetRAX)
      SetRAX->setFlag(MachineInstr::FrameSetup);

    if (isEAXAlive) {
      // Restore EAX
      MachineInstr *MI = addRegOffset(BuildMI(MF, DL, TII.get(X86::MOV32rm),
                                              X86::EAX),
                                      StackPtr, false, NumBytes - 4);
      MI->setFlag(MachineInstr::FrameSetup);
      MBB.insert(MBBI, MI);
    }
  } else if (NumBytes) {
    emitSPUpdate(MBB, MBBI, -(int64_t)NumBytes, /*InEpilogue=*/false);
  }

  if (NeedsWinCFI && NumBytes)
    BuildMI(MBB, MBBI, DL, TII.get(X86::SEH_StackAlloc))
        .addImm(NumBytes)
        .setMIFlag(MachineInstr::FrameSetup);

  int SEHFrameOffset = 0;
  if (IsWin64Prologue && HasFP) {
    // Set RBP to a small fixed offset from RSP. In the funclet case, we base
    // this calculation on the incoming RDX, which holds the value of RSP from
    // the parent frame at the end of the prologue.
    unsigned SPOrRDX = !IsFunclet ? StackPtr : RDX;
    SEHFrameOffset = calculateSetFPREG(ParentFrameNumBytes);
    if (SEHFrameOffset)
      addRegOffset(BuildMI(MBB, MBBI, DL, TII.get(X86::LEA64r), FramePtr),
                   SPOrRDX, false, SEHFrameOffset);
    else
      BuildMI(MBB, MBBI, DL, TII.get(X86::MOV64rr), FramePtr)
          .addReg(SPOrRDX);

    // If this is not a funclet, emit the CFI describing our frame pointer.
    if (NeedsWinCFI && !IsFunclet)
      BuildMI(MBB, MBBI, DL, TII.get(X86::SEH_SetFrame))
          .addImm(FramePtr)
          .addImm(SEHFrameOffset)
          .setMIFlag(MachineInstr::FrameSetup);
  } else if (IsFunclet && STI.is32Bit()) {
    // Reset EBP / ESI to something good for funclets.
    MBBI = restoreWin32EHStackPointers(MBB, MBBI, DL);
  }

  while (MBBI != MBB.end() && MBBI->getFlag(MachineInstr::FrameSetup)) {
    const MachineInstr *FrameInstr = &*MBBI;
    ++MBBI;

    if (NeedsWinCFI) {
      int FI;
      if (unsigned Reg = TII.isStoreToStackSlot(FrameInstr, FI)) {
        if (X86::FR64RegClass.contains(Reg)) {
          unsigned IgnoredFrameReg;
          int Offset = getFrameIndexReference(MF, FI, IgnoredFrameReg);
          Offset += SEHFrameOffset;

          BuildMI(MBB, MBBI, DL, TII.get(X86::SEH_SaveXMM))
              .addImm(Reg)
              .addImm(Offset)
              .setMIFlag(MachineInstr::FrameSetup);
        }
      }
    }
  }

  if (NeedsWinCFI)
    BuildMI(MBB, MBBI, DL, TII.get(X86::SEH_EndPrologue))
        .setMIFlag(MachineInstr::FrameSetup);

  // Realign stack after we spilled callee-saved registers (so that we'll be
  // able to calculate their offsets from the frame pointer).
  // Win64 requires aligning the stack after the prologue.
  if (IsWin64Prologue && TRI->needsStackRealignment(MF)) {
    assert(HasFP && "There should be a frame pointer if stack is realigned.");
    BuildStackAlignAND(MBB, MBBI, DL, MaxAlign);
  }

  // If we need a base pointer, set it up here. It's whatever the value
  // of the stack pointer is at this point. Any variable size objects
  // will be allocated after this, so we can still use the base pointer
  // to reference locals.
  if (TRI->hasBasePointer(MF)) {
    // Update the base pointer with the current stack pointer.
    unsigned Opc = Uses64BitFramePtr ? X86::MOV64rr : X86::MOV32rr;
    BuildMI(MBB, MBBI, DL, TII.get(Opc), BasePtr)
      .addReg(StackPtr)
      .setMIFlag(MachineInstr::FrameSetup);
    if (X86FI->getRestoreBasePointer()) {
      // Stash value of base pointer.  Saving RSP instead of EBP shortens
      // dependence chain. Used by SjLj EH.
      unsigned Opm = Uses64BitFramePtr ? X86::MOV64mr : X86::MOV32mr;
      addRegOffset(BuildMI(MBB, MBBI, DL, TII.get(Opm)),
                   FramePtr, true, X86FI->getRestoreBasePointerOffset())
        .addReg(StackPtr)
        .setMIFlag(MachineInstr::FrameSetup);
    }

    if (X86FI->getHasSEHFramePtrSave()) {
      // Stash the value of the frame pointer relative to the base pointer for
      // Win32 EH. This supports Win32 EH, which does the inverse of the above:
      // it recovers the frame pointer from the base pointer rather than the
      // other way around.
      unsigned Opm = Uses64BitFramePtr ? X86::MOV64mr : X86::MOV32mr;
      unsigned UsedReg;
      int Offset =
          getFrameIndexReference(MF, X86FI->getSEHFramePtrSaveIndex(), UsedReg);
      assert(UsedReg == BasePtr);
      addRegOffset(BuildMI(MBB, MBBI, DL, TII.get(Opm)), UsedReg, true, Offset)
          .addReg(FramePtr)
          .setMIFlag(MachineInstr::FrameSetup);
    }
  }

  if (((!HasFP && NumBytes) || PushedRegs) && NeedsDwarfCFI) {
    // Mark end of stack pointer adjustment.
    if (!HasFP && NumBytes) {
      // Define the current CFA rule to use the provided offset.
      assert(StackSize);
      BuildCFI(MBB, MBBI, DL, MCCFIInstruction::createDefCfaOffset(
                                  nullptr, -StackSize + stackGrowth));
    }

    // Emit DWARF info specifying the offsets of the callee-saved registers.
    if (PushedRegs)
      emitCalleeSavedFrameMoves(MBB, MBBI, DL);
  }
}

bool X86FrameLowering::canUseLEAForSPInEpilogue(
    const MachineFunction &MF) const {
  // We can't use LEA instructions for adjusting the stack pointer if this is a
  // leaf function in the Win64 ABI.  Only ADD instructions may be used to
  // deallocate the stack.
  // This means that we can use LEA for SP in two situations:
  // 1. We *aren't* using the Win64 ABI which means we are free to use LEA.
  // 2. We *have* a frame pointer which means we are permitted to use LEA.
  return !MF.getTarget().getMCAsmInfo()->usesWindowsCFI() || hasFP(MF);
}

static bool isFuncletReturnInstr(MachineInstr *MI) {
  switch (MI->getOpcode()) {
  case X86::CATCHRET:
  case X86::CLEANUPRET:
    return true;
  default:
    return false;
  }
  llvm_unreachable("impossible");
}

void X86FrameLowering::emitEpilogue(MachineFunction &MF,
                                    MachineBasicBlock &MBB) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  X86MachineFunctionInfo *X86FI = MF.getInfo<X86MachineFunctionInfo>();
  MachineBasicBlock::iterator MBBI = MBB.getFirstTerminator();
  DebugLoc DL;
  if (MBBI != MBB.end())
    DL = MBBI->getDebugLoc();
  // standard x86_64 and NaCl use 64-bit frame/stack pointers, x32 - 32-bit.
  const bool Is64BitILP32 = STI.isTarget64BitILP32();
  unsigned FramePtr = TRI->getFrameRegister(MF);
  unsigned MachineFramePtr =
      Is64BitILP32 ? getX86SubSuperRegister(FramePtr, MVT::i64, false)
                   : FramePtr;

  bool IsWin64Prologue = MF.getTarget().getMCAsmInfo()->usesWindowsCFI();
  bool NeedsWinCFI =
      IsWin64Prologue && MF.getFunction()->needsUnwindTableEntry();

  // Get the number of bytes to allocate from the FrameInfo.
  uint64_t StackSize = MFI->getStackSize();
  uint64_t MaxAlign = calculateMaxStackAlign(MF);
  unsigned CSSize = X86FI->getCalleeSavedFrameSize();
  uint64_t NumBytes = 0;

  if (MBBI->getOpcode() == X86::CATCHRET) {
    NumBytes = MFI->getMaxCallFrameSize();
    assert(hasFP(MF) && "EH funclets without FP not yet implemented");
    MachineBasicBlock *TargetMBB = MBBI->getOperand(0).getMBB();

    // If this is SEH, this isn't really a funclet return.
    bool IsSEH = isAsynchronousEHPersonality(
        classifyEHPersonality(MF.getFunction()->getPersonalityFn()));
    if (IsSEH) {
      if (STI.is32Bit())
        restoreWin32EHStackPointers(MBB, MBBI, DL, /*RestoreSP=*/true);
      BuildMI(MBB, MBBI, DL, TII.get(X86::JMP_4)).addMBB(TargetMBB);
      MBBI->eraseFromParent();
      return;
    }

    // For 32-bit, create a new block for the restore code.
    MachineBasicBlock *RestoreMBB = TargetMBB;
    if (STI.is32Bit()) {
      RestoreMBB = MF.CreateMachineBasicBlock(MBB.getBasicBlock());
      MF.insert(TargetMBB, RestoreMBB);
      MBB.removeSuccessor(TargetMBB);
      MBB.addSuccessor(RestoreMBB);
      RestoreMBB->addSuccessor(TargetMBB);
    }

    // Fill EAX/RAX with the address of the target block.
    unsigned ReturnReg = STI.is64Bit() ? X86::RAX : X86::EAX;
    if (STI.is64Bit()) {
      // LEA64r RestoreMBB(%rip), %rax
      BuildMI(MBB, MBBI, DL, TII.get(X86::LEA64r), ReturnReg)
          .addReg(X86::RIP)
          .addImm(0)
          .addReg(0)
          .addMBB(RestoreMBB)
          .addReg(0);
    } else {
      // MOV32ri $RestoreMBB, %eax
      BuildMI(MBB, MBBI, DL, TII.get(X86::MOV32ri))
          .addReg(ReturnReg)
          .addMBB(RestoreMBB);
    }

    // Pop EBP.
    BuildMI(MBB, MBBI, DL, TII.get(Is64Bit ? X86::POP64r : X86::POP32r),
            MachineFramePtr)
        .setMIFlag(MachineInstr::FrameDestroy);

    // Insert frame restoration code in a new block.
    if (STI.is32Bit()) {
      auto RestoreMBBI = RestoreMBB->begin();
      restoreWin32EHStackPointers(*RestoreMBB, RestoreMBBI, DL,
                                  /*RestoreSP=*/true);
      BuildMI(*RestoreMBB, RestoreMBBI, DL, TII.get(X86::JMP_4))
          .addMBB(TargetMBB);
    }
    // Replace CATCHRET with the appropriate RET.
    unsigned RetOp = STI.is64Bit() ? X86::RETQ : X86::RETL;
    MachineBasicBlock::iterator NewExit =
        BuildMI(MBB, MBBI, DL, TII.get(RetOp)).addReg(ReturnReg);
    MBBI->eraseFromParent();
    MBBI = NewExit;
  } else if (MBBI->getOpcode() == X86::CLEANUPRET) {
    NumBytes = MFI->getMaxCallFrameSize();
    assert(hasFP(MF) && "EH funclets without FP not yet implemented");
    BuildMI(MBB, MBBI, DL, TII.get(Is64Bit ? X86::POP64r : X86::POP32r),
            MachineFramePtr)
        .setMIFlag(MachineInstr::FrameDestroy);
    // Replace CLEANUPRET with the appropriate RET.
    unsigned RetOp = STI.is64Bit() ? X86::RETQ : X86::RETL;
    MachineBasicBlock::iterator NewExit =
        BuildMI(MBB, MBBI, DL, TII.get(RetOp));
    MBBI->eraseFromParent();
    MBBI = NewExit;
  } else if (hasFP(MF)) {
    // Calculate required stack adjustment.
    uint64_t FrameSize = StackSize - SlotSize;
    NumBytes = FrameSize - CSSize;

    // Callee-saved registers were pushed on stack before the stack was
    // realigned.
    if (TRI->needsStackRealignment(MF) && !IsWin64Prologue)
      NumBytes = RoundUpToAlignment(FrameSize, MaxAlign);

    // Pop EBP.
    BuildMI(MBB, MBBI, DL,
            TII.get(Is64Bit ? X86::POP64r : X86::POP32r), MachineFramePtr)
        .setMIFlag(MachineInstr::FrameDestroy);
  } else {
    NumBytes = StackSize - CSSize;
  }
  uint64_t SEHStackAllocAmt = NumBytes;

  // Skip the callee-saved pop instructions.
  while (MBBI != MBB.begin()) {
    MachineBasicBlock::iterator PI = std::prev(MBBI);
    unsigned Opc = PI->getOpcode();

    if ((Opc != X86::POP32r || !PI->getFlag(MachineInstr::FrameDestroy)) &&
        (Opc != X86::POP64r || !PI->getFlag(MachineInstr::FrameDestroy)) &&
        Opc != X86::DBG_VALUE && !PI->isTerminator())
      break;

    --MBBI;
  }
  MachineBasicBlock::iterator FirstCSPop = MBBI;

  if (MBBI != MBB.end())
    DL = MBBI->getDebugLoc();

  // If there is an ADD32ri or SUB32ri of ESP immediately before this
  // instruction, merge the two instructions.
  if (NumBytes || MFI->hasVarSizedObjects())
    NumBytes += mergeSPUpdates(MBB, MBBI, true);

  // If dynamic alloca is used, then reset esp to point to the last callee-saved
  // slot before popping them off! Same applies for the case, when stack was
  // realigned.
  if (TRI->needsStackRealignment(MF) || MFI->hasVarSizedObjects()) {
    if (TRI->needsStackRealignment(MF))
      MBBI = FirstCSPop;
    unsigned SEHFrameOffset = calculateSetFPREG(SEHStackAllocAmt);
    uint64_t LEAAmount =
        IsWin64Prologue ? SEHStackAllocAmt - SEHFrameOffset : -CSSize;

    // There are only two legal forms of epilogue:
    // - add SEHAllocationSize, %rsp
    // - lea SEHAllocationSize(%FramePtr), %rsp
    //
    // 'mov %FramePtr, %rsp' will not be recognized as an epilogue sequence.
    // However, we may use this sequence if we have a frame pointer because the
    // effects of the prologue can safely be undone.
    if (LEAAmount != 0) {
      unsigned Opc = getLEArOpcode(Uses64BitFramePtr);
      addRegOffset(BuildMI(MBB, MBBI, DL, TII.get(Opc), StackPtr),
                   FramePtr, false, LEAAmount);
      --MBBI;
    } else {
      unsigned Opc = (Uses64BitFramePtr ? X86::MOV64rr : X86::MOV32rr);
      BuildMI(MBB, MBBI, DL, TII.get(Opc), StackPtr)
        .addReg(FramePtr);
      --MBBI;
    }
  } else if (NumBytes) {
    // Adjust stack pointer back: ESP += numbytes.
    emitSPUpdate(MBB, MBBI, NumBytes, /*InEpilogue=*/true);
    --MBBI;
  }

  // Windows unwinder will not invoke function's exception handler if IP is
  // either in prologue or in epilogue.  This behavior causes a problem when a
  // call immediately precedes an epilogue, because the return address points
  // into the epilogue.  To cope with that, we insert an epilogue marker here,
  // then replace it with a 'nop' if it ends up immediately after a CALL in the
  // final emitted code.
  if (NeedsWinCFI)
    BuildMI(MBB, MBBI, DL, TII.get(X86::SEH_Epilogue));

  // Add the return addr area delta back since we are not tail calling.
  int Offset = -1 * X86FI->getTCReturnAddrDelta();
  assert(Offset >= 0 && "TCDelta should never be positive");
  if (Offset) {
    MBBI = MBB.getFirstTerminator();

    // Check for possible merge with preceding ADD instruction.
    Offset += mergeSPUpdates(MBB, MBBI, true);
    emitSPUpdate(MBB, MBBI, Offset, /*InEpilogue=*/true);
  }
}

// NOTE: this only has a subset of the full frame index logic. In
// particular, the FI < 0 and AfterFPPop logic is handled in
// X86RegisterInfo::eliminateFrameIndex, but not here. Possibly
// (probably?) it should be moved into here.
int X86FrameLowering::getFrameIndexReference(const MachineFunction &MF, int FI,
                                             unsigned &FrameReg) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();

  // We can't calculate offset from frame pointer if the stack is realigned,
  // so enforce usage of stack/base pointer.  The base pointer is used when we
  // have dynamic allocas in addition to dynamic realignment.
  if (TRI->hasBasePointer(MF))
    FrameReg = TRI->getBaseRegister();
  else if (TRI->needsStackRealignment(MF))
    FrameReg = TRI->getStackRegister();
  else
    FrameReg = TRI->getFrameRegister(MF);

  // Offset will hold the offset from the stack pointer at function entry to the
  // object.
  // We need to factor in additional offsets applied during the prologue to the
  // frame, base, and stack pointer depending on which is used.
  int Offset = MFI->getObjectOffset(FI) - getOffsetOfLocalArea();
  const X86MachineFunctionInfo *X86FI = MF.getInfo<X86MachineFunctionInfo>();
  unsigned CSSize = X86FI->getCalleeSavedFrameSize();
  uint64_t StackSize = MFI->getStackSize();
  bool HasFP = hasFP(MF);
  bool IsWin64Prologue = MF.getTarget().getMCAsmInfo()->usesWindowsCFI();
  int64_t FPDelta = 0;

  if (IsWin64Prologue) {
    assert(!MFI->hasCalls() || (StackSize % 16) == 8);

    // Calculate required stack adjustment.
    uint64_t FrameSize = StackSize - SlotSize;
    // If required, include space for extra hidden slot for stashing base pointer.
    if (X86FI->getRestoreBasePointer())
      FrameSize += SlotSize;
    uint64_t NumBytes = FrameSize - CSSize;

    uint64_t SEHFrameOffset = calculateSetFPREG(NumBytes);
    if (FI && FI == X86FI->getFAIndex())
      return -SEHFrameOffset;

    // FPDelta is the offset from the "traditional" FP location of the old base
    // pointer followed by return address and the location required by the
    // restricted Win64 prologue.
    // Add FPDelta to all offsets below that go through the frame pointer.
    FPDelta = FrameSize - SEHFrameOffset;
    assert((!MFI->hasCalls() || (FPDelta % 16) == 0) &&
           "FPDelta isn't aligned per the Win64 ABI!");
  }


  if (TRI->hasBasePointer(MF)) {
    assert(HasFP && "VLAs and dynamic stack realign, but no FP?!");
    if (FI < 0) {
      // Skip the saved EBP.
      return Offset + SlotSize + FPDelta;
    } else {
      assert((-(Offset + StackSize)) % MFI->getObjectAlignment(FI) == 0);
      return Offset + StackSize;
    }
  } else if (TRI->needsStackRealignment(MF)) {
    if (FI < 0) {
      // Skip the saved EBP.
      return Offset + SlotSize + FPDelta;
    } else {
      assert((-(Offset + StackSize)) % MFI->getObjectAlignment(FI) == 0);
      return Offset + StackSize;
    }
    // FIXME: Support tail calls
  } else {
    if (!HasFP)
      return Offset + StackSize;

    // Skip the saved EBP.
    Offset += SlotSize;

    // Skip the RETADDR move area
    int TailCallReturnAddrDelta = X86FI->getTCReturnAddrDelta();
    if (TailCallReturnAddrDelta < 0)
      Offset -= TailCallReturnAddrDelta;
  }

  return Offset + FPDelta;
}

// Simplified from getFrameIndexReference keeping only StackPointer cases
int X86FrameLowering::getFrameIndexReferenceFromSP(const MachineFunction &MF,
                                                   int FI,
                                                   unsigned &FrameReg) const {
  const MachineFrameInfo *MFI = MF.getFrameInfo();
  // Does not include any dynamic realign.
  const uint64_t StackSize = MFI->getStackSize();
  {
#ifndef NDEBUG
    // Note: LLVM arranges the stack as:
    // Args > Saved RetPC (<--FP) > CSRs > dynamic alignment (<--BP)
    //      > "Stack Slots" (<--SP)
    // We can always address StackSlots from RSP.  We can usually (unless
    // needsStackRealignment) address CSRs from RSP, but sometimes need to
    // address them from RBP.  FixedObjects can be placed anywhere in the stack
    // frame depending on their specific requirements (i.e. we can actually
    // refer to arguments to the function which are stored in the *callers*
    // frame).  As a result, THE RESULT OF THIS CALL IS MEANINGLESS FOR CSRs
    // AND FixedObjects IFF needsStackRealignment or hasVarSizedObject.

    assert(!TRI->hasBasePointer(MF) && "we don't handle this case");

    // We don't handle tail calls, and shouldn't be seeing them
    // either.
    int TailCallReturnAddrDelta =
        MF.getInfo<X86MachineFunctionInfo>()->getTCReturnAddrDelta();
    assert(!(TailCallReturnAddrDelta < 0) && "we don't handle this case!");
#endif
  }

  // Fill in FrameReg output argument.
  FrameReg = TRI->getStackRegister();

  // This is how the math works out:
  //
  //  %rsp grows (i.e. gets lower) left to right. Each box below is
  //  one word (eight bytes).  Obj0 is the stack slot we're trying to
  //  get to.
  //
  //    ----------------------------------
  //    | BP | Obj0 | Obj1 | ... | ObjN |
  //    ----------------------------------
  //    ^    ^      ^                   ^
  //    A    B      C                   E
  //
  // A is the incoming stack pointer.
  // (B - A) is the local area offset (-8 for x86-64) [1]
  // (C - A) is the Offset returned by MFI->getObjectOffset for Obj0 [2]
  //
  // |(E - B)| is the StackSize (absolute value, positive).  For a
  // stack that grown down, this works out to be (B - E). [3]
  //
  // E is also the value of %rsp after stack has been set up, and we
  // want (C - E) -- the value we can add to %rsp to get to Obj0.  Now
  // (C - E) == (C - A) - (B - A) + (B - E)
  //            { Using [1], [2] and [3] above }
  //         == getObjectOffset - LocalAreaOffset + StackSize
  //

  // Get the Offset from the StackPointer
  int Offset = MFI->getObjectOffset(FI) - getOffsetOfLocalArea();

  return Offset + StackSize;
}

bool X86FrameLowering::assignCalleeSavedSpillSlots(
    MachineFunction &MF, const TargetRegisterInfo *TRI,
    std::vector<CalleeSavedInfo> &CSI) const {
  MachineFrameInfo *MFI = MF.getFrameInfo();
  X86MachineFunctionInfo *X86FI = MF.getInfo<X86MachineFunctionInfo>();

  unsigned CalleeSavedFrameSize = 0;
  int SpillSlotOffset = getOffsetOfLocalArea() + X86FI->getTCReturnAddrDelta();

  if (hasFP(MF)) {
    // emitPrologue always spills frame register the first thing.
    SpillSlotOffset -= SlotSize;
    MFI->CreateFixedSpillStackObject(SlotSize, SpillSlotOffset);

    // Since emitPrologue and emitEpilogue will handle spilling and restoring of
    // the frame register, we can delete it from CSI list and not have to worry
    // about avoiding it later.
    unsigned FPReg = TRI->getFrameRegister(MF);
    for (unsigned i = 0; i < CSI.size(); ++i) {
      if (TRI->regsOverlap(CSI[i].getReg(),FPReg)) {
        CSI.erase(CSI.begin() + i);
        break;
      }
    }
  }

  // Assign slots for GPRs. It increases frame size.
  for (unsigned i = CSI.size(); i != 0; --i) {
    unsigned Reg = CSI[i - 1].getReg();

    if (!X86::GR64RegClass.contains(Reg) && !X86::GR32RegClass.contains(Reg))
      continue;

    SpillSlotOffset -= SlotSize;
    CalleeSavedFrameSize += SlotSize;

    int SlotIndex = MFI->CreateFixedSpillStackObject(SlotSize, SpillSlotOffset);
    CSI[i - 1].setFrameIdx(SlotIndex);
  }

  X86FI->setCalleeSavedFrameSize(CalleeSavedFrameSize);

  // Assign slots for XMMs.
  for (unsigned i = CSI.size(); i != 0; --i) {
    unsigned Reg = CSI[i - 1].getReg();
    if (X86::GR64RegClass.contains(Reg) || X86::GR32RegClass.contains(Reg))
      continue;

    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
    // ensure alignment
    SpillSlotOffset -= std::abs(SpillSlotOffset) % RC->getAlignment();
    // spill into slot
    SpillSlotOffset -= RC->getSize();
    int SlotIndex =
        MFI->CreateFixedSpillStackObject(RC->getSize(), SpillSlotOffset);
    CSI[i - 1].setFrameIdx(SlotIndex);
    MFI->ensureMaxAlignment(RC->getAlignment());
  }

  return true;
}

bool X86FrameLowering::spillCalleeSavedRegisters(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
    const std::vector<CalleeSavedInfo> &CSI,
    const TargetRegisterInfo *TRI) const {
  DebugLoc DL = MBB.findDebugLoc(MI);

  // Don't save CSRs in 32-bit EH funclets. The caller saves EBX, EBP, ESI, EDI
  // for us, and there are no XMM CSRs on Win32.
  if (MBB.isEHFuncletEntry() && STI.is32Bit() && STI.isOSWindows())
    return true;

  // Push GPRs. It increases frame size.
  unsigned Opc = STI.is64Bit() ? X86::PUSH64r : X86::PUSH32r;
  for (unsigned i = CSI.size(); i != 0; --i) {
    unsigned Reg = CSI[i - 1].getReg();

    if (!X86::GR64RegClass.contains(Reg) && !X86::GR32RegClass.contains(Reg))
      continue;
    // Add the callee-saved register as live-in. It's killed at the spill.
    MBB.addLiveIn(Reg);

    BuildMI(MBB, MI, DL, TII.get(Opc)).addReg(Reg, RegState::Kill)
      .setMIFlag(MachineInstr::FrameSetup);
  }

  // Make XMM regs spilled. X86 does not have ability of push/pop XMM.
  // It can be done by spilling XMMs to stack frame.
  for (unsigned i = CSI.size(); i != 0; --i) {
    unsigned Reg = CSI[i-1].getReg();
    if (X86::GR64RegClass.contains(Reg) || X86::GR32RegClass.contains(Reg))
      continue;
    // Add the callee-saved register as live-in. It's killed at the spill.
    MBB.addLiveIn(Reg);
    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);

    TII.storeRegToStackSlot(MBB, MI, Reg, true, CSI[i - 1].getFrameIdx(), RC,
                            TRI);
    --MI;
    MI->setFlag(MachineInstr::FrameSetup);
    ++MI;
  }

  return true;
}

bool X86FrameLowering::restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                                               MachineBasicBlock::iterator MI,
                                        const std::vector<CalleeSavedInfo> &CSI,
                                          const TargetRegisterInfo *TRI) const {
  if (CSI.empty())
    return false;

  // Don't restore CSRs in 32-bit EH funclets. Matches
  // spillCalleeSavedRegisters.
  if (isFuncletReturnInstr(MI) && STI.is32Bit() && STI.isOSWindows())
    return true;

  DebugLoc DL = MBB.findDebugLoc(MI);

  // Reload XMMs from stack frame.
  for (unsigned i = 0, e = CSI.size(); i != e; ++i) {
    unsigned Reg = CSI[i].getReg();
    if (X86::GR64RegClass.contains(Reg) ||
        X86::GR32RegClass.contains(Reg))
      continue;

    const TargetRegisterClass *RC = TRI->getMinimalPhysRegClass(Reg);
    TII.loadRegFromStackSlot(MBB, MI, Reg, CSI[i].getFrameIdx(), RC, TRI);
  }

  // POP GPRs.
  unsigned Opc = STI.is64Bit() ? X86::POP64r : X86::POP32r;
  for (unsigned i = 0, e = CSI.size(); i != e; ++i) {
    unsigned Reg = CSI[i].getReg();
    if (!X86::GR64RegClass.contains(Reg) &&
        !X86::GR32RegClass.contains(Reg))
      continue;

    BuildMI(MBB, MI, DL, TII.get(Opc), Reg)
        .setMIFlag(MachineInstr::FrameDestroy);
  }
  return true;
}

void X86FrameLowering::determineCalleeSaves(MachineFunction &MF,
                                            BitVector &SavedRegs,
                                            RegScavenger *RS) const {
  TargetFrameLowering::determineCalleeSaves(MF, SavedRegs, RS);

  MachineFrameInfo *MFI = MF.getFrameInfo();

  X86MachineFunctionInfo *X86FI = MF.getInfo<X86MachineFunctionInfo>();
  int64_t TailCallReturnAddrDelta = X86FI->getTCReturnAddrDelta();

  if (TailCallReturnAddrDelta < 0) {
    // create RETURNADDR area
    //   arg
    //   arg
    //   RETADDR
    //   { ...
    //     RETADDR area
    //     ...
    //   }
    //   [EBP]
    MFI->CreateFixedObject(-TailCallReturnAddrDelta,
                           TailCallReturnAddrDelta - SlotSize, true);
  }

  // Spill the BasePtr if it's used.
  if (TRI->hasBasePointer(MF)) {
    SavedRegs.set(TRI->getBaseRegister());

    // Allocate a spill slot for EBP if we have a base pointer and EH funclets.
    if (MF.getMMI().hasEHFunclets()) {
      int FI = MFI->CreateSpillStackObject(SlotSize, SlotSize);
      X86FI->setHasSEHFramePtrSave(true);
      X86FI->setSEHFramePtrSaveIndex(FI);
    }
  }
}

static bool
HasNestArgument(const MachineFunction *MF) {
  const Function *F = MF->getFunction();
  for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end();
       I != E; I++) {
    if (I->hasNestAttr())
      return true;
  }
  return false;
}

/// GetScratchRegister - Get a temp register for performing work in the
/// segmented stack and the Erlang/HiPE stack prologue. Depending on platform
/// and the properties of the function either one or two registers will be
/// needed. Set primary to true for the first register, false for the second.
static unsigned
GetScratchRegister(bool Is64Bit, bool IsLP64, const MachineFunction &MF, bool Primary) {
  CallingConv::ID CallingConvention = MF.getFunction()->getCallingConv();

  // Erlang stuff.
  if (CallingConvention == CallingConv::HiPE) {
    if (Is64Bit)
      return Primary ? X86::R14 : X86::R13;
    else
      return Primary ? X86::EBX : X86::EDI;
  }

  if (Is64Bit) {
    if (IsLP64)
      return Primary ? X86::R11 : X86::R12;
    else
      return Primary ? X86::R11D : X86::R12D;
  }

  bool IsNested = HasNestArgument(&MF);

  if (CallingConvention == CallingConv::X86_FastCall ||
      CallingConvention == CallingConv::Fast) {
    if (IsNested)
      report_fatal_error("Segmented stacks does not support fastcall with "
                         "nested function.");
    return Primary ? X86::EAX : X86::ECX;
  }
  if (IsNested)
    return Primary ? X86::EDX : X86::EAX;
  return Primary ? X86::ECX : X86::EAX;
}

// The stack limit in the TCB is set to this many bytes above the actual stack
// limit.
static const uint64_t kSplitStackAvailable = 256;

void X86FrameLowering::adjustForSegmentedStacks(
    MachineFunction &MF, MachineBasicBlock &PrologueMBB) const {
  MachineFrameInfo *MFI = MF.getFrameInfo();
  uint64_t StackSize;
  unsigned TlsReg, TlsOffset;
  DebugLoc DL;

  unsigned ScratchReg = GetScratchRegister(Is64Bit, IsLP64, MF, true);
  assert(!MF.getRegInfo().isLiveIn(ScratchReg) &&
         "Scratch register is live-in");

  if (MF.getFunction()->isVarArg())
    report_fatal_error("Segmented stacks do not support vararg functions.");
  if (!STI.isTargetLinux() && !STI.isTargetDarwin() && !STI.isTargetWin32() &&
      !STI.isTargetWin64() && !STI.isTargetFreeBSD() &&
      !STI.isTargetDragonFly())
    report_fatal_error("Segmented stacks not supported on this platform.");

  // Eventually StackSize will be calculated by a link-time pass; which will
  // also decide whether checking code needs to be injected into this particular
  // prologue.
  StackSize = MFI->getStackSize();

  // Do not generate a prologue for functions with a stack of size zero
  if (StackSize == 0)
    return;

  MachineBasicBlock *allocMBB = MF.CreateMachineBasicBlock();
  MachineBasicBlock *checkMBB = MF.CreateMachineBasicBlock();
  X86MachineFunctionInfo *X86FI = MF.getInfo<X86MachineFunctionInfo>();
  bool IsNested = false;

  // We need to know if the function has a nest argument only in 64 bit mode.
  if (Is64Bit)
    IsNested = HasNestArgument(&MF);

  // The MOV R10, RAX needs to be in a different block, since the RET we emit in
  // allocMBB needs to be last (terminating) instruction.

  for (const auto &LI : PrologueMBB.liveins()) {
    allocMBB->addLiveIn(LI);
    checkMBB->addLiveIn(LI);
  }

  if (IsNested)
    allocMBB->addLiveIn(IsLP64 ? X86::R10 : X86::R10D);

  MF.push_front(allocMBB);
  MF.push_front(checkMBB);

  // When the frame size is less than 256 we just compare the stack
  // boundary directly to the value of the stack pointer, per gcc.
  bool CompareStackPointer = StackSize < kSplitStackAvailable;

  // Read the limit off the current stacklet off the stack_guard location.
  if (Is64Bit) {
    if (STI.isTargetLinux()) {
      TlsReg = X86::FS;
      TlsOffset = IsLP64 ? 0x70 : 0x40;
    } else if (STI.isTargetDarwin()) {
      TlsReg = X86::GS;
      TlsOffset = 0x60 + 90*8; // See pthread_machdep.h. Steal TLS slot 90.
    } else if (STI.isTargetWin64()) {
      TlsReg = X86::GS;
      TlsOffset = 0x28; // pvArbitrary, reserved for application use
    } else if (STI.isTargetFreeBSD()) {
      TlsReg = X86::FS;
      TlsOffset = 0x18;
    } else if (STI.isTargetDragonFly()) {
      TlsReg = X86::FS;
      TlsOffset = 0x20; // use tls_tcb.tcb_segstack
    } else {
      report_fatal_error("Segmented stacks not supported on this platform.");
    }

    if (CompareStackPointer)
      ScratchReg = IsLP64 ? X86::RSP : X86::ESP;
    else
      BuildMI(checkMBB, DL, TII.get(IsLP64 ? X86::LEA64r : X86::LEA64_32r), ScratchReg).addReg(X86::RSP)
        .addImm(1).addReg(0).addImm(-StackSize).addReg(0);

    BuildMI(checkMBB, DL, TII.get(IsLP64 ? X86::CMP64rm : X86::CMP32rm)).addReg(ScratchReg)
      .addReg(0).addImm(1).addReg(0).addImm(TlsOffset).addReg(TlsReg);
  } else {
    if (STI.isTargetLinux()) {
      TlsReg = X86::GS;
      TlsOffset = 0x30;
    } else if (STI.isTargetDarwin()) {
      TlsReg = X86::GS;
      TlsOffset = 0x48 + 90*4;
    } else if (STI.isTargetWin32()) {
      TlsReg = X86::FS;
      TlsOffset = 0x14; // pvArbitrary, reserved for application use
    } else if (STI.isTargetDragonFly()) {
      TlsReg = X86::FS;
      TlsOffset = 0x10; // use tls_tcb.tcb_segstack
    } else if (STI.isTargetFreeBSD()) {
      report_fatal_error("Segmented stacks not supported on FreeBSD i386.");
    } else {
      report_fatal_error("Segmented stacks not supported on this platform.");
    }

    if (CompareStackPointer)
      ScratchReg = X86::ESP;
    else
      BuildMI(checkMBB, DL, TII.get(X86::LEA32r), ScratchReg).addReg(X86::ESP)
        .addImm(1).addReg(0).addImm(-StackSize).addReg(0);

    if (STI.isTargetLinux() || STI.isTargetWin32() || STI.isTargetWin64() ||
        STI.isTargetDragonFly()) {
      BuildMI(checkMBB, DL, TII.get(X86::CMP32rm)).addReg(ScratchReg)
        .addReg(0).addImm(0).addReg(0).addImm(TlsOffset).addReg(TlsReg);
    } else if (STI.isTargetDarwin()) {

      // TlsOffset doesn't fit into a mod r/m byte so we need an extra register.
      unsigned ScratchReg2;
      bool SaveScratch2;
      if (CompareStackPointer) {
        // The primary scratch register is available for holding the TLS offset.
        ScratchReg2 = GetScratchRegister(Is64Bit, IsLP64, MF, true);
        SaveScratch2 = false;
      } else {
        // Need to use a second register to hold the TLS offset
        ScratchReg2 = GetScratchRegister(Is64Bit, IsLP64, MF, false);

        // Unfortunately, with fastcc the second scratch register may hold an
        // argument.
        SaveScratch2 = MF.getRegInfo().isLiveIn(ScratchReg2);
      }

      // If Scratch2 is live-in then it needs to be saved.
      assert((!MF.getRegInfo().isLiveIn(ScratchReg2) || SaveScratch2) &&
             "Scratch register is live-in and not saved");

      if (SaveScratch2)
        BuildMI(checkMBB, DL, TII.get(X86::PUSH32r))
          .addReg(ScratchReg2, RegState::Kill);

      BuildMI(checkMBB, DL, TII.get(X86::MOV32ri), ScratchReg2)
        .addImm(TlsOffset);
      BuildMI(checkMBB, DL, TII.get(X86::CMP32rm))
        .addReg(ScratchReg)
        .addReg(ScratchReg2).addImm(1).addReg(0)
        .addImm(0)
        .addReg(TlsReg);

      if (SaveScratch2)
        BuildMI(checkMBB, DL, TII.get(X86::POP32r), ScratchReg2);
    }
  }

  // This jump is taken if SP >= (Stacklet Limit + Stack Space required).
  // It jumps to normal execution of the function body.
  BuildMI(checkMBB, DL, TII.get(X86::JA_1)).addMBB(&PrologueMBB);

  // On 32 bit we first push the arguments size and then the frame size. On 64
  // bit, we pass the stack frame size in r10 and the argument size in r11.
  if (Is64Bit) {
    // Functions with nested arguments use R10, so it needs to be saved across
    // the call to _morestack

    const unsigned RegAX = IsLP64 ? X86::RAX : X86::EAX;
    const unsigned Reg10 = IsLP64 ? X86::R10 : X86::R10D;
    const unsigned Reg11 = IsLP64 ? X86::R11 : X86::R11D;
    const unsigned MOVrr = IsLP64 ? X86::MOV64rr : X86::MOV32rr;
    const unsigned MOVri = IsLP64 ? X86::MOV64ri : X86::MOV32ri;

    if (IsNested)
      BuildMI(allocMBB, DL, TII.get(MOVrr), RegAX).addReg(Reg10);

    BuildMI(allocMBB, DL, TII.get(MOVri), Reg10)
      .addImm(StackSize);
    BuildMI(allocMBB, DL, TII.get(MOVri), Reg11)
      .addImm(X86FI->getArgumentStackSize());
  } else {
    BuildMI(allocMBB, DL, TII.get(X86::PUSHi32))
      .addImm(X86FI->getArgumentStackSize());
    BuildMI(allocMBB, DL, TII.get(X86::PUSHi32))
      .addImm(StackSize);
  }

  // __morestack is in libgcc
  if (Is64Bit && MF.getTarget().getCodeModel() == CodeModel::Large) {
    // Under the large code model, we cannot assume that __morestack lives
    // within 2^31 bytes of the call site, so we cannot use pc-relative
    // addressing. We cannot perform the call via a temporary register,
    // as the rax register may be used to store the static chain, and all
    // other suitable registers may be either callee-save or used for
    // parameter passing. We cannot use the stack at this point either
    // because __morestack manipulates the stack directly.
    //
    // To avoid these issues, perform an indirect call via a read-only memory
    // location containing the address.
    //
    // This solution is not perfect, as it assumes that the .rodata section
    // is laid out within 2^31 bytes of each function body, but this seems
    // to be sufficient for JIT.
    BuildMI(allocMBB, DL, TII.get(X86::CALL64m))
        .addReg(X86::RIP)
        .addImm(0)
        .addReg(0)
        .addExternalSymbol("__morestack_addr")
        .addReg(0);
    MF.getMMI().setUsesMorestackAddr(true);
  } else {
    if (Is64Bit)
      BuildMI(allocMBB, DL, TII.get(X86::CALL64pcrel32))
        .addExternalSymbol("__morestack");
    else
      BuildMI(allocMBB, DL, TII.get(X86::CALLpcrel32))
        .addExternalSymbol("__morestack");
  }

  if (IsNested)
    BuildMI(allocMBB, DL, TII.get(X86::MORESTACK_RET_RESTORE_R10));
  else
    BuildMI(allocMBB, DL, TII.get(X86::MORESTACK_RET));

  allocMBB->addSuccessor(&PrologueMBB);

  checkMBB->addSuccessor(allocMBB);
  checkMBB->addSuccessor(&PrologueMBB);

#ifdef XDEBUG
  MF.verify();
#endif
}

/// Erlang programs may need a special prologue to handle the stack size they
/// might need at runtime. That is because Erlang/OTP does not implement a C
/// stack but uses a custom implementation of hybrid stack/heap architecture.
/// (for more information see Eric Stenman's Ph.D. thesis:
/// http://publications.uu.se/uu/fulltext/nbn_se_uu_diva-2688.pdf)
///
/// CheckStack:
///       temp0 = sp - MaxStack
///       if( temp0 < SP_LIMIT(P) ) goto IncStack else goto OldStart
/// OldStart:
///       ...
/// IncStack:
///       call inc_stack   # doubles the stack space
///       temp0 = sp - MaxStack
///       if( temp0 < SP_LIMIT(P) ) goto IncStack else goto OldStart
void X86FrameLowering::adjustForHiPEPrologue(
    MachineFunction &MF, MachineBasicBlock &PrologueMBB) const {
  MachineFrameInfo *MFI = MF.getFrameInfo();
  DebugLoc DL;
  // HiPE-specific values
  const unsigned HipeLeafWords = 24;
  const unsigned CCRegisteredArgs = Is64Bit ? 6 : 5;
  const unsigned Guaranteed = HipeLeafWords * SlotSize;
  unsigned CallerStkArity = MF.getFunction()->arg_size() > CCRegisteredArgs ?
                            MF.getFunction()->arg_size() - CCRegisteredArgs : 0;
  unsigned MaxStack = MFI->getStackSize() + CallerStkArity*SlotSize + SlotSize;

  assert(STI.isTargetLinux() &&
         "HiPE prologue is only supported on Linux operating systems.");

  // Compute the largest caller's frame that is needed to fit the callees'
  // frames. This 'MaxStack' is computed from:
  //
  // a) the fixed frame size, which is the space needed for all spilled temps,
  // b) outgoing on-stack parameter areas, and
  // c) the minimum stack space this function needs to make available for the
  //    functions it calls (a tunable ABI property).
  if (MFI->hasCalls()) {
    unsigned MoreStackForCalls = 0;

    for (MachineFunction::iterator MBBI = MF.begin(), MBBE = MF.end();
         MBBI != MBBE; ++MBBI)
      for (MachineBasicBlock::iterator MI = MBBI->begin(), ME = MBBI->end();
           MI != ME; ++MI) {
        if (!MI->isCall())
          continue;

        // Get callee operand.
        const MachineOperand &MO = MI->getOperand(0);

        // Only take account of global function calls (no closures etc.).
        if (!MO.isGlobal())
          continue;

        const Function *F = dyn_cast<Function>(MO.getGlobal());
        if (!F)
          continue;

        // Do not update 'MaxStack' for primitive and built-in functions
        // (encoded with names either starting with "erlang."/"bif_" or not
        // having a ".", such as a simple <Module>.<Function>.<Arity>, or an
        // "_", such as the BIF "suspend_0") as they are executed on another
        // stack.
        if (F->getName().find("erlang.") != StringRef::npos ||
            F->getName().find("bif_") != StringRef::npos ||
            F->getName().find_first_of("._") == StringRef::npos)
          continue;

        unsigned CalleeStkArity =
          F->arg_size() > CCRegisteredArgs ? F->arg_size()-CCRegisteredArgs : 0;
        if (HipeLeafWords - 1 > CalleeStkArity)
          MoreStackForCalls = std::max(MoreStackForCalls,
                               (HipeLeafWords - 1 - CalleeStkArity) * SlotSize);
      }
    MaxStack += MoreStackForCalls;
  }

  // If the stack frame needed is larger than the guaranteed then runtime checks
  // and calls to "inc_stack_0" BIF should be inserted in the assembly prologue.
  if (MaxStack > Guaranteed) {
    MachineBasicBlock *stackCheckMBB = MF.CreateMachineBasicBlock();
    MachineBasicBlock *incStackMBB = MF.CreateMachineBasicBlock();

    for (const auto &LI : PrologueMBB.liveins()) {
      stackCheckMBB->addLiveIn(LI);
      incStackMBB->addLiveIn(LI);
    }

    MF.push_front(incStackMBB);
    MF.push_front(stackCheckMBB);

    unsigned ScratchReg, SPReg, PReg, SPLimitOffset;
    unsigned LEAop, CMPop, CALLop;
    if (Is64Bit) {
      SPReg = X86::RSP;
      PReg  = X86::RBP;
      LEAop = X86::LEA64r;
      CMPop = X86::CMP64rm;
      CALLop = X86::CALL64pcrel32;
      SPLimitOffset = 0x90;
    } else {
      SPReg = X86::ESP;
      PReg  = X86::EBP;
      LEAop = X86::LEA32r;
      CMPop = X86::CMP32rm;
      CALLop = X86::CALLpcrel32;
      SPLimitOffset = 0x4c;
    }

    ScratchReg = GetScratchRegister(Is64Bit, IsLP64, MF, true);
    assert(!MF.getRegInfo().isLiveIn(ScratchReg) &&
           "HiPE prologue scratch register is live-in");

    // Create new MBB for StackCheck:
    addRegOffset(BuildMI(stackCheckMBB, DL, TII.get(LEAop), ScratchReg),
                 SPReg, false, -MaxStack);
    // SPLimitOffset is in a fixed heap location (pointed by BP).
    addRegOffset(BuildMI(stackCheckMBB, DL, TII.get(CMPop))
                 .addReg(ScratchReg), PReg, false, SPLimitOffset);
    BuildMI(stackCheckMBB, DL, TII.get(X86::JAE_1)).addMBB(&PrologueMBB);

    // Create new MBB for IncStack:
    BuildMI(incStackMBB, DL, TII.get(CALLop)).
      addExternalSymbol("inc_stack_0");
    addRegOffset(BuildMI(incStackMBB, DL, TII.get(LEAop), ScratchReg),
                 SPReg, false, -MaxStack);
    addRegOffset(BuildMI(incStackMBB, DL, TII.get(CMPop))
                 .addReg(ScratchReg), PReg, false, SPLimitOffset);
    BuildMI(incStackMBB, DL, TII.get(X86::JLE_1)).addMBB(incStackMBB);

    stackCheckMBB->addSuccessor(&PrologueMBB, 99);
    stackCheckMBB->addSuccessor(incStackMBB, 1);
    incStackMBB->addSuccessor(&PrologueMBB, 99);
    incStackMBB->addSuccessor(incStackMBB, 1);
  }
#ifdef XDEBUG
  MF.verify();
#endif
}

bool X86FrameLowering::adjustStackWithPops(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI, DebugLoc DL, int Offset) const {

  if (Offset <= 0)
    return false;

  if (Offset % SlotSize)
    return false;

  int NumPops = Offset / SlotSize;
  // This is only worth it if we have at most 2 pops.
  if (NumPops != 1 && NumPops != 2)
    return false;

  // Handle only the trivial case where the adjustment directly follows
  // a call. This is the most common one, anyway.
  if (MBBI == MBB.begin())
    return false;
  MachineBasicBlock::iterator Prev = std::prev(MBBI);
  if (!Prev->isCall() || !Prev->getOperand(1).isRegMask())
    return false;

  unsigned Regs[2];
  unsigned FoundRegs = 0;

  auto RegMask = Prev->getOperand(1);

  auto &RegClass =
      Is64Bit ? X86::GR64_NOREX_NOSPRegClass : X86::GR32_NOREX_NOSPRegClass;
  // Try to find up to NumPops free registers.
  for (auto Candidate : RegClass) {

    // Poor man's liveness:
    // Since we're immediately after a call, any register that is clobbered
    // by the call and not defined by it can be considered dead.
    if (!RegMask.clobbersPhysReg(Candidate))
      continue;

    bool IsDef = false;
    for (const MachineOperand &MO : Prev->implicit_operands()) {
      if (MO.isReg() && MO.isDef() && MO.getReg() == Candidate) {
        IsDef = true;
        break;
      }
    }

    if (IsDef)
      continue;

    Regs[FoundRegs++] = Candidate;
    if (FoundRegs == (unsigned)NumPops)
      break;
  }

  if (FoundRegs == 0)
    return false;

  // If we found only one free register, but need two, reuse the same one twice.
  while (FoundRegs < (unsigned)NumPops)
    Regs[FoundRegs++] = Regs[0];

  for (int i = 0; i < NumPops; ++i)
    BuildMI(MBB, MBBI, DL, 
            TII.get(STI.is64Bit() ? X86::POP64r : X86::POP32r), Regs[i]);

  return true;
}

void X86FrameLowering::
eliminateCallFramePseudoInstr(MachineFunction &MF, MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator I) const {
  bool reserveCallFrame = hasReservedCallFrame(MF);
  unsigned Opcode = I->getOpcode();
  bool isDestroy = Opcode == TII.getCallFrameDestroyOpcode();
  DebugLoc DL = I->getDebugLoc();
  uint64_t Amount = !reserveCallFrame ? I->getOperand(0).getImm() : 0;
  uint64_t InternalAmt = (isDestroy || Amount) ? I->getOperand(1).getImm() : 0;
  I = MBB.erase(I);

  if (!reserveCallFrame) {
    // If the stack pointer can be changed after prologue, turn the
    // adjcallstackup instruction into a 'sub ESP, <amt>' and the
    // adjcallstackdown instruction into 'add ESP, <amt>'
    if (Amount == 0)
      return;

    // We need to keep the stack aligned properly.  To do this, we round the
    // amount of space needed for the outgoing arguments up to the next
    // alignment boundary.
    unsigned StackAlign = getStackAlignment();
    Amount = RoundUpToAlignment(Amount, StackAlign);

    // Factor out the amount that gets handled inside the sequence
    // (Pushes of argument for frame setup, callee pops for frame destroy)
    Amount -= InternalAmt;

    if (Amount) {
      // Add Amount to SP to destroy a frame, and subtract to setup.
      int Offset = isDestroy ? Amount : -Amount;

      if (!(MF.getFunction()->optForMinSize() && 
            adjustStackWithPops(MBB, I, DL, Offset)))
        BuildStackAdjustment(MBB, I, DL, Offset, /*InEpilogue=*/false);
    }

    return;
  }

  if (isDestroy && InternalAmt) {
    // If we are performing frame pointer elimination and if the callee pops
    // something off the stack pointer, add it back.  We do this until we have
    // more advanced stack pointer tracking ability.
    // We are not tracking the stack pointer adjustment by the callee, so make
    // sure we restore the stack pointer immediately after the call, there may
    // be spill code inserted between the CALL and ADJCALLSTACKUP instructions.
    MachineBasicBlock::iterator B = MBB.begin();
    while (I != B && !std::prev(I)->isCall())
      --I;
    BuildStackAdjustment(MBB, I, DL, -InternalAmt, /*InEpilogue=*/false);
  }
}

bool X86FrameLowering::canUseAsEpilogue(const MachineBasicBlock &MBB) const {
  assert(MBB.getParent() && "Block is not attached to a function!");

  if (canUseLEAForSPInEpilogue(*MBB.getParent()))
    return true;

  // If we cannot use LEA to adjust SP, we may need to use ADD, which
  // clobbers the EFLAGS. Check that none of the terminators reads the
  // EFLAGS, and if one uses it, conservatively assume this is not
  // safe to insert the epilogue here.
  return !terminatorsNeedFlagsAsInput(MBB);
}

MachineBasicBlock::iterator X86FrameLowering::restoreWin32EHStackPointers(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    DebugLoc DL, bool RestoreSP) const {
  assert(STI.isTargetWindowsMSVC() && "funclets only supported in MSVC env");
  assert(STI.isTargetWin32() && "EBP/ESI restoration only required on win32");
  assert(STI.is32Bit() && !Uses64BitFramePtr &&
         "restoring EBP/ESI on non-32-bit target");

  MachineFunction &MF = *MBB.getParent();
  unsigned FramePtr = TRI->getFrameRegister(MF);
  unsigned BasePtr = TRI->getBaseRegister();
  MachineModuleInfo &MMI = MF.getMMI();
  const Function *Fn = MF.getFunction();
  WinEHFuncInfo &FuncInfo = MMI.getWinEHFuncInfo(Fn);
  X86MachineFunctionInfo *X86FI = MF.getInfo<X86MachineFunctionInfo>();
  MachineFrameInfo *MFI = MF.getFrameInfo();

  // FIXME: Don't set FrameSetup flag in catchret case.

  int FI = FuncInfo.EHRegNodeFrameIndex;
  int EHRegSize = MFI->getObjectSize(FI);

  if (RestoreSP) {
    // MOV32rm -EHRegSize(%ebp), %esp
    addRegOffset(BuildMI(MBB, MBBI, DL, TII.get(X86::MOV32rm), X86::ESP),
                 X86::EBP, true, -EHRegSize)
        .setMIFlag(MachineInstr::FrameSetup);
  }

  unsigned UsedReg;
  int EHRegOffset = getFrameIndexReference(MF, FI, UsedReg);
  int EndOffset = -EHRegOffset - EHRegSize;
  FuncInfo.EHRegNodeEndOffset = EndOffset;
  assert(EndOffset >= 0 &&
         "end of registration object above normal EBP position!");

  if (UsedReg == FramePtr) {
    // ADD $offset, %ebp
    assert(UsedReg == FramePtr);
    unsigned ADDri = getADDriOpcode(false, EndOffset);
    BuildMI(MBB, MBBI, DL, TII.get(ADDri), FramePtr)
        .addReg(FramePtr)
        .addImm(EndOffset)
        .setMIFlag(MachineInstr::FrameSetup)
        ->getOperand(3)
        .setIsDead();
  } else {
    assert(UsedReg == BasePtr);
    // LEA offset(%ebp), %esi
    addRegOffset(BuildMI(MBB, MBBI, DL, TII.get(X86::LEA32r), BasePtr),
                 FramePtr, false, EndOffset)
        .setMIFlag(MachineInstr::FrameSetup);
    // MOV32rm SavedEBPOffset(%esi), %ebp
    assert(X86FI->getHasSEHFramePtrSave());
    int Offset =
        getFrameIndexReference(MF, X86FI->getSEHFramePtrSaveIndex(), UsedReg);
    assert(UsedReg == BasePtr);
    addRegOffset(BuildMI(MBB, MBBI, DL, TII.get(X86::MOV32rm), FramePtr),
                 UsedReg, true, Offset)
        .setMIFlag(MachineInstr::FrameSetup);
  }
  return MBBI;
}
