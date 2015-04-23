//===---- AVRAsmParser.cpp - Parse AVR assembly to MCInst instructions ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AVR.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"
#include "AVRRegisterInfo.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCTargetAsmParser.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "avr-asm-parser"


namespace {
class AVRAsmParser : public MCTargetAsmParser {
  MCSubtargetInfo &STI;
  MCAsmParser &Parser;


#define GET_ASSEMBLER_HEADER
#include "AVRGenAsmMatcher.inc"

  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo, bool MatchingInlineAsm) 
                               override;

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;

  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc,
                        OperandVector &Operands) override;
  
  bool ParseDirective(AsmToken directiveID) override { return true; }

  //! \brief Parses identifiers as AsmToken::Token's when needed.
  //!
  //! Some instructions have hard coded values as operands.
  //! For example, the `lpm Rd, Z` instruction, where the second
  //! operand is always explicitly Z.
  //!
  //! The parser naively parses `Z` and all other identifiers as immediates.
  //! The problem with this is that the instruction matcher expects `Z` to
  //! be of kind AsmToken::Token.
  //!
  //! This function fixes this by maintaining a table of operand values

  //! (such as `Z`) and mnemonics (such as `lpm`) and parsing all operands
  //! that fit the criteria as AsmToken::Token values.
  //!
  //! \return `false` if we found a token that fit the criteria and
  //!         parsed it, `true` otherwise.
  bool ParseCustomOperand(OperandVector &Operands, StringRef Mnemonic);

  //! \brief Parses an assembly operand.
  //! \param Operands A list to add the successfully parsed operand to.
  //! \param Mnemonic The mnemonic of the instruction.
  //! \return `false` if parsing succeeds, `true` otherwise.
  bool ParseOperand(OperandVector &Operands, StringRef Mnemonic);

  //! \brief Attempts to parse a register.
  //! \return The register number, or `-1` if the token is not a register.
  int tryParseRegister(StringRef Mnemonic);

  bool tryParseRegisterOperand(OperandVector &Operands,
                               StringRef Mnemonic);

  //! \brief Matches a register name to a register number.
  //! \return The register number, or -1 if the register is invalid.
  int matchRegisterName(StringRef Symbol);
  
public:
  AVRAsmParser(MCSubtargetInfo &sti, MCAsmParser &parser,
                const MCInstrInfo &MII, const MCTargetOptions &Options)
    : MCTargetAsmParser(), STI(sti), Parser(parser) {
    // Initialize the set of available features.
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

  MCAsmParser &getParser() const { return Parser; }
  MCAsmLexer &getLexer() const { return Parser.getLexer(); }

};
}

namespace {

/// AVROperand - Instances of this class represent a parsed AVR machine
/// instruction.
class AVROperand : public MCParsedAsmOperand {

  enum KindTy {
    k_Immediate,
    k_Register,
    k_Token
  } Kind;

public:
  AVROperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

  union {
    struct {
      const char *Data;
      unsigned Length;
    } Tok;

    struct {
      unsigned RegNum;
    } Reg;

    struct {
      const MCExpr *Val;
    } Imm;
  };

  SMLoc StartLoc, EndLoc;

public:
  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateReg(getReg()));
  }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const{
    // Add as immediate when possible.  Null MCExpr = 0.
    if (Expr == 0)
      Inst.addOperand(MCOperand::CreateImm(0));
    else if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr))
      Inst.addOperand(MCOperand::CreateImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::CreateExpr(Expr));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCExpr *Expr = getImm();
    addExpr(Inst,Expr);
  }

  bool isReg() const { return Kind == k_Register; }
  bool isImm() const { return Kind == k_Immediate; }
  bool isToken() const { return Kind == k_Token; }
  bool isMem() const { return false; }

  StringRef getToken() const {
    assert(Kind == k_Token && "Invalid access!");
    return StringRef(Tok.Data, Tok.Length);
  }

  unsigned getReg() const {
    assert((Kind == k_Register) && "Invalid access!");
    return Reg.RegNum;
  }

  const MCExpr *getImm() const {
    assert((Kind == k_Immediate) && "Invalid access!");
    return Imm.Val;
  }

  static std::unique_ptr<AVROperand> CreateToken(StringRef Str, SMLoc S) {
    auto Op = make_unique<AVROperand>(k_Token);
    Op->Tok.Data = Str.data();
    Op->Tok.Length = Str.size();
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  /// Internal constructor for register kinds
  static std::unique_ptr<AVROperand> CreateReg(unsigned RegNum, SMLoc S, 
                                                SMLoc E) {
    auto Op = make_unique<AVROperand>(k_Register);
    Op->Reg.RegNum = RegNum;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static std::unique_ptr<AVROperand> CreateImm(const MCExpr *Val, SMLoc S, SMLoc E) {
    auto Op = make_unique<AVROperand>(k_Immediate);
    Op->Imm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  /// getStartLoc - Get the location of the first token of this operand.
  SMLoc getStartLoc() const { return StartLoc; }
  /// getEndLoc - Get the location of the last token of this operand.
  SMLoc getEndLoc() const { return EndLoc; }

  virtual void print(raw_ostream &OS) const {
    OS << "AVROperand = ";
    
    switch(Kind) {
      case k_Token:
        OS << "Token(\"" << Tok.Data << "\")";
        break;
      case k_Register:
        OS << "Register(Num = " << Reg.RegNum << ")";
        break;
      case k_Immediate:
        OS << "Immediate(Expr = ";
        
        Imm.Val->print(OS);
        
        OS << ")";
        
        break;
      default:
        llvm_unreachable("unimplemented");
        break;
    }
    
    OS << "\n";
  }
};
}

bool AVRAsmParser::
MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                        OperandVector &Operands,
                        MCStreamer &Out, uint64_t &ErrorInfo,
                        bool MatchingInlineAsm) {
  MCInst Inst;
  unsigned MatchResult = MatchInstructionImpl(Operands, Inst, ErrorInfo,
                                              MatchingInlineAsm);
  switch (MatchResult) {
  default: break;
  case Match_Success: {
      Inst.setLoc(IDLoc);
      Out.EmitInstruction(Inst, STI);
      
    return false;
  }
  case Match_MissingFeature:
    Error(IDLoc, "instruction requires a CPU feature not currently enabled");
    return true;
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(IDLoc, "too few operands for instruction");

      ErrorLoc = ((AVROperand &)*Operands[ErrorInfo]).getStartLoc();
      if (ErrorLoc == SMLoc()) ErrorLoc = IDLoc;
    }

    return Error(ErrorLoc, "invalid operand for instruction");
  }
  case Match_MnemonicFail:
    return Error(IDLoc, "invalid instruction");
  }
  return true;
}

int AVRAsmParser::matchRegisterName(StringRef Name) {
   
   // Note that the register name is in lower-case.
   
   int CC;
    CC = StringSwitch<unsigned>(Name)
      .Case("r0",  AVR::R0)
      .Case("r1",  AVR::R1)
      .Case("r2",  AVR::R2)
      .Case("r3",  AVR::R3)
      .Case("r4",  AVR::R4)
      .Case("r5",  AVR::R5)
      .Case("r6",  AVR::R6)
      .Case("r7",  AVR::R7)
      .Case("r8",  AVR::R8)
      .Case("r9",  AVR::R9)
      .Case("r10", AVR::R10)
      .Case("r11", AVR::R11)
      .Case("r12", AVR::R12)
      .Case("r13", AVR::R13)
      .Case("r14", AVR::R14)
      .Case("r15", AVR::R15)
      .Case("r16", AVR::R16)
      .Case("r17", AVR::R17)
      .Case("r18", AVR::R18)
      .Case("r19", AVR::R19)
      .Case("r20", AVR::R20)
      .Case("r21", AVR::R21)
      .Case("r22", AVR::R22)
      .Case("r23", AVR::R23)
      .Case("r24", AVR::R24)
      .Case("r25", AVR::R25)
      .Case("r26", AVR::R26)
      .Case("r27", AVR::R27)
      .Case("r28", AVR::R28)
      .Case("r29", AVR::R29)
      .Case("r30", AVR::R30)
      .Case("r31", AVR::R31)
      .Case("spl", AVR::SPL)
      .Case("sph", AVR::SPH)
      .Case("sp",  AVR::SP)
      .Case("x",   AVR::R27R26)
      .Case("y",   AVR::R29R28)
      .Case("z",   AVR::R31R30)
      
      .Default(-1);

  if (CC != -1)
    return CC;

  return -1;
}

int AVRAsmParser::tryParseRegister(StringRef Mnemonic) {
  const AsmToken &Tok = Parser.getTok();
  int RegNum = -1;

  if (Tok.is(AsmToken::Identifier)) {
  
    std::string lowerCase = Tok.getString().lower();
    RegNum = matchRegisterName(lowerCase);
  } else { // not a register
      RegNum = -1;
  }
  
  return RegNum;
}

bool AVRAsmParser::
  tryParseRegisterOperand(OperandVector &Operands,
                          StringRef Mnemonic){

  SMLoc S = Parser.getTok().getLoc();
  int RegNo = -1;

  RegNo = tryParseRegister(Mnemonic);
  
  if (RegNo == -1)
    return true;

  Operands.push_back(AVROperand::CreateReg(RegNo, S,
                     Parser.getTok().getLoc()));
  
  Parser.Lex(); // Eat register token.
  return false;
}

/// \brief Maps operand names to tokens.
/// \note See ParseCustomOperand for details.
struct CustomOperandMapping {
  StringRef mnemonic;
  StringRef token;
};

bool AVRAsmParser::ParseCustomOperand(OperandVector &Operands,
                                      StringRef Mnemonic)
{
  // A list of (containing mnemonic, identifier) to
  // process as tokens.
  const CustomOperandMapping mappings[] = {
    CustomOperandMapping { "lpm",  "Z" },
    CustomOperandMapping { "lpmw", "Z" },
    CustomOperandMapping { "elpm", "Z" },
    CustomOperandMapping { "spm",  "Z" },

    CustomOperandMapping { "xch", "Z" },
    CustomOperandMapping { "las", "Z" },
    CustomOperandMapping { "lac", "Z" },
    CustomOperandMapping { "lat", "Z" }
  };


  // we only operate on identifiers
  assert(getLexer().is(AsmToken::Identifier));

  auto ident = getLexer().getTok().getIdentifier();

  for(auto &mapping : mappings) {

    // check if we found a match
    if( (mapping.mnemonic.compare_lower(Mnemonic) == 0) && (mapping.token.compare_lower(ident) == 0) ) {
      // add the token as an operand of type Token.
      SMLoc S = Parser.getTok().getLoc();
      Operands.push_back(AVROperand::CreateToken(mapping.token, S));

      Parser.Lex();

      return false;
    }
  }

  return true;
}


bool AVRAsmParser::ParseOperand(OperandVector &Operands,
                                StringRef Mnemonic) {
  DEBUG(dbgs() << "ParseOperand\n");

  SMLoc S = Parser.getTok().getLoc();
  SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
  const MCExpr *EVal;

  switch (getLexer().getKind()) {
    default:
      Error(S, "unexpected token in operand");
      return true;
    case AsmToken::Identifier:
    case AsmToken::LParen:
    case AsmToken::Integer:
    case AsmToken::String: {

      // check if the operand is an identifier
      if(getLexer().is(AsmToken::Identifier)) {

        // check if we need to parse this operand as
        // as token.
        if(!ParseCustomOperand(Operands, Mnemonic)) {
          return false;
        }
      }

      // try parse the operand as a register
      if(!tryParseRegisterOperand(Operands, Mnemonic)) {
        return false;
      
      } else { // the operand not a register

        if (getParser().parseExpression(EVal))
          return true;

        SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
        Operands.push_back(AVROperand::CreateImm(EVal, S, E));
        return false;
      }
    }
    // Parse the syntax .[+][-]offset
    // for PC-relative call.
    case AsmToken::Dot: {
      Parser.Lex(); // eat `.`
      if(!Parser.parseExpression(EVal, E)) {
        Operands.push_back(AVROperand::CreateImm(EVal, S, E));
        return false;
      }
    }
    case AsmToken::Plus:
    case AsmToken::Minus: {
      auto nextTok = Parser.getLexer().peekTok(true);

      // we are parsing an integer immediate
      if(nextTok.getKind() == AsmToken::Integer) {

        // try and parse the expression
        if(!Parser.parseExpression(EVal, E)) {
          Operands.push_back(AVROperand::CreateImm(EVal, S, E));
          return false;

        } else { // could not parse expression
          return true;
        }

      } else { // we should parse the '+' or '-' as a token

        Operands.push_back(AVROperand::CreateToken(getLexer().getTok().getString(), S));
        Parser.Lex();
        return false;
      }
    }

  } // switch(getLexer().getKind())
  
  // could not parse operand
  return true;
}

bool AVRAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                  SMLoc &EndLoc) {

  StartLoc = Parser.getTok().getLoc();
  RegNo = tryParseRegister("");
  EndLoc = Parser.getTok().getLoc();
  return (RegNo == (unsigned)-1);
}

bool AVRAsmParser::
ParseInstruction(ParseInstructionInfo &Info, StringRef Name, SMLoc NameLoc,
                 OperandVector &Operands) {

  // Create the leading tokens for the mnemonic, split by '.' characters.
  size_t Start = 0, Next = Name.find('.');
  StringRef Mnemonic = Name.slice(Start, Next);

  Operands.push_back(AVROperand::CreateToken(Mnemonic, NameLoc));

  // Read the remaining operands.
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    // Read the first operand.
    if (ParseOperand(Operands, Name)) {
      SMLoc Loc = getLexer().getLoc();
      Parser.eatToEndOfStatement();
      return Error(Loc, "unexpected token in argument list");
    }

    while (getLexer().isNot(AsmToken::EndOfStatement)) {

      if(getLexer().is(AsmToken::Comma))
        Parser.Lex();  // Eat the comma.

      // Parse and remember the operand.
      if (ParseOperand(Operands, Name)) {
        SMLoc Loc = getLexer().getLoc();
        Parser.eatToEndOfStatement();
        return Error(Loc, "unexpected token in argument list");
      }
    }
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    Parser.eatToEndOfStatement();
    return Error(Loc, "unexpected token in argument list");
  }

  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

extern "C" void LLVMInitializeAVRAsmParser() {
  RegisterMCAsmParser<AVRAsmParser> X(TheAVRTarget);
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "AVRGenAsmMatcher.inc"

