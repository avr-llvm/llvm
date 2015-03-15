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
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "mccodeemitter"

#define GET_INSTRMAP_INFO
#include "AVRGenInstrInfo.inc"
#undef GET_INSTRMAP_INFO

MCCodeEmitter *llvm::createAVRMCCodeEmitter(const MCInstrInfo &MCII,
                                         const MCRegisterInfo &MRI,
                                         const MCSubtargetInfo &STI,
                                         MCContext &Ctx) {
  return new AVRMCCodeEmitter(MCII, Ctx, false);
}

void AVRMCCodeEmitter::EmitByte(unsigned char C, raw_ostream &OS) const {
  OS << (char)C;
}

void AVRMCCodeEmitter::EmitInstruction(uint64_t Val, unsigned Size,
                                        const MCSubtargetInfo &STI,
                                        raw_ostream &OS) const {
  for (unsigned i = 0; i < Size; ++i) {
    unsigned Shift = !IsLittleEndian ? i * 8 : (Size - 1 - i) * 8;
    EmitByte((Val >> Shift) & 0xff, OS);
  }
}

/// EncodeInstruction - Emit the instruction.
/// Size the instruction with Desc.getSize().
void AVRMCCodeEmitter::
EncodeInstruction(const MCInst &MI, raw_ostream &OS,
                  SmallVectorImpl<MCFixup> &Fixups,
                  const MCSubtargetInfo &STI) const
{
  uint32_t Binary = getBinaryCodeForInstr(MI, Fixups, STI);
  
  const MCInstrDesc &Desc = MCII.get(MI.getOpcode());
  
  // Get byte count of instruction
  unsigned Size = Desc.getSize();
  if (!Size)
    llvm_unreachable("Desc.getSize() returns 0");

  EmitInstruction(Binary, Size, STI, OS);
}

/// getMemriEncoding - Return binary encoding of a pointer register plus displacement operand.
/// If the offset operand requires relocation, record the relocation.
unsigned
AVRMCCodeEmitter::getMemriEncoding(const MCInst &MI, unsigned OpNo,
                                   SmallVectorImpl<MCFixup> &Fixups,
                                   const MCSubtargetInfo &STI) const {
                                   
  // the first operand should be the pointer register.
  assert(MI.getOperand(OpNo).isReg());
  
  // the second operand should be the displacement as an immediate value.
  assert(MI.getOperand(OpNo+1).isImm());
  
  unsigned encoding = 0;
  
  uint8_t reg_bit = 0; // register bit.
  uint8_t disp_bits = MI.getOperand(OpNo+1).getImm(); // displacement bits (6 bits).
  
  switch(MI.getOperand(OpNo).getReg())
  {
      case AVR::R29R28: // Y pointer register
      {
          reg_bit = 1;
          
          break;
      }
      case AVR::R31R30: // Z pointer register
      {
          reg_bit = 0;
          
          break;
      }
      case AVR::R27R26: // X pointer register (not supported).
      {
          llvm_unreachable("cannot encode the X pointer register for this encoding");
          
          break;
      }
      default: // Some other register we don't support.
      {
          llvm_unreachable("register not supported for this operand encoding");
          
          break;
      }
  }
  
  encoding = (disp_bits<<1) | reg_bit;

  return encoding;
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
    
    Fixups.push_back(MCFixup::Create(0, Expr, MCFixupKind(Kind), MI.getLoc()));
    
    // All of the information is in the fixup.
    return 0;
  } 
  else {
    return (MO.getImm() - 2) >> 1;
  }
}

unsigned
AVRMCCodeEmitter::getLDSTPtrRegEncoding(const MCInst &MI, unsigned OpNo,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        const MCSubtargetInfo &STI) const {
    
    // the operand should be a pointer register.
    assert(MI.getOperand(OpNo).isReg());
  
    const MCOperand MO = MI.getOperand(OpNo);
    
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
    
    bool is_predec = OpNo == AVR::LDRdPtrPd ||
                     OpNo == AVR::STPtrPdRr;
    
    bool is_postinc = OpNo == AVR::LDRdPtrPi ||
                      OpNo == AVR::STPtrPiRr;
    
    bool is_reg_x = MO.getReg() == AVR::R27R26;
    
    if(is_predec || is_postinc || is_reg_x)
        encoding |= 0x04; // 0b100
    
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
    
    Fixups.push_back(MCFixup::Create(0, AVRExpr, MCFixupKind(FixupKind)));
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
