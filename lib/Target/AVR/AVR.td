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

// A pseudo subtarget feature aggregates features, grouping them. We don't want
// to create a new member in AVRSubtarget (to store a value) for each group because
// we do not care if the group is supported, only the subfeatures inside the group.
// We fix this by simply setting a dummy member for each pseudo feature, which is
// then ignored.
class PseudoSubtargetFeature<string name, string desc, list<SubtargetFeature> i>
  : SubtargetFeature<name, "DummyFeature", "true", desc, i> { }

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

// Device has >128K program memory.
def FeatureEIND         : SubtargetFeature<"eind", "HasEIND", "true",
                                "The device has more than 128K program memory">;

def FeatureTiny         : SubtargetFeature<"tiny", "IsTiny", "true",
                                "The device supports instructions specific to the Tiny family">;

def FeatureMega         : SubtargetFeature<"mega", "IsMega", "true",
                                "The device belongs to the Mega family">;

def FeatureTiny1        : PseudoSubtargetFeature<"tiny1", "gredsf",
                            [FeatureILPM]>;

def Feature2xxx         : PseudoSubtargetFeature<"2xxx", "sadasds",
                            [FeatureTiny1, FeatureSRAM]>;

def Feature2xxe         : PseudoSubtargetFeature<"2xxe", "asdsd",
                            [Feature2xxx, FeatureILPMX]>;

def FeatureRF401        : PseudoSubtargetFeature<"rf401", "adssd",
                            [Feature2xxx, FeatureIMOVW, FeatureILPMX]>;

def FeatureTiny2        : PseudoSubtargetFeature<"tiny2", "greyge",
                            [Feature2xxx, FeatureIMOVW, FeatureILPMX, FeatureISPM, FeatureIBREAK]>;

def FeatureM603         : PseudoSubtargetFeature<"m603", "sadsf",
                            [Feature2xxx, FeatureMega]>;

def FeatureM103         : PseudoSubtargetFeature<"m103", "sadsf",
                            [FeatureM603, FeatureIELPM]>;

def FeatureM8           : PseudoSubtargetFeature<"m8", "sadsf",
                            [Feature2xxx,  FeatureIMUL, FeatureIMOVW,
                             FeatureILPMX, FeatureISPM]>;

def FeaturePWMx         : PseudoSubtargetFeature<"pwmx", "sadsf",
                            [FeatureM8, FeatureIBREAK]>;

def FeatureM161         : PseudoSubtargetFeature<"m161", "sadsf",
                            [FeatureM603, FeatureIMUL, FeatureIMOVW,
                             FeatureILPMX ,FeatureISPM]>;

def FeatureM94K         : PseudoSubtargetFeature<"m94k", "sadsf",
                            [FeatureM603,FeatureIMUL,
                             FeatureIMOVW,FeatureILPMX]>;

def FeatureM323         : PseudoSubtargetFeature<"m323", "sadsf",
                            [FeatureM161, FeatureIBREAK]>;

def FeatureM128         : PseudoSubtargetFeature<"m128", "sadsf",
                            [FeatureM323, FeatureIELPM, FeatureIELPMX]>;

def FeatureM256         : PseudoSubtargetFeature<"m256", "sadsf",
                            [FeatureM128, FeatureEIND]>;

def FeatureXMEGA        : PseudoSubtargetFeature<"xmega", "The device belongs to the XMEGA family",
                            [FeatureM256, FeatureISPMX, FeatureDES]>;

def FeatureXMEGAU       : PseudoSubtargetFeature<"xmegau", "dsaf",
                            [FeatureXMEGA, FeatureRMW]>;

def FeatureAVR1         : PseudoSubtargetFeature<"avr1", "The device supports AVR architecture 1",
                            [FeatureTiny1]>;

def FeatureAVR2         : PseudoSubtargetFeature<"avr2", "The device supports AVR architecture 2",
                            [Feature2xxx]>;

def FeatureAVR25        : PseudoSubtargetFeature<"avr25", "The device supports AVR architecture 25",
                            [FeatureTiny2]>;

def FeatureAVR3         : PseudoSubtargetFeature<"avr3", "The device supports AVR architecture 3",
                            [FeatureM603]>;

def FeatureAVR31        : PseudoSubtargetFeature<"avr31", "The device supports AVR architecture 31",
                            [FeatureM103]>;

def FeatureAVR35        : PseudoSubtargetFeature<"avr35", "The device supports AVR architecture 35",
                            [FeatureAVR3, FeatureIMOVW, FeatureILPMX,
                             FeatureISPM, FeatureIBREAK]>;

def FeatureAVR3_ALL     : PseudoSubtargetFeature<"avr3_all", "The device supports AVR architecture 3,31 and 35",
                            [FeatureAVR3, FeatureAVR31, FeatureAVR35]>;

def FeatureAVR4         : PseudoSubtargetFeature<"avr4", "The device supports AVR architecture 4",
                            [FeaturePWMx]>;

def FeatureAVR5         : PseudoSubtargetFeature<"avr5", "The device supports AVR architecture 5",
                            [FeatureM323]>;

def FeatureAVR6         : PseudoSubtargetFeature<"avr6", "The device supports AVR architecture 6",
                            [FeatureILPM,   FeatureILPMX, FeatureSRAM,   FeatureMega,
                             FeatureIMUL,   FeatureIELPM, FeatureIELPMX, FeatureISPM,
                             FeatureIBREAK, FeatureEIND,  FeatureIMOVW]>;

def FeatureAVRTiny      : PseudoSubtargetFeature<"avrtiny", "The device belongs to the Tiny family",
                            [FeatureIBREAK, FeatureSRAM, FeatureTiny]>;

//===----------------------------------------------------------------------===//
// AVR microcontrollers supported.
//===----------------------------------------------------------------------===//

class Device<string Name, list<SubtargetFeature> Features>
  : Processor<Name, NoItineraries, Features>;

def : Device<"avr1",  [FeatureAVR1]>;
def : Device<"avr2",  [FeatureAVR2]>;
def : Device<"avr25", [FeatureAVR25]>;
def : Device<"avr3",  [FeatureAVR3_ALL]>;
def : Device<"avr31", [FeatureAVR31]>;
def : Device<"avr35", [FeatureAVR35]>;
def : Device<"avr4",  [FeatureAVR4]>;
def : Device<"avr5",  [FeatureAVR5]>;
//def : Device<"avr51", [FeatureAVR51>;
def : Device<"avr6",  [FeatureAVR6]>;

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
