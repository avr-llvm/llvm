//===-- AVR.td - Describe the AVR Target Machine -----------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// This is the top level entry point for the AVR target.
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Target-independent interfaces which we are implementing
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// AVR Subtarget Features.
//===----------------------------------------------------------------------===//


//===----------------------------------------------------------------------===//
// AVR microcontrollers supported.
//===----------------------------------------------------------------------===//

class Device<string Name, list<SubtargetFeature> Features>
  : Processor<Name, NoItineraries, Features>;

def : Device<"generic", []>;

//:FIXME: list here all mcu models

//===----------------------------------------------------------------------===//
// Register File Description
//===----------------------------------------------------------------------===//

include "AVRRegisterInfo.td"

//===----------------------------------------------------------------------===//
// Instruction Descriptions
//===----------------------------------------------------------------------===//

include "AVRInstrInfo.td"

def AVRInstrInfo : InstrInfo;

//===----------------------------------------------------------------------===//
// Calling Conventions
//===----------------------------------------------------------------------===//

include "AVRCallingConv.td"

//===----------------------------------------------------------------------===//
// Assembly Printers
//===----------------------------------------------------------------------===//

def AVRAsmWriter : AsmWriter
{
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}

//===----------------------------------------------------------------------===//
// Target Declaration
//===----------------------------------------------------------------------===//

def AVR : Target
{
  let InstructionSet = AVRInstrInfo;
  let AssemblyWriters = [AVRAsmWriter];
}
