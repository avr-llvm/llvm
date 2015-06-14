//===-- AVRExpandPseudoInsts.cpp - Expand pseudo instructions -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions. This pass should be run after register allocation but before
// the post-regalloc scheduling pass.
//
//===----------------------------------------------------------------------===//

#include "AVRConfig.h"

#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Target/TargetRegisterInfo.h"

#include "AVR.h"
#include "AVRInstrInfo.h"
#include "AVRTargetMachine.h"

#define UNUSED_VARIABLE(x) (void)(x)

namespace llvm {

class AVRExpandPseudo : public MachineFunctionPass {
public:
  static char ID;
  const TargetRegisterInfo *TRI;
  const TargetInstrInfo *TII;
  AVRExpandPseudo() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  const char *getPassName() const override {
    return "AVR pseudo instruction expansion pass";
  }
private:
  // LLRTs: line length reduction typedefs
  typedef MachineBasicBlock Block;
  typedef Block::iterator   BlockIt;

  bool expandMBB(Block &MBB);
  bool expandMI(Block &MBB, BlockIt MBBI);
  template <unsigned OP>
  bool expand(Block & MBB, BlockIt MBBI);

  MachineInstrBuilder buildMI(Block & MBB, BlockIt MBBI, unsigned Opcode) {
    return BuildMI(MBB, MBBI, MBBI->getDebugLoc(), TII->get(Opcode));
  }
  MachineInstrBuilder buildMI(Block & MBB, BlockIt MBBI, unsigned Opcode, unsigned DstReg) {
    return BuildMI(MBB, MBBI, MBBI->getDebugLoc(), TII->get(Opcode), DstReg);
  }

  void splitRegs(unsigned Reg, unsigned &LoReg, unsigned &HiReg) {
    LoReg = TRI->getSubReg(Reg, AVR::sub_lo);
    HiReg = TRI->getSubReg(Reg, AVR::sub_hi);
  }
};

char AVRExpandPseudo::ID = 0;


bool
AVRExpandPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  BlockIt MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    BlockIt NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool
AVRExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  bool Modified = false;
  const AVRTargetMachine &TM = (const AVRTargetMachine&)MF.getTarget();
  TRI = TM.getSubtargetImpl()->getRegisterInfo();
  TII = TM.getSubtargetImpl()->getInstrInfo();

  typedef MachineFunction::iterator FuncIt;
  for (FuncIt MFI = MF.begin(), E = MF.end(); MFI != E; ++MFI) {
    Modified |= expandMBB(*MFI);
  }

  return Modified;
}

template <>
bool
AVRExpandPseudo::expand<AVR::ADDWRdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  OpLo = AVR::ADDRdRr;
  OpHi = AVR::ADCRdRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill))
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill))
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  UNUSED_VARIABLE(MIBLO);

  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::ADCWRdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  OpLo = AVR::ADCRdRr;
  OpHi = AVR::ADCRdRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill))
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));
  // SREG is always implicitly killed
  MIBLO->getOperand(4).setIsKill();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill))
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::SUBWRdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  OpLo = AVR::SUBRdRr;
  OpHi = AVR::SBCRdRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill))
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill))
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));
  
  UNUSED_VARIABLE(MIBLO);
  
  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::SUBIWRdK>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  OpLo = AVR::SUBIRdK;
  OpHi = AVR::SBCIRdK;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(SrcIsKill));

  switch (MI.getOperand(2).getType()) {
    case MachineOperand::MO_GlobalAddress: {
      const GlobalValue *GV = MI.getOperand(2).getGlobal();
      int64_t Offs = MI.getOperand(2).getOffset();
      unsigned TF = MI.getOperand(2).getTargetFlags();
      MIBLO.addGlobalAddress(GV, Offs, TF | AVRII::MO_NEG | AVRII::MO_LO);
      MIBHI.addGlobalAddress(GV, Offs, TF | AVRII::MO_NEG | AVRII::MO_HI);
      break;
    }
    case MachineOperand::MO_Immediate: {
      unsigned Imm = MI.getOperand(2).getImm();
      MIBLO.addImm(Imm & 0xff);
      MIBHI.addImm((Imm >> 8) & 0xff);
      break;
    }
    default: llvm_unreachable("Unknown operand type!");
  }

  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::SBCWRdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  OpLo = AVR::SBCRdRr;
  OpHi = AVR::SBCRdRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill))
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));
  // SREG is always implicitly killed
  MIBLO->getOperand(4).setIsKill();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill))
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::SBCIWRdK>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  unsigned Imm = MI.getOperand(2).getImm();
  unsigned Lo8 = Imm & 0xff;
  unsigned Hi8 = (Imm >> 8) & 0xff;
  OpLo = AVR::SBCIRdK;
  OpHi = AVR::SBCIRdK;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(SrcIsKill))
    . addImm(Lo8);
  // SREG is always implicitly killed
  MIBLO->getOperand(4).setIsKill();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(SrcIsKill))
    . addImm(Hi8);

  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  MIBHI->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::ANDWRdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  OpLo = AVR::ANDRdRr;
  OpHi = AVR::ANDRdRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill))
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));
  // SREG is always implicitly dead
  MIBLO->getOperand(3).setIsDead();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill))
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::ANDIWRdK>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  unsigned Imm = MI.getOperand(2).getImm();
  unsigned Lo8 = Imm & 0xff;
  unsigned Hi8 = (Imm >> 8) & 0xff;
  OpLo = AVR::ANDIRdK;
  OpHi = AVR::ANDIRdK;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(SrcIsKill))
    . addImm(Lo8);
  // SREG is always implicitly dead
  MIBLO->getOperand(3).setIsDead();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(SrcIsKill))
    . addImm(Hi8);

  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::ORWRdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  OpLo = AVR::ORRdRr;
  OpHi = AVR::ORRdRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill))
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));
  // SREG is always implicitly dead
  MIBLO->getOperand(3).setIsDead();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill))
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::ORIWRdK>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  unsigned Imm = MI.getOperand(2).getImm();
  unsigned Lo8 = Imm & 0xff;
  unsigned Hi8 = (Imm >> 8) & 0xff;
  OpLo = AVR::ORIRdK;
  OpHi = AVR::ORIRdK;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(SrcIsKill))
    . addImm(Lo8);
  // SREG is always implicitly dead
  MIBLO->getOperand(3).setIsDead();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(SrcIsKill))
    . addImm(Hi8);

  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::EORWRdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(3).isDead();
  OpLo = AVR::EORRdRr;
  OpHi = AVR::EORRdRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill))
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));
  // SREG is always implicitly dead
  MIBLO->getOperand(3).setIsDead();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill))
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead) MIBHI->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::COMWRd>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = AVR::COMRd;
  OpHi = AVR::COMRd;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill));
  // SREG is always implicitly dead
  MIBLO->getOperand(2).setIsDead();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill));

  if (ImpIsDead) MIBHI->getOperand(2).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::CPWRdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = AVR::CPRdRr;
  OpHi = AVR::CPCRdRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, getKillRegState(DstIsKill))
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, getKillRegState(DstIsKill))
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  UNUSED_VARIABLE(MIBLO);

  if (ImpIsDead) MIBHI->getOperand(2).setIsDead();
  // SREG is always implicitly killed
  MIBHI->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::CPCWRdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = AVR::CPCRdRr;
  OpHi = AVR::CPCRdRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, getKillRegState(DstIsKill))
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));
  // SREG is always implicitly killed
  MIBLO->getOperand(3).setIsKill();

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, getKillRegState(DstIsKill))
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  if (ImpIsDead) MIBHI->getOperand(2).setIsDead();
  // SREG is always implicitly killed
  MIBHI->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::LDIWRdK>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  OpLo = AVR::LDIRdK;
  OpHi = AVR::LDIRdK;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead));

  switch (MI.getOperand(1).getType()) {
    case MachineOperand::MO_GlobalAddress: {
      const GlobalValue *GV = MI.getOperand(1).getGlobal();
      int64_t Offs = MI.getOperand(1).getOffset();
      unsigned TF = MI.getOperand(1).getTargetFlags();
      MIBLO.addGlobalAddress(GV, Offs, TF | AVRII::MO_LO);
      MIBHI.addGlobalAddress(GV, Offs, TF | AVRII::MO_HI);
      break;
    }
    case MachineOperand::MO_BlockAddress: {
      const BlockAddress *BA = MI.getOperand(1).getBlockAddress();
      unsigned TF = MI.getOperand(1).getTargetFlags();
      MIBLO.addOperand(MachineOperand::CreateBA(BA, TF | AVRII::MO_LO));
      MIBHI.addOperand(MachineOperand::CreateBA(BA, TF | AVRII::MO_HI));
      break;
    }
    case MachineOperand::MO_Immediate: {
      unsigned Imm = MI.getOperand(1).getImm();
      MIBLO.addImm(Imm & 0xff);
      MIBHI.addImm((Imm >> 8) & 0xff);
      break;
    }
    default: llvm_unreachable("Unknown operand type!");
  }

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::LDSWRdK>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  OpLo = AVR::LDSRdK;
  OpHi = AVR::LDSRdK;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead));

  switch (MI.getOperand(1).getType()) {
    case MachineOperand::MO_GlobalAddress: {
      const GlobalValue *GV = MI.getOperand(1).getGlobal();
      int64_t Offs = MI.getOperand(1).getOffset();
      unsigned TF = MI.getOperand(1).getTargetFlags();
      MIBLO.addGlobalAddress(GV, Offs, TF);
      MIBHI.addGlobalAddress(GV, Offs + 1, TF);
      break;
    }
    case MachineOperand::MO_Immediate: {
      unsigned Imm = MI.getOperand(1).getImm();
      MIBLO.addImm(Imm);
      MIBHI.addImm(Imm + 1);
      break;
    }
    default: llvm_unreachable("Unknown operand type!");
  }

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::LDWRdPtr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  OpLo = AVR::LDRdPtr;
  OpHi = AVR::LDDRdPtrQ;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(SrcReg);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(SrcReg, getKillRegState(SrcIsKill))
    . addImm(1);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::LDWRdPtrPi>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsDead = MI.getOperand(1).isKill();
  OpLo = AVR::LDRdPtrPi;
  OpHi = AVR::LDRdPtrPi;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(SrcReg, RegState::Define)
    . addReg(SrcReg, RegState::Kill);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(SrcReg, RegState::Define | getDeadRegState(SrcIsDead))
    . addReg(SrcReg, RegState::Kill);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::LDWRdPtrPd>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  //:TODO: verify this expansion
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsDead = MI.getOperand(1).isKill();
  OpLo = AVR::LDRdPtrPd;
  OpHi = AVR::LDRdPtrPd;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(SrcReg, RegState::Define)
    . addReg(SrcReg, RegState::Kill);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(SrcReg, RegState::Define | getDeadRegState(SrcIsDead))
    . addReg(SrcReg, RegState::Kill);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::LDDWRdPtrQ>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  unsigned Imm = MI.getOperand(2).getImm();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  OpLo = AVR::LDDRdPtrQ;
  OpHi = AVR::LDDRdPtrQ;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  assert(Imm < 63 && "Offset is out of range");
  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(SrcReg)
    . addImm(Imm);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(SrcReg, getKillRegState(SrcIsKill))
    . addImm(Imm + 1);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::STSWKRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  OpLo = AVR::STSKRr;
  OpHi = AVR::STSKRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);

  // Write the high byte first in case this address belongs to a special
  // I/O address with a special temporary register.
  auto MIBHI = buildMI(MBB, MBBI, OpHi);
  auto MIBLO = buildMI(MBB, MBBI, OpLo);

  switch (MI.getOperand(0).getType()) {
    case MachineOperand::MO_GlobalAddress: {
      const GlobalValue *GV = MI.getOperand(0).getGlobal();
      int64_t Offs = MI.getOperand(0).getOffset();
      unsigned TF = MI.getOperand(0).getTargetFlags();
      MIBLO.addGlobalAddress(GV, Offs, TF);
      MIBHI.addGlobalAddress(GV, Offs + 1, TF);
      break;
    }
    case MachineOperand::MO_Immediate: {
      unsigned Imm = MI.getOperand(0).getImm();
      MIBLO.addImm(Imm);
      MIBHI.addImm(Imm + 1);
      break;
    }
    default: llvm_unreachable("Unknown operand type!");
  }

  MIBLO.addReg(SrcLoReg, getKillRegState(SrcIsKill));
  MIBHI.addReg(SrcHiReg, getKillRegState(SrcIsKill));

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::STWPtrRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(1).isKill();
  OpLo = AVR::STPtrRr;
  OpHi = AVR::STDPtrQRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);

  //:TODO: need to reverse this order like inw and stsw?
  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstReg)
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstReg, getKillRegState(DstIsKill))
    . addImm(1)
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::STWPtrPiRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  unsigned Imm = MI.getOperand(3).getImm();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(2).isKill();
  OpLo = AVR::STPtrPiRr;
  OpHi = AVR::STPtrPiRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstReg, RegState::Define)
    . addReg(DstReg, RegState::Kill)
    . addReg(SrcLoReg, getKillRegState(SrcIsKill))
    . addImm(Imm);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstReg, RegState::Kill)
    . addReg(SrcHiReg, getKillRegState(SrcIsKill))
    . addImm(Imm);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::STWPtrPdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  //:TODO: verify this expansion
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  unsigned Imm = MI.getOperand(3).getImm();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(2).isKill();
  OpLo = AVR::STPtrPdRr;
  OpHi = AVR::STPtrPdRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);

  assert(DstReg != SrcReg && "SrcReg and DstReg cannot be the same");

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstReg, RegState::Define)
    . addReg(DstReg, RegState::Kill)
    . addReg(SrcHiReg, getKillRegState(SrcIsKill))
    . addImm(Imm);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstReg, RegState::Kill)
    . addReg(SrcLoReg, getKillRegState(SrcIsKill))
    . addImm(Imm);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::STDWPtrQRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(2).getReg();
  unsigned Imm = MI.getOperand(1).getImm();
  bool DstIsKill = MI.getOperand(0).isKill();
  bool SrcIsKill = MI.getOperand(2).isKill();
  OpLo = AVR::STDPtrQRr;
  OpHi = AVR::STDPtrQRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);

  assert(Imm < 63 && "Offset is out of range");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstReg)
    . addImm(Imm)
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstReg, getKillRegState(DstIsKill))
    . addImm(Imm + 1)
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::INWRdA>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned Imm = MI.getOperand(1).getImm();
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  OpLo = AVR::INRdA;
  OpHi = AVR::INRdA;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  assert(Imm < 63 && "Address is out of range");

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addImm(Imm);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addImm(Imm + 1);

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::OUTWARr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned Imm = MI.getOperand(0).getImm();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  OpLo = AVR::OUTARr;
  OpHi = AVR::OUTARr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);

  assert(Imm < 63 && "Address is out of range");

  // 16 bit I/O writes need the high byte first
  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addImm(Imm + 1)
    . addReg(SrcHiReg, getKillRegState(SrcIsKill));

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addImm(Imm)
    . addReg(SrcLoReg, getKillRegState(SrcIsKill));

  MIBLO->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());
  MIBHI->setMemRefs(MI.memoperands_begin(), MI.memoperands_end());

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::PUSHWRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, SrcLoReg, SrcHiReg;
  unsigned SrcReg = MI.getOperand(0).getReg();
  bool SrcIsKill = MI.getOperand(0).isKill();
  unsigned Flags = MI.getFlags();
  OpLo = AVR::PUSHRr;
  OpHi = AVR::PUSHRr;
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(SrcLoReg, getKillRegState(SrcIsKill))
    . setMIFlags(Flags);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(SrcHiReg, getKillRegState(SrcIsKill))
    . setMIFlags(Flags);

  UNUSED_VARIABLE(MIBLO);
  UNUSED_VARIABLE(MIBHI);

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::POPWRd>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned Flags = MI.getFlags();
  OpLo = AVR::POPRd;
  OpHi = AVR::POPRd;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBHI = buildMI(MBB, MBBI, OpHi, DstHiReg)
    . setMIFlags(Flags);

  auto MIBLO = buildMI(MBB, MBBI, OpLo, DstLoReg)
    . setMIFlags(Flags);

  UNUSED_VARIABLE(MIBLO);
  UNUSED_VARIABLE(MIBHI);

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::LSLWRd>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = AVR::LSLRd;
  OpHi = AVR::ROLRd;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill));

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill));

  UNUSED_VARIABLE(MIBLO);

  if (ImpIsDead) MIBHI->getOperand(2).setIsDead();
  // SREG is always implicitly killed
  MIBHI->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::LSRWRd>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = AVR::RORRd;
  OpHi = AVR::LSRRd;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill));

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill));

  UNUSED_VARIABLE(MIBHI);

  if (ImpIsDead) MIBLO->getOperand(2).setIsDead();
  // SREG is always implicitly killed
  MIBLO->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::RORWRd>(Block & MBB, BlockIt MBBI) {
  assert(0 && "RORW unimplemented");
}

template <>
bool
AVRExpandPseudo::expand<AVR::ROLWRd>(Block & MBB, BlockIt MBBI) {
  assert(0 && "ROLW unimplemented");
}

template <>
bool
AVRExpandPseudo::expand<AVR::ASRWRd>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool DstIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  OpLo = AVR::RORRd;
  OpHi = AVR::ASRRd;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, getKillRegState(DstIsKill));

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstLoReg, getKillRegState(DstIsKill));

  UNUSED_VARIABLE(MIBHI);

  if (ImpIsDead) MIBLO->getOperand(2).setIsDead();
  // SREG is always implicitly killed
  MIBLO->getOperand(3).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::SEXT>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned DstLoReg, DstHiReg;
  /*
     sext R17:R16, R17
     mov     r16, r17
     lsl     r17
     sbc     r17, r17
     sext R17:R16, R13
     mov     r16, r13
     mov     r17, r13
     lsl     r17
     sbc     r17, r17
     sext R17:R16, R16
     mov     r17, r16
     lsl     r17
     sbc     r17, r17
   */
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  splitRegs(DstReg, DstLoReg, DstHiReg);

  if (SrcReg != DstLoReg) {
    auto MOV = buildMI(MBB, MBBI, AVR::MOVRdRr)
      . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      . addReg(SrcReg);
    if (SrcReg == DstHiReg) MOV->getOperand(1).setIsKill();
  }

  if (SrcReg != DstHiReg) {
    buildMI(MBB, MBBI, AVR::MOVRdRr)
      . addReg(DstHiReg, RegState::Define)
      . addReg(SrcReg, getKillRegState(SrcIsKill));
  }

  buildMI(MBB, MBBI, AVR::LSLRd)
    . addReg(DstHiReg, RegState::Define)
    . addReg(DstHiReg, RegState::Kill);

  auto SBC = buildMI(MBB, MBBI, AVR::SBCRdRr)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, RegState::Kill)
    . addReg(DstHiReg, RegState::Kill);

  if (ImpIsDead) SBC->getOperand(3).setIsDead();
  // SREG is always implicitly killed
  SBC->getOperand(4).setIsKill();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::ZEXT>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned DstLoReg, DstHiReg;
  /*
     zext R25:R24, R20
     mov      R24, R20
     eor      R25, R25
     zext R25:R24, R24
     eor      R25, R25
     zext R25:R24, R25
     mov      R24, R25
     eor      R25, R25
   */
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool SrcIsKill = MI.getOperand(1).isKill();
  bool ImpIsDead = MI.getOperand(2).isDead();
  splitRegs(DstReg, DstLoReg, DstHiReg);

  if (SrcReg != DstLoReg) {
    buildMI(MBB, MBBI, AVR::MOVRdRr)
      . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
      . addReg(SrcReg, getKillRegState(SrcIsKill));
  }

  auto EOR = buildMI(MBB, MBBI, AVR::EORRdRr)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, RegState::Kill)
    . addReg(DstHiReg, RegState::Kill);

  if (ImpIsDead) EOR->getOperand(3).setIsDead();

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::SPREAD>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned OpLo, OpHi, DstLoReg, DstHiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  unsigned Flags = MI.getFlags();
  OpLo = AVR::INRdA;
  OpHi = AVR::INRdA;
  splitRegs(DstReg, DstLoReg, DstHiReg);

  auto MIBLO = buildMI(MBB, MBBI, OpLo)
    . addReg(DstLoReg, RegState::Define | getDeadRegState(DstIsDead))
    . addImm(0x3d)
    . setMIFlags(Flags);

  auto MIBHI = buildMI(MBB, MBBI, OpHi)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addImm(0x3e)
    . setMIFlags(Flags);

  UNUSED_VARIABLE(MIBLO);
  UNUSED_VARIABLE(MIBHI);

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::SPWRITE>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned SrcLoReg, SrcHiReg;
  unsigned SrcReg = MI.getOperand(1).getReg();
  bool SrcIsKill = MI.getOperand(1).isKill();
  unsigned Flags = MI.getFlags();
  splitRegs(SrcReg, SrcLoReg, SrcHiReg);

  buildMI(MBB, MBBI, AVR::INRdA)
    . addReg(AVR::R0, RegState::Define)
    . addImm(0x3f)
    . setMIFlags(Flags);
  buildMI(MBB, MBBI, AVR::BCLRs)
    . addImm(0x07)
    . setMIFlags(Flags);
  buildMI(MBB, MBBI, AVR::OUTARr)
    . addImm(0x3e)
    . addReg(SrcHiReg, getKillRegState(SrcIsKill))
    . setMIFlags(Flags);
  buildMI(MBB, MBBI, AVR::OUTARr)
    . addImm(0x3f)
    . addReg(AVR::R0, RegState::Kill)
    . setMIFlags(Flags);
  buildMI(MBB, MBBI, AVR::OUTARr)
    . addImm(0x3d)
    . addReg(SrcLoReg, getKillRegState(SrcIsKill))
    . setMIFlags(Flags);

  MI.eraseFromParent();
  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::MULRdRrP>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  // mul r25, r24
  // mov rdest, r0
  // clr r1
  unsigned DstReg = MI.getOperand(0).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();

  MI.setDesc(TII->get(AVR::MULRdRr));
  MI.getOperand(2).setIsDead();

  buildMI(MBB, std::next(MBBI), AVR::MOVRdRr)
    . addReg(DstReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(AVR::R0, RegState::Kill);

  //:TODO: clr r1

  return true;
}

template <>
bool
AVRExpandPseudo::expand<AVR::MULWRdRr>(Block & MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  unsigned DstLoReg, DstHiReg;
  /*
     %R25R24 = MULWRdRr %R23R22, %R19R18<kill>
     mul r22,r18
     movw r24,r0
     mul r22,r19
     add r25,r0
     mul r23,r18
     add r25,r0
     clr r1
   */
  unsigned Src1LoReg, Src1HiReg, Src2LoReg, Src2HiReg;
  unsigned DstReg = MI.getOperand(0).getReg();
  unsigned Src1Reg = MI.getOperand(1).getReg();
  unsigned Src2Reg = MI.getOperand(2).getReg();
  bool DstIsDead = MI.getOperand(0).isDead();
  bool Src1IsKill = MI.getOperand(1).isKill();
  bool Src2IsKill = MI.getOperand(2).isKill();
  bool ImpIsDead = MI.getOperand(5).isDead();
  MachineInstrBuilder MIB;
  splitRegs(Src1Reg, Src1LoReg, Src1HiReg);
  splitRegs(Src2Reg, Src2LoReg, Src2HiReg);
  splitRegs(DstReg, DstLoReg, DstHiReg);

  MIB = buildMI(MBB, MBBI, AVR::MULRdRr)
    . addReg(Src1LoReg)
    . addReg(Src2LoReg);
  MIB->getOperand(3).setIsDead();

  buildMI(MBB, MBBI, AVR::MOVWRdRr)
    . addReg(DstReg, RegState::Define)
    . addReg(AVR::R1R0, RegState::Kill);

  MIB = buildMI(MBB, MBBI, AVR::MULRdRr)
    . addReg(Src1LoReg, getKillRegState(Src1IsKill))
    . addReg(Src2HiReg, getKillRegState(Src2IsKill));
  MIB->getOperand(2).setIsDead();
  MIB->getOperand(4).setIsDead();

  MIB = buildMI(MBB, MBBI, AVR::ADDRdRr)
    . addReg(DstHiReg, RegState::Define)
    . addReg(DstHiReg, RegState::Kill)
    . addReg(AVR::R0, RegState::Kill);
  MIB->getOperand(3).setIsDead();

  MIB = buildMI(MBB, MBBI, AVR::MULRdRr)
    . addReg(Src1HiReg, getKillRegState(Src1IsKill))
    . addReg(Src2LoReg, getKillRegState(Src2IsKill));
  MIB->getOperand(2).setIsDead();
  MIB->getOperand(4).setIsDead();

  MIB = buildMI(MBB, MBBI, AVR::ADDRdRr)
    . addReg(DstHiReg, RegState::Define | getDeadRegState(DstIsDead))
    . addReg(DstHiReg, RegState::Kill)
    . addReg(AVR::R0, RegState::Kill);

  if (ImpIsDead) MIB->getOperand(3).setIsDead();

  //:TODO: clr r1
  MI.eraseFromParent();
  return true;
}

bool
AVRExpandPseudo::expandMI(Block &MBB, BlockIt MBBI) {
  MachineInstr & MI = *MBBI;
  int Opcode = MBBI->getOpcode();
#define EXPAND(Op) case Op: return expand<Op>(MBB, MI)
  switch (Opcode) {
    EXPAND(AVR::ADDWRdRr);
    EXPAND(AVR::ADCWRdRr);
    EXPAND(AVR::SUBWRdRr);
    EXPAND(AVR::SUBIWRdK);
    EXPAND(AVR::SBCWRdRr);
    EXPAND(AVR::SBCIWRdK);
    EXPAND(AVR::ANDWRdRr);
    EXPAND(AVR::ANDIWRdK);
    EXPAND(AVR::ORWRdRr);
    EXPAND(AVR::ORIWRdK);
    EXPAND(AVR::EORWRdRr);
    EXPAND(AVR::COMWRd);
    EXPAND(AVR::CPWRdRr);
    EXPAND(AVR::CPCWRdRr);
    EXPAND(AVR::LDIWRdK);
    EXPAND(AVR::LDSWRdK);
    EXPAND(AVR::LDWRdPtr);
    EXPAND(AVR::LDWRdPtrPi);
    EXPAND(AVR::LDWRdPtrPd);
    case   AVR::LDDWRdYQ:     //:FIXME: remove this once PR13375 gets fixed
    EXPAND(AVR::LDDWRdPtrQ);
    EXPAND(AVR::STSWKRr);
    EXPAND(AVR::STWPtrRr);
    EXPAND(AVR::STWPtrPiRr);
    EXPAND(AVR::STWPtrPdRr);
    EXPAND(AVR::STDWPtrQRr);
    EXPAND(AVR::INWRdA);
    EXPAND(AVR::OUTWARr);
    EXPAND(AVR::PUSHWRr);
    EXPAND(AVR::POPWRd);
    EXPAND(AVR::LSLWRd);
    EXPAND(AVR::LSRWRd);
    EXPAND(AVR::RORWRd);
    EXPAND(AVR::ROLWRd);
    EXPAND(AVR::ASRWRd);
    EXPAND(AVR::SEXT);
    EXPAND(AVR::ZEXT);
    EXPAND(AVR::SPREAD);
    EXPAND(AVR::SPWRITE);
    EXPAND(AVR::MULRdRrP);
    EXPAND(AVR::MULWRdRr);
  }
#undef EXPAND
  return false;
}

FunctionPass * createAVRExpandPseudoPass() { return new AVRExpandPseudo(); }

} // end of namespace llvm

