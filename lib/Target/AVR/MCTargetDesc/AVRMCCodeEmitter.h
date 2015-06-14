//===-- AVRMCCodeEmitter.h - Convert AVR Code to Machine Code -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the AVRMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//
//

#ifndef LLVM_AVR_CODE_EMITTER_H
# define LLVM_AVR_CODE_EMITTER_H

# include "AVRConfig.h"

# include "llvm/MC/MCCodeEmitter.h"
# include "llvm/Support/DataTypes.h"

namespace llvm {

class MCContext;
class MCExpr;
class MCInst;
class MCInstrInfo;
class MCFixup;
class MCOperand;
class MCSubtargetInfo;
class raw_ostream;

/**
 * Writes AVR machine code to a stream.
 */
class AVRMCCodeEmitter : public MCCodeEmitter {

public:
  AVRMCCodeEmitter(const MCInstrInfo &mcii, MCContext &Ctx_)
      : MCII(mcii), Ctx(Ctx_) {}

  void emitByte(unsigned char C, raw_ostream &OS) const;

  /**
   * Writes a single word to a stream.
   */
  void emitWord(uint16_t word, raw_ostream &OS) const;

  /**
   * Writes a number of words to the stream.
   */
  void emitWords(uint16_t *words, size_t count, raw_ostream &OS) const;

  void emitInstruction(uint64_t Val, unsigned Size, const MCSubtargetInfo &STI,
                       raw_ostream &OS) const;

  void encodeInstruction(const MCInst &MI, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;

  /**
   * Finishes up encoding an LD/ST instruction.
   *
   * The purpose of this function is to set an bit in the instruction
   * which follows no logical pattern. See the implementation for details.
   */
  unsigned loadStorePostEncoder(const MCInst &MI,
                                unsigned EncodedValue,
                                const MCSubtargetInfo &STI) const;

  unsigned getRelCondBrTargetEncoding(unsigned size,
                                   const MCInst &MI, unsigned OpNo,
                                   SmallVectorImpl<MCFixup> &Fixups,
                                   const MCSubtargetInfo &STI) const;
  
  /*!
   * Gets the encoding for a break target.
   */
  unsigned getRelCondBr7TargetEncoding(const MCInst &MI, unsigned OpNo,
                                    SmallVectorImpl<MCFixup> &Fixups,
                                    const MCSubtargetInfo &STI) const
  {
      return getRelCondBrTargetEncoding(7, MI, OpNo, Fixups, STI);
  }
  
  unsigned getRelCondBr13TargetEncoding(const MCInst &MI, unsigned OpNo,
                                    SmallVectorImpl<MCFixup> &Fixups,
                                    const MCSubtargetInfo &STI) const
  {
      return getRelCondBrTargetEncoding(13, MI, OpNo, Fixups, STI);
  }
  
  unsigned getLDSTPtrRegEncoding(const MCInst &MI, unsigned OpNo,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  unsigned getI8ImmComEncoding(const MCInst &MI, unsigned OpNo,
                               SmallVectorImpl<MCFixup> &Fixups,
                               const MCSubtargetInfo &STI) const;

  /*!
   * Gets the encoding of the target for the `CALL k` instruction.
   */
  unsigned getCallTargetEncoding(const MCInst &MI, unsigned OpNo,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;

  /*!
   *  TableGen'erated function for getting the binary encoding for an instruction.
   */
  uint64_t getBinaryCodeForInstr(const MCInst &MI,
                                 SmallVectorImpl<MCFixup> &Fixups,
                                 const MCSubtargetInfo &STI) const;
  
  unsigned getExprOpValue(const MCExpr *Expr, SmallVectorImpl<MCFixup> &Fixups,
                          const MCSubtargetInfo &STI) const;
  
  /*!
   * Returns the  binary encoding of operand.
   *
   * If the machine operand requires relocation, the relocation is recorded
   * and zero is returned.
   */
  unsigned getMachineOpValue(const MCInst &MI, const MCOperand &MO,
                             SmallVectorImpl<MCFixup> &Fixups,
                             const MCSubtargetInfo &STI) const;

private:

  AVRMCCodeEmitter(const AVRMCCodeEmitter &) = delete;
  void operator=(const AVRMCCodeEmitter &) = delete;

  const MCInstrInfo &MCII;
  MCContext &Ctx;
}; // class AVRMCCodeEmitter

} // end namespace of llvm.

#endif // LLVM_AVR_CODE_EMITTER_H
