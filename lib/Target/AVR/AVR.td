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

// The device has the `LPM` instruction (with implied operand being r0).
def FeatureILPM         : SubtargetFeature<"lpm", "HasLPM", "true",
                                "Enable 'lmp' instruction">;

def FeatureILPMX        : SubtargetFeature<"lpmx", "HasLPMX", "true",
                                "Enable 'lpm Rd, Z[+]` instruction">;

def FeatureIELPM        : SubtargetFeature<"elpm", "HasELPM", "true",
                                "Enable 'elpm' instruction">;

def FeatureIELPMX       : SubtargetFeature<"elpmx", "HasELPMX", "true",
                                "Enable 'elpm Rd, Z[+]' instruction">;

def FeatureIMUL         : SubtargetFeature<"mul", "HasMUL", "true",
                                "Enable 'mul Rd, Rr' instruction">;

def FeatureIFMUL        : SubtargetFeature<"fmul", "HasFMUL", "true",
                                "Enable 'fmul Rd, Rr' instruction">;

def FeatureIMOVW         : SubtargetFeature<"movw", "HasMOVW", "true",
                                "Enable 'movw Rd:Rd+1, Rr:Rr+1' instruction">;

def FeatureIBREAK       : SubtargetFeature<"break", "HasBREAK", "true",
                                "Enable 'break' instruction">;

// The device is self-programmable.
def FeatureISPM         : SubtargetFeature<"spm", "HasSPM", "true",
                                "Enable 'spm' instruction">;

def FeatureISPMX        : SubtargetFeature<"spmx", "HasSPMX", "true",
                                "Enable 'spm Z[+]' instruction">;

def FeatureRMW          : SubtargetFeature<"rmw", "HasRMW", "true",
                                "Enable xch,lac,las,lat instructions">;

def FeatureSRAM         : SubtargetFeature<"sram", "HasSRAM", "true",
                                "The device has SRAM">;

def FeatureDES          : SubtargetFeature<"des", "SupportsDES", "true",
                                "The device supports special DES instructions">;

// Device has >128K program memory (none yet).
def FeatureEIND         : SubtargetFeature<"eind", "HasEIND", "true",
                                "The device has more than 128K program memory">;

def FeatureTiny         : SubtargetFeature<"tiny", "IsTiny", "true",
                                "The device supports instructions specific to the Tiny family">;

def FeatureMega         : SubtargetFeature<"mega", "IsMega", "true",
                                "The device belongs to the Mega family">;

def FeatureTiny1        : SubtargetFeature<"tiny1", "SupportsTiny1", "true",
                                "gredsf", [FeatureILPM]>;

def Feature2xxx         : SubtargetFeature<"2xxx", "Supports2xxx", "true",
                                "sadasds", [FeatureTiny1,FeatureSRAM]>;

def Feature2xxe         : SubtargetFeature<"2xxe", "Supports2xxe", "true",
                                "asdsd", [Feature2xxx,FeatureILPMX]>;

def FeatureRF401        : SubtargetFeature<"rf401", "SupportsRF401", "true",
                                "adssd", [Feature2xxx,FeatureIMOVW,FeatureILPMX]>;

def FeatureTiny2        : SubtargetFeature<"tiny2", "SupportsTiny2", "true",
                                "greyge", [Feature2xxx, FeatureIMOVW, FeatureILPMX, FeatureISPM, FeatureIBREAK]>;

def FeatureM603         : SubtargetFeature<"m603", "SupportsM603", "true",
                                "sadsf", [Feature2xxx,FeatureMega]>;

def FeatureM103         : SubtargetFeature<"m103", "SupportsM103", "true",
                                "sadsf", [FeatureM603,FeatureIELPM]>;

def FeatureM8           : SubtargetFeature<"m8", "SupportsM8", "true",
                                "sadsf", [Feature2xxx,FeatureIMUL,FeatureIMOVW,FeatureILPMX,FeatureISPM]>;

def FeaturePWMx         : SubtargetFeature<"pwmx", "SupportsPWMx", "true",
                                "sadsf", [FeatureM8,FeatureIBREAK]>;

def FeatureM161         : SubtargetFeature<"m161", "SupportsM161", "true",
                                "sadsf", [FeatureM603,FeatureIMUL,FeatureIMOVW,FeatureILPMX,FeatureISPM]>;

def FeatureM94K         : SubtargetFeature<"m94k", "SupportsM94k", "true",
                                "sadsf", [FeatureM603,FeatureIMUL,FeatureIMOVW,FeatureILPMX]>;

def FeatureM323         : SubtargetFeature<"m323", "SupportsM323", "true",
                                "sadsf", [FeatureM161,FeatureIBREAK]>;

def FeatureM128         : SubtargetFeature<"m128", "SupportsM128", "true",
                                "sadsf", [FeatureM323,FeatureIELPM,FeatureIELPMX]>;

def FeatureM256         : SubtargetFeature<"m256", "SupportsM256", "true",
                                "sadsf", [FeatureM128,FeatureEIND]>;

def FeatureXMEGA        : SubtargetFeature<"xmega", "SupportsXMEGA", "true",
                                "The device belongs to the XMEGA family", [FeatureM256,FeatureISPMX,FeatureDES]>;

def FeatureXMEGAU       : SubtargetFeature<"xmegau", "SupportsXMEGAU", "true",
                                "dsaf", [FeatureXMEGA,FeatureRMW]>;

def FeatureAVR1         : SubtargetFeature<"avr1", "SupportsAVR1", "true",
                                "The device supports AVR architecture 1", [FeatureTiny1]>;

def FeatureAVR2         : SubtargetFeature<"avr2", "SupportsAVR2", "true",
                                "The device supports AVR architecture 2", [Feature2xxx]>;

def FeatureAVR25        : SubtargetFeature<"avr25", "SupportsAVR25", "true",
                                "The device supports AVR architecture 25", [FeatureTiny2]>;

def FeatureAVR3         : SubtargetFeature<"avr3", "SupportsAVR3", "true",
                                "The device supports AVR architecture 3", [FeatureM603]>;

def FeatureAVR31        : SubtargetFeature<"avr31", "SupportsAVR31", "true",
                                "The device supports AVR architecture 31", [FeatureM103]>;

def FeatureAVR35        : SubtargetFeature<"avr35", "SupportsAVR35", "true",
                                "The device supports AVR architecture 35", [FeatureAVR3,FeatureIMOVW,FeatureILPMX,FeatureISPM,FeatureIBREAK]>;

def FeatureAVR3_ALL     : SubtargetFeature<"avr3_all", "SupportsAVR3_ALL", "true",
                                "The device supports AVR architecture 3,31 and 35", [FeatureAVR3,FeatureAVR31,FeatureAVR35]>;

def FeatureAVR4         : SubtargetFeature<"avr4", "SupportsAVR4", "true",
                                "The device supports AVR architecture 4", [FeaturePWMx]>;

def FeatureAVR5         : SubtargetFeature<"avr5", "SupportsAVR5", "true",
                                "The device supports AVR architecture 5", [FeatureM323]>;

def FeatureAVR6         : SubtargetFeature<"avr6", "SupportsAVR6", "true",
                                "The device supports AVR architecture 6",
                                [FeatureILPM,FeatureILPMX,FeatureSRAM,FeatureMega,
                                 FeatureIMUL,FeatureIELPM,FeatureIELPMX,FeatureISPM,
                                 FeatureIBREAK,FeatureEIND,FeatureIMOVW]>;

def FeatureAVRTiny      : SubtargetFeature<"avrtiny", "SupportsTiny", "true",
                                "The device belongs to the Tiny family",
                                [FeatureIBREAK,FeatureSRAM,FeatureTiny]>;

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
