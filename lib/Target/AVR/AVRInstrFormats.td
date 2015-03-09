//===-- AVRInstrInfo.td - AVR Instruction Formats ----------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// AVR Instruction Format Definitions.
//
//===----------------------------------------------------------------------===//

// :TODO: set is isAsCheapAsAMove per format instead of per instruction??!

// A generic AVR instruction.
class AVRInst<dag outs, dag ins, string asmstr, list<dag> pattern> : Instruction
{
  let Namespace = "AVR";

  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = asmstr;
  let Pattern = pattern;
}

/// A 16-bit AVR instruction.
class AVRInst16<dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst<outs, ins, asmstr, pattern>
{
  field bits<16> Inst;
  
  let Size = 2;
}

/// a 32-bit AVR instruction.
class AVRInst32<dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst<outs, ins, asmstr, pattern>
{
  field bits<32> Inst;
  
  let Size = 4;
}

// A class for pseudo instructions.
// Psuedo instructions are not real AVR instructions. The DAG stores
// psuedo instructions which are replaced by real AVR instructions by
// AVRExpandPseudoInsts.cpp.
//
// For example, the ADDW (add wide, as in add 16 bit values) instruction is defined
// as a pseudo instruction. In AVRExpandPseudoInsts.cpp, the instruction is then replaced
// by two add instructions - one for each byte.
class Pseudo<dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  let Pattern = pattern;
  
  let isPseudo = 1;
}

//===----------------------------------------------------------------------===//
// Register / register instruction: <|opcode|ffrd|dddd|rrrr|>
// opcode = 4 bits.
// f = secondary opcode = 2 bits
// d = destination = 5 bits
// r = source = 5 bits
// (Accepts all registers)
//===----------------------------------------------------------------------===//
class FRdRr<bits<4> opcode, bits<2> f, dag outs, dag ins, string asmstr,
            list<dag> pattern> : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> rd;
  bits<5> rr;

  let Inst{15-12} = opcode;
  let Inst{11-10} = f;
  let Inst{9} = rr{4};
  let Inst{8-4} = rd;
  let Inst{3-0} = rr{3-0};
}

//===----------------------------------------------------------------------===//
// Instruction of the format `<mnemonic> Z, Rd`
// <|1001|001r|rrrr|0ttt>
//===----------------------------------------------------------------------===//
class FZRd<bits<3> t, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> rd;

  let Inst{15-12} = 0b1001;

  let Inst{11-9} = 0b001;
  let Inst{8} = rd{4};

  let Inst{7-4} = rd{3-0};

  let Inst{3} = 0;
  let Inst{2-0} = t;
}

//===----------------------------------------------------------------------===//
// Register / immediate8 instruction: <|opcode|KKKK|dddd|KKKK|>
// opcode = 4 bits.
// K = constant data = 8 bits
// d = destination = 4 bits
// (Only accepts r16-r31)
//===----------------------------------------------------------------------===//
class FRdK<bits<4> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> rd;              // accept 5 bits but only encode the lower 4
  bits<8> k;

  let Inst{15-12} = opcode;
  let Inst{11-8} = k{7-4};
  let Inst{7-4} = rd{3-0};
  let Inst{3-0} = k{3-0};
}

//===----------------------------------------------------------------------===//
// Register instruction: <|opcode|fffd|dddd|ffff|>
// opcode = 4 bits.
// f = secondary opcode = 7 bits
// d = destination = 5 bits
// (Accepts all registers)
//===----------------------------------------------------------------------===//
class FRd<bits<4> opcode, bits<7> f, dag outs, dag ins, string asmstr,
          list<dag> pattern> : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> d;

  let Inst{15-12} = opcode;
  let Inst{11-9} = f{6-4};
  let Inst{8-4} = d;
  let Inst{3-0} = f{3-0};
}

//===----------------------------------------------------------------------===//
// [STD/LDD] P+q, Rr special encoding: <|10q0|qqtr|rrrr|pqqq>
// t = type (1 for STD, 0 for LDD)
// q = displacement (6 bits)
// r = register (5 bits)
// p = pointer register (1 bit) [1 for Y, 0 for Z]
//===----------------------------------------------------------------------===//
// Caveat:
// This format is a dirty, dirty hack. In AVRInstrInfo.td, there is an operand
// type called `memri`, which describes a pointer register and an immediate.
// This operand has a custom encoding function in AVRMCCodeEmitter.cpp. Ideally
// we would expose the ptr register and the immediate as seperate operands,so
// that we can assign their bits here (and wouldn't need an encoder function).
// I couldn't find a way to do this without breaking pattern matching. Instead,
// they are unified into one encoding function, which outputs <qqqqqqp>. The only
// way I could get tablegen to create the correct instructions was to increase the
// size of `bits<6> q` to `bits<7> q` and to increase all indices by one (because
// all bits were off by one place). Thus, we ignore q{0}.
//===----------------------------------------------------------------------===//
class FPqRr<bit t, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bit p;
  bits<7> q;
  bits<5> r;

  let Inst{15-14} = 0b10;
  let Inst{13} = q{6};
  let Inst{12} = 0b0;
  
  let Inst{11-10} = q{5-4};
  let Inst{9} = t;
  let Inst{8} = r{4};
  
  let Inst{7-4} = r{3-0};
  
  let Inst{3} = p;
  let Inst{2-0} = q{3-1};
}

//===---------------------------------------------------------------------===//
// An ST/LD instruction.
// <|100p|00tr|rrrr|ppaa|>
// t = type (1 for store, 0 for load)
// a = regular/postinc/predec (reg = 0b00, postinc = 0b01, predec = 0b10)
// p = pointer register (and inconsistent bit).
// r = src/dst register
//===---------------------------------------------------------------------===//
// TODO: List encodings of LDD/STD instructions
// 
// The encoding of this format of instruction is inconsistent w.r.t
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
// To solve this issue, in AVRInstrInfo.td, a new operand type is added: LDSTPtrReg.
// In the LD/ST family of instructions, the pointer register occupies two bits.
// The LDSTPtrReg operand has a special encoding function which uses the lowest
// three bits - the bottom two being the pointer register, and the top bit
// being our inconsistent bit. The custom encoding function uses the aforementioned
// boolean expression and puts it in the third bit of its result, so we can use it here.
// Thus, the format of `ppp` is not 3 bits for the pointer register, but `ipp` where
// `i` is the inconsistent bit.
//===---------------------------------------------------------------------===//
class FSTLD<bit type, bits<2> mode, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<3> ptrreg;
  bits<5> reg;
  
  let Inst{15-13} = 0b100;
  let Inst{12} = ptrreg{2};
  
  let Inst{11-10} = 0b00;
  let Inst{9} = type;
  let Inst{8} = reg{4};
  
  let Inst{7-4} = reg{3-0};
  
  let Inst{3-2} = ptrreg{1-0};
  let Inst{1-0} = mode{1-0};
}

// A 'ST Ptr, Rr' or 'LD Rd, Ptr' instruction.
class FSTLDPtrReg<bit t, dag outs, dag ins, string asmstr, list<dag> pattern>
  : FSTLD<t, 0b00, outs, ins, asmstr, pattern> { }

//===---------------------------------------------------------------------===//
// A 'ST Ptr+, Rr' or 'LD Rd, Ptr+' instruction.
// Store with postincrement.
//===---------------------------------------------------------------------===//
class FSTLDPtrPiReg<bit t, dag outs, dag ins, string asmstr, list<dag> pattern>
  : FSTLD<t, 0b01, outs, ins, asmstr, pattern> { }

//===---------------------------------------------------------------------===//
// A 'ST -Ptr, Rr' or 'LD Rd, -Ptr' instruction.
// Store with predecrement.
//===---------------------------------------------------------------------===//
class FSTLDPtrPdReg<bit t, dag outs, dag ins, string asmstr, list<dag> pattern>
  : FSTLD<t, 0b10, outs, ins, asmstr, pattern> { }

//===--------------------------------------------------------------------------------------------===//
// Special format for the TST instruction.
// TST Rd
// `TST Rd` is semantically equivalent to `AND Rd, Rd` and shares the same binary representation.
// <|0010|00rd|dddd|rrrr> (where Rd = Rr)
//===--------------------------------------------------------------------------------------------===//
class FTST<dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> reg;
  
  let Inst{15-12} = 0b0010;
  
  let Inst{11-10} = 0b00;
  let Inst{9} = reg{4};
  let Inst{8} = reg{4};
  
  let Inst{7-4} = reg{3-0};
  let Inst{3-0} = reg{3-0};
}

//===---------------------------------------------------------------------===//
// Special format for the LPM/ELPM instructions
// [E]LPM Rd, Z[+]
// <|1001|000d|dddd|01ep>
// d = destination register
// e = is elpm
// p = is postincrement
//===---------------------------------------------------------------------===//
class FLPMX<bit e, bit p, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
   bits<5> reg;
   
   let Inst{15-12} = 0b1001;
   
   let Inst{11-9} = 0b000;
   let Inst{8} = reg{4};
   
   let Inst{7-4} = reg{3-0};

   let Inst{3-2} = 0b01;
   let Inst{1} = e;
   let Inst{0} = p;
}

//===----------------------------------------------------------------------===//
// MOVWRdRr special encoding: <|0000|0001|dddd|rrrr|>
// d = destination = 4 bits
// r = source = 4 bits
// (Only accepts even registers)
//===----------------------------------------------------------------------===//
class FMOVWRdRr<dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> d;
  bits<5> r;

  let Inst{15-8} = 0b00000001;
  let Inst{7-4} = d{3-0};
  let Inst{3-0} = r{3-0};
}

//===----------------------------------------------------------------------===//
// MULrr special encoding: <|1001|11rd|dddd|rrrr|>
// d = multiplicand = 5 bits
// r = multiplier = 5 bits
// (Accepts all registers)
//===----------------------------------------------------------------------===//
class Mu1RdRr<dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> d;
  bits<5> r;

  let Inst{15-10} = 0b100111;
  let Inst{9} = r{4};
  let Inst{8-4} = d;
  let Inst{3-0} = r{3-0};
}

//===----------------------------------------------------------------------===//
// MULSrr special encoding: <|0000|0010|dddd|rrrr|>
// d = multiplicand = 4 bits
// r = multiplier = 4 bits
// (Only accepts r16-r31)
//===----------------------------------------------------------------------===//
class Mu2RdRr<bit f, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> rd;              // accept 5 bits but only encode the lower 4
  bits<5> rr;              // accept 5 bits but only encode the lower 4

  let Inst{15-9} = 0b0000001;
  let Inst{8} = f;
  let Inst{7-4} = rd{3-0};
  let Inst{3-0} = rr{3-0};
}

// 0000 0011 fddd frrr
class Mu3RdRr<bits<2> f, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<3> rd;              // accept 5 bits but only encode the lower 4
  bits<3> rr;              // accept 5 bits but only encode the lower 4

  let Inst{15-8} = 0b00000011;
  let Inst{7} = f{1};
  let Inst{6-4} = rd;
  let Inst{3} = f{0};
  let Inst{2-0} = rr;
}


//===----------------------------------------------------------------------===//
// Arithmetic word instructions (ADIW / SBIW): <|1001|011f|kkdd|kkkk|>
// f = secondary opcode = 1 bit
// k = constant data = 6 bits
// d = destination = 4 bits
// (Only accepts r25:24 r27:26 r29:28 r31:30)
//===----------------------------------------------------------------------===//
class FWRdK<bit f, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> dst;              // accept 5 bits but only encode bits 1 and 2
  bits<6> k;

  let Inst{15-9} = 0b1001011;
  let Inst{8} = f;
  let Inst{7-6} = k{5-4};
  let Inst{5-4} = dst{2-1};
  let Inst{3-0} = k{3-0};
}

//===----------------------------------------------------------------------===//
// In I/O instruction: <|1011|0AAd|dddd|AAAA|>
// A = I/O location address = 6 bits
// d = destination = 5 bits
// (Accepts all registers)
//===----------------------------------------------------------------------===//
class FIORdA<dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> d;
  bits<6> A;

  let Inst{15-11} = 0b10110;
  let Inst{10-9} = A{5-4};
  let Inst{8-4} = d;
  let Inst{3-0} = A{3-0};
}

//===----------------------------------------------------------------------===//
// Out I/O instruction: <|1011|1AAr|rrrr|AAAA|>
// A = I/O location address = 6 bits
// d = destination = 5 bits
// (Accepts all registers)
//===----------------------------------------------------------------------===//
class FIOARr<dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<6> A;
  bits<5> r;

  let Inst{15-11} = 0b10111;
  let Inst{10-9} = A{5-4};
  let Inst{8-4} = r;
  let Inst{3-0} = A{3-0};
}

//===----------------------------------------------------------------------===//
// I/O bit instruction.
// <|1001|10tt|AAAA|Abbb>
// t = type (1 for SBI, 0 for CBI)
// A = I/O location address (5 bits)
// b = bit number
//===----------------------------------------------------------------------===//
class FIOBIT<bits<2> t, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> A;
  bits<3> b;
  
  let Inst{15-12} = 0b1001;
  
  let Inst{11-10} = 0b10;
  let Inst{9-8} = t;
  
  let Inst{7-4} = A{4-1};
  
  let Inst{3} = A{0};
  let Inst{2-0} = b{2-0};
}

//===----------------------------------------------------------------------===//
// BST/BLD instruction.
// <|1111|1ttd|dddd|0bbb>
// t = type (1 for BST, 0 for BLD)
// d = destination register
// b = bit
//===----------------------------------------------------------------------===//
class FRdB<bits<2> t, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<5> rd;
  bits<3> b;

  let Inst{15-12} = 0b1111;

  let Inst{11} = 0b1;
  let Inst{10-9} = t;
  let Inst{8} = rd{4};

  let Inst{7-4} = rd{3-0};

  let Inst{3} = 0;
  let Inst{2-0} = b;
}

// <|1001|0100|KKKK|1011>
class FDES<dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<4> k;

  let Inst{15-12} = 0b1001;

  let Inst{11-8} = 0b0100;

  let Inst{7-4} = k;

  let Inst{3-0} = 0b1011;
}

//===----------------------------------------------------------------------===//
// Conditional Branching instructions: <|1111|0fkk|kkkk|ksss|>
// f = secondary opcode = 1 bit
// k = constant address = 7 bits
// s = bit in status register = 3 bits
//===----------------------------------------------------------------------===//
class FBRsk<bit f, bits<3> s, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<7> k;

  let Inst{15-11} = 0b11110;
  let Inst{10} = f;
  let Inst{9-3} = k;
  let Inst{2-0} = s;
}

//===----------------------------------------------------------------------===//
// Branching instructions: <|opcode|>
//===----------------------------------------------------------------------===//
class FBR<bits<16> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  let Inst = opcode;
}

//===----------------------------------------------------------------------===//
// Interrupt instructions: <|opcode|>
//===----------------------------------------------------------------------===//
class FI<bits<16> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  let Inst = opcode;
}

//===----------------------------------------------------------------------===//
// Special, opcode only instructions: <|opcode|>
//===----------------------------------------------------------------------===//

class F16<bits<16> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  let Inst = opcode;
}

class F32<bits<32> opcode, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst32<outs, ins, asmstr, pattern>
{
  let Inst = opcode;
}

//===----------------------------------------------------------------------===//
// Branching instructions with immediate12: <|110f|kkkk|kkkk|kkkk|>
// f = secondary opcode = 1 bit
// k = constant address = 12 bits
//===----------------------------------------------------------------------===//
class FBRk<bit f, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<12> k;

  let Inst{15-13} = 0b110;
  let Inst{12} = f;
  let Inst{11-0} = k;
}

//===----------------------------------------------------------------------===//
// 32 bits branching instructions: <|1001|010k|kkkk|fffk|kkkk|kkkk|kkkk|kkkk|>
// f = secondary opcode = 3 bits
// k = constant address = 22 bits
//===----------------------------------------------------------------------===//
class F32BRk<bits<3> f, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst32<outs, ins, asmstr, pattern>
{
  bits<22> k;

  let Inst{31-25} = 0b1001010;
  let Inst{24-20} = k{21-17};
  let Inst{19-17} = f;
  let Inst{16-0} = k{16-0};
}

//===----------------------------------------------------------------------===//
// 32 bits direct mem instructions: <|1001|00fd|dddd|0000|kkkk|kkkk|kkkk|kkkk|>
// f = secondary opcode = 1 bit
// d = destination = 5 bits
// k = constant address = 16 bits
// (Accepts all registers)
//===----------------------------------------------------------------------===//
class F32DM<bit f, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst32<outs, ins, asmstr, pattern>
{
  bits<16> k;
  bits<5> d;

  let Inst{31-26} = 0b100100;
  let Inst{25} = f;
  let Inst{24-20} = d;
  let Inst{19-16} = 0b0000;
  let Inst{15-0} = k{15-0};
}

//===----------------------------------------------------------------------===//
// 16 bits flag set/clear instruction: <|1001|0100|bfff|1000>
// b = mode (0 for set, 1 for clear)
// f = flag
//===----------------------------------------------------------------------===//
class FFL<bit b, bits<3> f, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  let Inst{15-12} = 0b1001;
  
  let Inst{11-8} = 0b0100;
  
  let Inst{7} = b;
  let Inst{6-4} = f;
  
  let Inst{3-0} = 0b1000;
}

// <|1001|0100|bfff|1000>
class FS<bit b, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<3> s;

  let Inst{15-12} = 0b1001;

  let Inst{11-8} = 0b0100;

  let Inst{7} = b;
  let Inst{6-4} = s;

  let Inst{3-0} = 0b1000;
}

// Set/clr bit in status flag instructions/
// <BRBS|BRBC> s, k
// ---------------------
// <|1111|0fkk|kkkk|ksss>
class FSK<bit f, dag outs, dag ins, string asmstr, list<dag> pattern>
  : AVRInst16<outs, ins, asmstr, pattern>
{
  bits<7> k;
  bits<3> s;

  let Inst{15-12} = 0b1111;

  let Inst{11} = 0;
  let Inst{10} = f;
  let Inst{9-8} = k{6-5};

  let Inst{7-4} = k{4-1};

  let Inst{3} = k{0};
  let Inst{2-0} = s;
}
