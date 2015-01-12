//===-- AVRRegisterInfo.td - AVR Register defs -------------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//  Declarations that describe the AVR register file
//===----------------------------------------------------------------------===//

// general purpose register definition
class AVRReg<bits<16> num, string n, list<Register> subregs = []>
  : RegisterWithSubRegs<n, subregs>
{
  field bits<16> Num = num;
  
  let HWEncoding = num;
  let Namespace = "AVR";
  let SubRegs = subregs;
}

// subregister indices
let Namespace = "AVR" in
{
  def sub_lo : SubRegIndex<8>;
  def sub_hi : SubRegIndex<8, 8>;
}

//===----------------------------------------------------------------------===//
//  Register definitions
//===----------------------------------------------------------------------===//

// 8 bit registers
def R0 : AVRReg<0, "r0">, DwarfRegNum<[0]>;
def R1 : AVRReg<1, "r1">, DwarfRegNum<[1]>;
def R2 : AVRReg<2, "r2">, DwarfRegNum<[2]>;
def R3 : AVRReg<3, "r3">, DwarfRegNum<[3]>;
def R4 : AVRReg<4, "r4">, DwarfRegNum<[4]>;
def R5 : AVRReg<5, "r5">, DwarfRegNum<[5]>;
def R6 : AVRReg<6, "r6">, DwarfRegNum<[6]>;
def R7 : AVRReg<7, "r7">, DwarfRegNum<[7]>;
def R8 : AVRReg<8, "r8">, DwarfRegNum<[8]>;
def R9 : AVRReg<9, "r9">, DwarfRegNum<[9]>;
def R10 : AVRReg<10, "r10">, DwarfRegNum<[10]>;
def R11 : AVRReg<11, "r11">, DwarfRegNum<[11]>;
def R12 : AVRReg<12, "r12">, DwarfRegNum<[12]>;
def R13 : AVRReg<13, "r13">, DwarfRegNum<[13]>;
def R14 : AVRReg<14, "r14">, DwarfRegNum<[14]>;
def R15 : AVRReg<15, "r15">, DwarfRegNum<[15]>;
def R16 : AVRReg<16, "r16">, DwarfRegNum<[16]>;
def R17 : AVRReg<17, "r17">, DwarfRegNum<[17]>;
def R18 : AVRReg<18, "r18">, DwarfRegNum<[18]>;
def R19 : AVRReg<19, "r19">, DwarfRegNum<[19]>;
def R20 : AVRReg<20, "r20">, DwarfRegNum<[20]>;
def R21 : AVRReg<21, "r21">, DwarfRegNum<[21]>;
def R22 : AVRReg<22, "r22">, DwarfRegNum<[22]>;
def R23 : AVRReg<23, "r23">, DwarfRegNum<[23]>;
def R24 : AVRReg<24, "r24">, DwarfRegNum<[24]>;
def R25 : AVRReg<25, "r25">, DwarfRegNum<[25]>;
def R26 : AVRReg<26, "r26">, DwarfRegNum<[26]>;
def R27 : AVRReg<27, "r27">, DwarfRegNum<[27]>;
def R28 : AVRReg<28, "r28">, DwarfRegNum<[28]>;
def R29 : AVRReg<29, "r29">, DwarfRegNum<[29]>;
def R30 : AVRReg<30, "r30">, DwarfRegNum<[30]>;
def R31 : AVRReg<31, "r31">, DwarfRegNum<[31]>;
def SPL : AVRReg<32, "SPL">, DwarfRegNum<[32]>;
def SPH : AVRReg<33, "SPH">, DwarfRegNum<[33]>;

let SubRegIndices = [sub_lo, sub_hi],
CoveredBySubRegs = 1 in
{
  // 16 bit register pairs
  def SP     : AVRReg<32, "SP",  [SPL, SPH]>, DwarfRegNum<[32]>;
  def R31R30 : AVRReg<30, "r30", [R30, R31]>, DwarfRegNum<[30]>;
  def R29R28 : AVRReg<28, "r28", [R28, R29]>, DwarfRegNum<[28]>;
  def R27R26 : AVRReg<26, "r26", [R26, R27]>, DwarfRegNum<[26]>;
  def R25R24 : AVRReg<24, "r24", [R24, R25]>, DwarfRegNum<[24]>;
  def R23R22 : AVRReg<22, "r22", [R22, R23]>, DwarfRegNum<[22]>;
  def R21R20 : AVRReg<20, "r20", [R20, R21]>, DwarfRegNum<[20]>;
  def R19R18 : AVRReg<18, "r18", [R18, R19]>, DwarfRegNum<[18]>;
  def R17R16 : AVRReg<16, "r16", [R16, R17]>, DwarfRegNum<[16]>;
  def R15R14 : AVRReg<14, "r14", [R14, R15]>, DwarfRegNum<[14]>;
  def R13R12 : AVRReg<12, "r12", [R12, R13]>, DwarfRegNum<[12]>;
  def R11R10 : AVRReg<10, "r10", [R10, R11]>, DwarfRegNum<[10]>;
  def R9R8 : AVRReg<8, "r8", [R8, R9]>, DwarfRegNum<[8]>;
  def R7R6 : AVRReg<6, "r6", [R6, R7]>, DwarfRegNum<[6]>;
  def R5R4 : AVRReg<4, "r4", [R4, R5]>, DwarfRegNum<[4]>;
  def R3R2 : AVRReg<2, "r2", [R2, R3]>, DwarfRegNum<[2]>;
  def R1R0 : AVRReg<0, "r0", [R0, R1]>, DwarfRegNum<[0]>;
}

//===----------------------------------------------------------------------===//
// Register Classes
//===----------------------------------------------------------------------===//

//:TODO: use proper set instructions instead of using always "add"

// main 8 bit register class
def GPR8 : RegisterClass<"AVR", [i8], 8,
  (
    // return value and arguments
    add R24, R25, R18, R19, R20, R21, R22, R23,
    // scratch registers
    R30, R31, R26, R27,
    // callee saved registers
    R28, R29, R17, R16, R15, R14, R13, R12, R11, R10,
    R9, R8, R7, R6, R5, R4, R3, R2, R0, R1
  )>;

// Simple lower registers r0..r15
def GPR8lo : RegisterClass<"AVR", [i8], 8,
  (
    add R15, R14, R13, R12, R11, R10, R9, R8, R7, R6, R5, R4, R3, R2, R0, R1
  )>;

// 8 bit register class for imm instructions
def LD8 : RegisterClass<"AVR", [i8], 8,
  (
    // return value and arguments
    add R24, R25, R18, R19, R20, R21, R22, R23,
    // scratch registers
    R30, R31, R26, R27,
    // callee saved registers
    R28, R29, R17, R16
  )>;

// Simple lower registers r16..r23
def LD8lo : RegisterClass<"AVR", [i8], 8,
  (
    add R23, R22, R21, R20, R19, R18, R17, R16
  )>;

// main 16 bit pair register class
def DREGS : RegisterClass<"AVR", [i16], 8,
  (
    // return value and arguments
    add R25R24, R19R18, R21R20, R23R22,
    // scratch registers
    R31R30, R27R26,
    // callee saved registers
    R29R28, R17R16, R15R14, R13R12, R11R10,
    R9R8, R7R6, R5R4, R3R2, R1R0
  )>;

// 16 bit register class for imm instructions
def DLDREGS : RegisterClass<"AVR", [i16], 8,
  (
    // return value and arguments
    add R25R24, R19R18, R21R20, R23R22,
    // scratch registers
    R31R30, R27R26,
    // callee saved registers
    R29R28, R17R16
  )>;

// 16 bit register class for the adiw/sbiw instructions
def IWREGS : RegisterClass<"AVR", [i16], 8,
  (
    // return value and arguments
    add R25R24,
    // scratch registers
    R31R30, R27R26,
    // callee saved registers
    R29R28
  )>;

// 16 bit register class for the ld and st instructions
// aka x, y, z
def PTRREGS : RegisterClass<"AVR", [i16], 8,
  (
    add R27R26, // X
        R29R28, // Y
        R31R30  // Z
  )>;

// 16 bit register class for the ldd and std instructions
// aka y, z
def PTRDISPREGS : RegisterClass<"AVR", [i16], 8,
  (
    add R31R30, R29R28
  )>;

// Register class used for the stack read pseudo instruction
def GPRSP: RegisterClass<"AVR", [i16], 8, (add SP)>;

//:TODO: if we remove this we get an error in tablegen
//:TODO: this is just a hack, remove it once add16 works!
def SREG : AVRReg<14, "FLAGS">, DwarfRegNum<[88]>;
def CCR : RegisterClass<"AVR", [i8], 8, (add SREG)>
{
  let CopyCost = -1;      // Don't allow copying of status registers
}

//===----------------------------------------------------------------------===//
// Register Classes
//===----------------------------------------------------------------------===//


