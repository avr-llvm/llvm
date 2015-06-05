//===-- AVRMCCodeEmitter.cpp - Convert AVR Code to Machine Code ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the AVRMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//
//

#include "AVRMCCodeEmitter.h"
#include "MCTargetDesc/AVRFixupKinds.h"
#include "MCTargetDesc/AVRMCExpr.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "mccodeemitter"

#define GET_INSTRMAP_INFO
#include "AVRGenInstrInfo.inc"
#undef GET_INSTRMAP_INFO

MCCodeEmitter *llvm::createAVRMCCodeEmitter(const MCInstrInfo &MCII,
                                         const MCRegisterInfo &MRI,
                                         MCContext &Ctx) {
  return new AVRMCCodeEmitter(MCII, Ctx);
}

void AVRMCCodeEmitter::emitByte(unsigned char C, raw_ostream &OS) const {
  OS << (char)C;
}

void AVRMCCodeEmitter::emitWord(uint16_t word, raw_ostream &OS) const {
  emitByte((word & 0x00ff)>>0, OS);
  emitByte((word & 0xff00)>>8, OS);
}

void AVRMCCodeEmitter::emitWords(uint16_t* words, size_t count, raw_ostream &OS) const {
  for(int64_t i=count-1; i>=0; --i) {
    uint16_t word = words[i];

    emitWord(word, OS);
  }
}

void AVRMCCodeEmitter::emitInstruction(uint64_t Val, unsigned Size,
                                        const MCSubtargetInfo &STI,
                                        raw_ostream &OS) const {
  uint16_t* words = (uint16_t*) &Val;
  size_t wordCount = Size/2;

  emitWords(words, wordCount, OS);
}

void AVRMCCodeEmitter::
encodeInstruction(const MCInst &MI, raw_ostream &OS,
                  SmallVectorImpl<MCFixup> &Fixups,
                  const MCSubtargetInfo &STI) const
{
  uint32_t Binary = getBinaryCodeForInstr(MI, Fixups, STI);
  
  const MCInstrDesc &Desc = MCII.get(MI.getOpcode());
  
  // Get byte count of instruction
  unsigned Size = Desc.getSize();
  if (!Size)
    llvm_unreachable("Desc.getSize() returns 0");

  emitInstruction(Binary, Size, STI, OS);
}

// The encoding of the LD/ST family of instructions is inconsistent w.r.t
// the pointer register and the addressing mode.
//
// The permutations of the format are as followed:
// ld Rd, X    `1001 000d dddd 1100`
// ld Rd, X+   `1001 000d dddd 1101`
// ld Rd, -X   `1001 000d dddd 1110`
//
// ld Rd, Y    `1000 000d dddd 1000`
// ld Rd, Y+   `1001 000d dddd 1001`
// ld Rd, -Y   `1001 000d dddd 1010`

// ld Rd, Z    `1000 000d dddd 0000`
// ld Rd, Z+   `1001 000d dddd 0001`
// ld Rd, -Z   `1001 000d dddd 0010`
//                 ^
//                 |
// Note this one inconsistent bit - it is 1 sometimes and 0 at other times.
// There is no logical pattern. Looking at a truth table, the following
// formula can be derived to fit the pattern:
// 
// inconsistent_bit = is_predec OR is_postinc OR is_reg_x
//
// We manually set this bit in the post encoder method.
unsigned AVRMCCodeEmitter::
loadStorePostEncoder(const MCInst &MI,
                     unsigned EncodedValue,
                     const MCSubtargetInfo &STI) const {

  assert(MI.getOperand(0).isReg() && MI.getOperand(1).isReg() && "the load/store operands must be registers");

  auto opcode = MI.getOpcode();

  // check whether either of the registers are the X pointer register.
  bool isRegX = (MI.getOperand(0).getReg() == AVR::R27R26) ||
                (MI.getOperand(1).getReg() == AVR::R27R26);

  bool isPredec = opcode == AVR::LDRdPtrPd ||
                  opcode == AVR::STPtrPdRr;

  bool isPostinc = opcode == AVR::LDRdPtrPi ||
                   opcode == AVR::STPtrPiRr;


  // check if we need to set the inconsistent bit
  if(isRegX || isPredec || isPostinc) {
    EncodedValue |= (1<<12);
  }

  return EncodedValue;
}

unsigned
AVRMCCodeEmitter::getRelCondBrTargetEncoding(unsigned size,
                                          const MCInst &MI, unsigned OpNo,
                                          SmallVectorImpl<MCFixup> &Fixups,
                                          const MCSubtargetInfo &STI) const {
                                   
  const MCOperand MO = MI.getOperand(OpNo);
  
  if (MO.isExpr())
  {
    const MCOperand &MO = MI.getOperand(OpNo);

    AVR::Fixups Kind = AVR::Fixups(0);
    
    switch(size)
    {
        case 7:
        {
          Kind = AVR::fixup_7_pcrel;
          break;
        }
        case 13:
        {
          Kind = AVR::fixup_13_pcrel;
          break;
        }
        default:
        {
          llvm_unreachable("unknown size");
          break;
        }
    }

    const MCExpr *Expr = MO.getExpr();
    
    Fixups.push_back(MCFixup::create(0, Expr, MCFixupKind(Kind), MI.getLoc()));
    
    // All of the information is in the fixup.
    return 0;
  } 
  else {
    // take the size of the current instruction away.
    // with labels, this is implicitly handled.
    auto target = MO.getImm();

    //if(target < 0) target -= 2;

    return AVR::fixups::adjustRelativeBranchTarget(target);
  }
}

unsigned
AVRMCCodeEmitter::getLDSTPtrRegEncoding(const MCInst &MI, unsigned OpNo,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
    
    // the operand should be a pointer register.
    assert(MI.getOperand(OpNo).isReg());
  
    auto MO = MI.getOperand(OpNo);
    
    unsigned encoding;
    
    switch (MO.getReg()) {
        case AVR::R27R26: // X pointer register
        {
            encoding = 0x03; // 0b11
            break;
        }
        case AVR::R29R28: // Y pointer register
        {
            encoding = 0x02; // 0b10
            break;
        }
        case AVR::R31R30: // Z pointer register
        {
            encoding = 0x00; // 0b00
            break;
        }
        default:
        {
            llvm_unreachable("invalid pointer register");
            break;
        }
    }

    return encoding;
}


unsigned
AVRMCCodeEmitter::getI8ImmComEncoding(const MCInst &MI, unsigned OpNo,
                                      SmallVectorImpl<MCFixup> &Fixups,
                                      const MCSubtargetInfo &STI) const {
  // the operand should be a pointer register.
  assert(MI.getOperand(OpNo).isImm());

  auto immediateValue = MI.getOperand(OpNo).getImm();

  auto compliment = 0xff - immediateValue;

  return compliment;
}

unsigned
AVRMCCodeEmitter::getCallTargetEncoding(const MCInst &MI, unsigned OpNo,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
  auto MO = MI.getOperand(OpNo);
  
  if (MO.isExpr())
  {
    Fixups.push_back(MCFixup::create(0, MO.getExpr(), MCFixupKind(AVR::fixup_call), MI.getLoc()));
    
    return 0;
  } 
  else {
    return AVR::fixups::adjustBranchTarget(MO.getImm());
  }
}


unsigned AVRMCCodeEmitter::
getExprOpValue(const MCExpr *Expr,SmallVectorImpl<MCFixup> &Fixups,
               const MCSubtargetInfo &STI) const {
               
  MCExpr::ExprKind Kind = Expr->getKind();

  if (Kind == MCExpr::Binary) {
    Expr = static_cast<const MCBinaryExpr*>(Expr)->getLHS();
    Kind = Expr->getKind();
  }
  
  if(Kind == MCExpr::Target) {
    const AVRMCExpr *AVRExpr = cast<AVRMCExpr>(Expr);

    AVR::Fixups FixupKind = AVR::Fixups(0);
    
    switch (AVRExpr->getKind()) {
      // FIXME: uncomment once the fixup types are implemented
      
      case AVRMCExpr::VK_AVR_LO8: {
        FixupKind = AVR::fixup_8_lo8;
        break;
      }
      case AVRMCExpr::VK_AVR_HI8: {
        FixupKind = AVR::fixup_8_hi8;
        break;
      }
      default: {
        llvm_unreachable("Unsupported fixup kind for target expression!");
      }
    }
    
    Fixups.push_back(MCFixup::create(0, AVRExpr, MCFixupKind(FixupKind)));
    return 0;
  }

  assert (Kind == MCExpr::SymbolRef);

  // All of the information is in the fixup.
  return 0;
}

unsigned AVRMCCodeEmitter::
getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                  SmallVectorImpl<MCFixup> &Fixups,
                  const MCSubtargetInfo &STI) const {
  if (MO.isReg()) {
    unsigned Reg = MO.getReg();
    unsigned RegNo = Ctx.getRegisterInfo()->getEncodingValue(Reg);
    return RegNo;
  } else if (MO.isImm()) {
    return static_cast<unsigned>(MO.getImm());
  } else if (MO.isFPImm()) {
    return static_cast<unsigned>(APFloat(MO.getFPImm())
        .bitcastToAPInt().getHiBits(32).getLimitedValue());
  }
  
  // MO must be an Expr.
  assert(MO.isExpr());
  
  return getExprOpValue(MO.getExpr(),Fixups, STI);
}

#include "AVRGenMCCodeEmitter.inc"
