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

// :TODO: Implement the skip errata, see `gcc/config/avr/avr-arch.h` for details.


// A pseudo subtarget feature aggregates features, grouping them. We don't want
// to create a new member in AVRSubtarget (to store a value) for each group because
// we do not care if the group is supported, only the subfeatures inside the group.
// We fix this by simply setting a dummy member for each pseudo feature, which is
// then ignored.
class PseudoSubtargetFeature<string name, string desc, list<SubtargetFeature> i>
  : SubtargetFeature<name, "m_DummyFeature", "true", desc, i> { }

// The device has SRAM, and supports the bare minimum of SRAM-relevant instructions.
// These are:
// LD - all 9 variants
// ST - all 9 variants
// LDD - two variants for Y and Z
// STD - two variants for Y and Z
// `LDS Rd, K`
// `STS k, Rr`
// `PUSH`/`POP`
def FeatureSRAM           : SubtargetFeature<"sram", "m_hasSRAM", "true",
                                  "The device has random access memory">;

// The device supports the `JMP k` and `CALL k` instructions.
def FeatureJMPCALL        : SubtargetFeature<"jmpcall", "m_hasJMPCALL", "true",
                                  "The device supports the `JMP` and `CALL` instructions">;

// The device supports the indirect branches `IJMP` and `ICALL`.
def FeatureIJMPCALL       : SubtargetFeature<"ijmpcall", "m_hasIJMPCALL", "true",
                                  "The device supports `IJMP`/`ICALL` instructions">;

// The device supports the extended indirect branches `EIJMP` and `EICALL`.
def FeatureEIJMPCALL      : SubtargetFeature<"eijmpcall", "m_hasEIJMPCALL", "true",
                                  "The device supports the `EIJMP`/`EICALL` instructions">;

// The device supports `ADDI Rd, K`, `SUBI Rd, K`.
def FeatureADDSUBIW       : SubtargetFeature<"addsubiw", "m_hasADDSUBIW", "true",
                                  "Enable 16-bit register-immediate addition and subtraction instructions">;

// The device has an 8-bit stack pointer (SP) register.
def FeatureSmallStack     : SubtargetFeature<"smallstack", "m_hasSmallStack", "true",
                                  "The device has an 8-bit stack pointer">;

// The device supports the 16-bit GPR pair MOVW instruction.
def FeatureMOVW           : SubtargetFeature<"movw", "m_hasMOVW", "true",
                                  "The device supports the 16-bit MOVW instruction">;

// The device supports the `LPM` instruction, with implied destination being r0.
def FeatureLPM            : SubtargetFeature<"lpm", "m_hasLPM", "true",
                                  "The device supports the `LPM` instruction">;

// The device supports the `LPM Rd, Z[+] instruction.
def FeatureLPMX           : SubtargetFeature<"lpmx", "m_hasLPMX", "true",
                                  "The device supports the `LPM Rd, Z[+]` instruction">;

// The device supports the `ELPM` instruction.
def FeatureELPM           : SubtargetFeature<"elpm", "m_hasELPM", "true",
                                  "The device supports the ELPM instruction">;

// The device supports the `ELPM Rd, Z[+]` instructions.
def FeatureELPMX          : SubtargetFeature<"elpmx", "m_hasELPMX", "true",
                                  "The device supports the `ELPM Rd, Z[+]` instructions">;

// The device supports the `SPM` instruction.
def FeatureSPM            : SubtargetFeature<"spm", "m_hasSPM", "true",
                                  "The device supports the `SPM` instruction">;

// The device supports the `SPM Z+` instruction.
def FeatureSPMX           : SubtargetFeature<"spmx", "m_hasSPMX", "true",
                                  "The device supports the `SPM Z+` instruction">;

// The device supports the `DES k` instruction.
def FeatureDES            : SubtargetFeature<"des", "m_hasDES", "true",
                                  "The device supports the `DES k` instruction">;

// The device supports the Read-Write-Modify instructions XCH, LAS, LAC, and LAT.
def FeatureRMW            : SubtargetFeature<"rmw", "m_supportsRMW", "true",
                                  "The device supports the read-write-modify instructions XCH, LAS, LAC, LAT">;

// The device supports the `[F]MUL[S][U]` family of instructions.
def FeatureMultiplication : SubtargetFeature<"mul", "m_supportsMultiplication", "true",
                                  "The device supports the multiplication instructions">;

// The device supports the `BREAK` instruction.
def FeatureBREAK          : SubtargetFeature<"break", "m_hasBREAK", "true",
                                  "The device supports the `BREAK` instruction">;

// The device has instruction encodings specific to the Tiny core.
def FeatureTinyEncoding   : SubtargetFeature<"tinyencoding", "m_hasTinyEncoding", "true",
                                  "The device has Tiny core specific instruction encodings">;

// The device has at least the bare minimum that **every** single AVR device should have.
def FeatureAVR0           : PseudoSubtargetFeature<"avr0", "The device is an AVR",
                                  []>;

// The device is a part of the avr1 family.
def FeatureAVR1           : PseudoSubtargetFeature<"avr1", "The device is a part of the avr1 family",
                                  [FeatureAVR0, FeatureLPM]>;

// The device is a part of the avr2 family.
def FeatureAVR2           : PseudoSubtargetFeature<"avr2", "The device is a part of the avr2 family",
                                  [FeatureAVR1, FeatureIJMPCALL, FeatureADDSUBIW, FeatureSRAM]>;

// The device is a part of the avr25 family.
def FeatureAVR25          : PseudoSubtargetFeature<"avr25", "The device is a part of the avr25 family",
                                  [FeatureAVR2, FeatureMOVW, FeatureLPMX, FeatureSPM, FeatureBREAK]>;

// The device is a part of the avr3 family.
def FeatureAVR3           : PseudoSubtargetFeature<"avr3", "The device is a part of the avr3 family",
                                  [FeatureAVR2, FeatureJMPCALL]>;

// The device is a part of the avr31 family.
def FeatureAVR31          : PseudoSubtargetFeature<"avr31", "The device is a part of the avr31 family",
                                  [FeatureAVR3, FeatureELPM]>;

// The device is a part of the avr35 family.
def FeatureAVR35          : PseudoSubtargetFeature<"avr35", "The device is a part of the avr35 family",
                                  [FeatureAVR3, FeatureMOVW, FeatureLPMX, FeatureSPM, FeatureBREAK]>;

// The device is a part of the avr4 family.
def FeatureAVR4           : PseudoSubtargetFeature<"avr4", "The device is a part of the avr4 family",
                                  [FeatureAVR2, FeatureMultiplication, FeatureMOVW,
                                   FeatureLPMX, FeatureSPM,  FeatureBREAK]>;

// The device is a part of the avr5 family.
def FeatureAVR5           : PseudoSubtargetFeature<"avr5", "The device is a part of the avr5 family",
                                  [FeatureAVR3, FeatureMultiplication, FeatureMOVW,
                                   FeatureLPMX, FeatureSPM, FeatureBREAK]>;

// The device is part of the avr51 family.
def FeatureAVR51          : PseudoSubtargetFeature<"avr51", "The device is part of the avr52 family",
                                  [FeatureAVR5, FeatureELPM, FeatureELPMX]>;

// The device is part of the avr6 family.
def FeatureAVR6           : PseudoSubtargetFeature<"avr6", "The device is part of the avr6 family",
                                  [FeatureAVR0, FeatureLPM, FeatureLPMX, FeatureSRAM,
                                   FeatureJMPCALL, FeatureMultiplication, FeatureELPM,
                                   FeatureELPMX, FeatureSPM, FeatureBREAK, FeatureEIJMPCALL, FeatureMOVW]>;

// The device is a part of the tiny family.
def FeatureAVRTiny        : PseudoSubtargetFeature<"avrtiny", "The device is a part of the tiny family",
                                  [FeatureAVR0, FeatureBREAK, FeatureSRAM, FeatureTinyEncoding]>;

// The device is a part of the XMega family.
def FeatureXMEGA          : PseudoSubtargetFeature<"xmega", "The device is a part of the XMEGA family",
                                  [FeatureAVR51, FeatureEIJMPCALL, FeatureSPMX, FeatureDES]>;

// The device is part of the XMegaU family.
def FeatureXMEGAU         : PseudoSubtargetFeature<"xmegau", "The device is part of the XMEGAU family",
                                  [FeatureXMEGA, FeatureRMW]>;

//===----------------------------------------------------------------------===//
// AVR microcontrollers supported.
//===----------------------------------------------------------------------===//

class Device<string Name, list<SubtargetFeature> Features>
  : Processor<Name, NoItineraries, Features>;

// Generic MCUs
def : Device<"avr1",  [FeatureAVR1]>;
def : Device<"avr2",  [FeatureAVR2]>;
def : Device<"avr25", [FeatureAVR25]>;
def : Device<"avr3",  [FeatureAVR3]>;
def : Device<"avr31", [FeatureAVR31]>;
def : Device<"avr35", [FeatureAVR35]>;
def : Device<"avr4",  [FeatureAVR4]>;
def : Device<"avr5",  [FeatureAVR5]>;
def : Device<"avr51", [FeatureAVR51]>;
def : Device<"avr6",  [FeatureAVR6]>;

// Specific MCUs
def : Device<"avrxmega1",          [FeatureXMEGA]>;
def : Device<"avrxmega2",          [FeatureXMEGA]>;
def : Device<"avrxmega3",          [FeatureXMEGA]>;
def : Device<"avrxmega4",          [FeatureXMEGA]>;
def : Device<"avrxmega5",          [FeatureXMEGA]>;
def : Device<"avrxmega6",          [FeatureXMEGA]>;
def : Device<"avrxmega7",          [FeatureXMEGA]>;
def : Device<"avrtiny",            [FeatureAVRTiny]>;
def : Device<"at90s1200",          [FeatureAVR0]>;
def : Device<"attiny11",           [FeatureAVR1]>;
def : Device<"attiny12",           [FeatureAVR1]>;
def : Device<"attiny15",           [FeatureAVR1]>;
def : Device<"attiny28",           [FeatureAVR1]>;
def : Device<"at90s2313",          [FeatureAVR2]>;
def : Device<"at90s2323",          [FeatureAVR2]>;
def : Device<"at90s2333",          [FeatureAVR2]>;
def : Device<"at90s2343",          [FeatureAVR2]>;
def : Device<"attiny22",           [FeatureAVR2]>;
def : Device<"attiny26",           [FeatureAVR2, FeatureLPMX]>;
def : Device<"at86rf401",          [FeatureAVR2, FeatureMOVW, FeatureLPMX]>;
def : Device<"at90s4414",          [FeatureAVR2]>;
def : Device<"at90s4433",          [FeatureAVR2]>;
def : Device<"at90s4434",          [FeatureAVR2]>;
def : Device<"at90s8515",          [FeatureAVR2]>;
def : Device<"at90c8534",          [FeatureAVR2]>;
def : Device<"at90s8535",          [FeatureAVR2]>;
def : Device<"ata5272",            [FeatureAVR25]>;
def : Device<"attiny13",           [FeatureAVR25]>;
def : Device<"attiny13a",          [FeatureAVR25]>;
def : Device<"attiny2313",         [FeatureAVR25]>;
def : Device<"attiny2313a",        [FeatureAVR25]>;
def : Device<"attiny24",           [FeatureAVR25]>;
def : Device<"attiny24a",          [FeatureAVR25]>;
def : Device<"attiny4313",         [FeatureAVR25]>;
def : Device<"attiny44",           [FeatureAVR25]>;
def : Device<"attiny44a",          [FeatureAVR25]>;
def : Device<"attiny84",           [FeatureAVR25]>;
def : Device<"attiny84a",          [FeatureAVR25]>;
def : Device<"attiny25",           [FeatureAVR25]>;
def : Device<"attiny45",           [FeatureAVR25]>;
def : Device<"attiny85",           [FeatureAVR25]>;
def : Device<"attiny261",          [FeatureAVR25]>;
def : Device<"attiny261a",         [FeatureAVR25]>;
def : Device<"attiny461",          [FeatureAVR25]>;
def : Device<"attiny461a",         [FeatureAVR25]>;
def : Device<"attiny861",          [FeatureAVR25]>;
def : Device<"attiny861a",         [FeatureAVR25]>;
def : Device<"attiny87",           [FeatureAVR25]>;
def : Device<"attiny43u",          [FeatureAVR25]>;
def : Device<"attiny48",           [FeatureAVR25]>;
def : Device<"attiny88",           [FeatureAVR25]>;
def : Device<"attiny828",          [FeatureAVR25]>;
def : Device<"at43usb355",         [FeatureAVR3]>;
def : Device<"at76c711",           [FeatureAVR3]>;
def : Device<"atmega103",          [FeatureAVR31]>;
def : Device<"at43usb320",         [FeatureAVR31]>;
def : Device<"attiny167",          [FeatureAVR35]>;
def : Device<"at90usb82",          [FeatureAVR35]>;
def : Device<"at90usb162",         [FeatureAVR35]>;
def : Device<"ata5505",            [FeatureAVR35]>;
def : Device<"atmega8u2",          [FeatureAVR35]>;
def : Device<"atmega16u2",         [FeatureAVR35]>;
def : Device<"atmega32u2",         [FeatureAVR35]>;
def : Device<"attiny1634",         [FeatureAVR35]>;
def : Device<"atmega8",            [FeatureAVR4]>;
def : Device<"ata6289",            [FeatureAVR4]>;
def : Device<"atmega8a",           [FeatureAVR4]>;
def : Device<"ata6285",            [FeatureAVR4]>;
def : Device<"ata6286",            [FeatureAVR4]>;
def : Device<"atmega48",           [FeatureAVR4]>;
def : Device<"atmega48a",          [FeatureAVR4]>;
def : Device<"atmega48pa",         [FeatureAVR4]>;
def : Device<"atmega48p",          [FeatureAVR4]>;
def : Device<"atmega88",           [FeatureAVR4]>;
def : Device<"atmega88a",          [FeatureAVR4]>;
def : Device<"atmega88p",          [FeatureAVR4]>;
def : Device<"atmega88pa",         [FeatureAVR4]>;
def : Device<"atmega8515",         [FeatureAVR2, FeatureMultiplication, FeatureMOVW, FeatureLPMX, FeatureSPM]>;
def : Device<"atmega8535",         [FeatureAVR2, FeatureMultiplication, FeatureMOVW, FeatureLPMX, FeatureSPM]>;
def : Device<"atmega8hva",         [FeatureAVR4]>;
def : Device<"at90pwm1",           [FeatureAVR4]>;
def : Device<"at90pwm2",           [FeatureAVR4]>;
def : Device<"at90pwm2b",          [FeatureAVR4]>;
def : Device<"at90pwm3",           [FeatureAVR4]>;
def : Device<"at90pwm3b",          [FeatureAVR4]>;
def : Device<"at90pwm81",          [FeatureAVR4]>;
def : Device<"ata5790",            [FeatureAVR5]>;
def : Device<"ata5795",            [FeatureAVR5]>;
def : Device<"atmega16",           [FeatureAVR5]>;
def : Device<"atmega16a",          [FeatureAVR5]>;
def : Device<"atmega161",          [FeatureAVR3, FeatureMultiplication, FeatureMOVW, FeatureLPMX, FeatureSPM]>;
def : Device<"atmega162",          [FeatureAVR5]>;
def : Device<"atmega163",          [FeatureAVR3, FeatureMultiplication, FeatureMOVW, FeatureLPMX, FeatureSPM]>;
def : Device<"atmega164a",         [FeatureAVR5]>;
def : Device<"atmega164p",         [FeatureAVR5]>;
def : Device<"atmega164pa",        [FeatureAVR5]>;
def : Device<"atmega165",          [FeatureAVR5]>;
def : Device<"atmega165a",         [FeatureAVR5]>;
def : Device<"atmega165p",         [FeatureAVR5]>;
def : Device<"atmega165pa",        [FeatureAVR5]>;
def : Device<"atmega168",          [FeatureAVR5]>;
def : Device<"atmega168a",         [FeatureAVR5]>;
def : Device<"atmega168p",         [FeatureAVR5]>;
def : Device<"atmega168pa",        [FeatureAVR5]>;
def : Device<"atmega169",          [FeatureAVR5]>;
def : Device<"atmega169a",         [FeatureAVR5]>;
def : Device<"atmega169p",         [FeatureAVR5]>;
def : Device<"atmega169pa",        [FeatureAVR5]>;
def : Device<"atmega32",           [FeatureAVR5]>;
def : Device<"atmega32a",          [FeatureAVR5]>;
def : Device<"atmega323",          [FeatureAVR5]>;
def : Device<"atmega324a",         [FeatureAVR5]>;
def : Device<"atmega324p",         [FeatureAVR5]>;
def : Device<"atmega324pa",        [FeatureAVR5]>;
def : Device<"atmega325",          [FeatureAVR5]>;
def : Device<"atmega325a",         [FeatureAVR5]>;
def : Device<"atmega325p",         [FeatureAVR5]>;
def : Device<"atmega325pa",        [FeatureAVR5]>;
def : Device<"atmega3250",         [FeatureAVR5]>;
def : Device<"atmega3250a",        [FeatureAVR5]>;
def : Device<"atmega3250p",        [FeatureAVR5]>;
def : Device<"atmega3250pa",       [FeatureAVR5]>;
def : Device<"atmega328",          [FeatureAVR5]>;
def : Device<"atmega328p",         [FeatureAVR5]>;
def : Device<"atmega329",          [FeatureAVR5]>;
def : Device<"atmega329a",         [FeatureAVR5]>;
def : Device<"atmega329p",         [FeatureAVR5]>;
def : Device<"atmega329pa",        [FeatureAVR5]>;
def : Device<"atmega3290",         [FeatureAVR5]>;
def : Device<"atmega3290a",        [FeatureAVR5]>;
def : Device<"atmega3290p",        [FeatureAVR5]>;
def : Device<"atmega3290pa",       [FeatureAVR5]>;
def : Device<"atmega406",          [FeatureAVR5]>;
def : Device<"atmega64",           [FeatureAVR5]>;
def : Device<"atmega64a",          [FeatureAVR5]>;
def : Device<"atmega640",          [FeatureAVR5]>;
def : Device<"atmega644",          [FeatureAVR5]>;
def : Device<"atmega644a",         [FeatureAVR5]>;
def : Device<"atmega644p",         [FeatureAVR5]>;
def : Device<"atmega644pa",        [FeatureAVR5]>;
def : Device<"atmega645",          [FeatureAVR5]>;
def : Device<"atmega645a",         [FeatureAVR5]>;
def : Device<"atmega645p",         [FeatureAVR5]>;
def : Device<"atmega649",          [FeatureAVR5]>;
def : Device<"atmega649a",         [FeatureAVR5]>;
def : Device<"atmega649p",         [FeatureAVR5]>;
def : Device<"atmega6450",         [FeatureAVR5]>;
def : Device<"atmega6450a",        [FeatureAVR5]>;
def : Device<"atmega6450p",        [FeatureAVR5]>;
def : Device<"atmega6490",         [FeatureAVR5]>;
def : Device<"atmega6490a",        [FeatureAVR5]>;
def : Device<"atmega6490p",        [FeatureAVR5]>;
def : Device<"atmega64rfr2",       [FeatureAVR5]>;
def : Device<"atmega644rfr2",      [FeatureAVR5]>;
def : Device<"atmega16hva",        [FeatureAVR5]>;
def : Device<"atmega16hva2",       [FeatureAVR5]>;
def : Device<"atmega16hvb",        [FeatureAVR5]>;
def : Device<"atmega16hvbrevb",    [FeatureAVR5]>;
def : Device<"atmega32hvb",        [FeatureAVR5]>;
def : Device<"atmega32hvbrevb",    [FeatureAVR5]>;
def : Device<"atmega64hve",        [FeatureAVR5]>;
def : Device<"at90can32",          [FeatureAVR5]>;
def : Device<"at90can64",          [FeatureAVR5]>;
def : Device<"at90pwm161",         [FeatureAVR5]>;
def : Device<"at90pwm216",         [FeatureAVR5]>;
def : Device<"at90pwm316",         [FeatureAVR5]>;
def : Device<"atmega32c1",         [FeatureAVR5]>;
def : Device<"atmega64c1",         [FeatureAVR5]>;
def : Device<"atmega16m1",         [FeatureAVR5]>;
def : Device<"atmega32m1",         [FeatureAVR5]>;
def : Device<"atmega64m1",         [FeatureAVR5]>;
def : Device<"atmega16u4",         [FeatureAVR5]>;
def : Device<"atmega32u4",         [FeatureAVR5]>;
def : Device<"atmega32u6",         [FeatureAVR5]>;
def : Device<"at90usb646",         [FeatureAVR5]>;
def : Device<"at90usb647",         [FeatureAVR5]>;
def : Device<"at90scr100",         [FeatureAVR5]>;
def : Device<"at94k",              [FeatureAVR3, FeatureMultiplication, FeatureMOVW, FeatureLPMX]>;
def : Device<"m3000",              [FeatureAVR5]>;
def : Device<"atmega128",          [FeatureAVR51]>;
def : Device<"atmega128a",         [FeatureAVR51]>;
def : Device<"atmega1280",         [FeatureAVR51]>;
def : Device<"atmega1281",         [FeatureAVR51]>;
def : Device<"atmega1284",         [FeatureAVR51]>;
def : Device<"atmega1284p",        [FeatureAVR51]>;
def : Device<"atmega128rfa1",      [FeatureAVR51]>;
def : Device<"atmega128rfr2",      [FeatureAVR51]>;
def : Device<"atmega1284rfr2",     [FeatureAVR51]>;
def : Device<"at90can128",         [FeatureAVR51]>;
def : Device<"at90usb1286",        [FeatureAVR51]>;
def : Device<"at90usb1287",        [FeatureAVR51]>;
def : Device<"atmega2560",         [FeatureAVR6]>;
def : Device<"atmega2561",         [FeatureAVR6]>;
def : Device<"atmega256rfr2",      [FeatureAVR6]>;
def : Device<"atmega2564rfr2",     [FeatureAVR6]>;
def : Device<"atxmega16a4",        [FeatureXMEGA]>;
def : Device<"atxmega16a4u",       [FeatureXMEGAU]>;
def : Device<"atxmega16c4",        [FeatureXMEGAU]>;
def : Device<"atxmega16d4",        [FeatureXMEGA]>;
def : Device<"atxmega32a4",        [FeatureXMEGA]>;
def : Device<"atxmega32a4u",       [FeatureXMEGAU]>;
def : Device<"atxmega32c4",        [FeatureXMEGAU]>;
def : Device<"atxmega32d4",        [FeatureXMEGA]>;
def : Device<"atxmega32e5",        [FeatureXMEGA]>;
def : Device<"atxmega16e5",        [FeatureXMEGA]>;
def : Device<"atxmega8e5",         [FeatureXMEGA]>;
def : Device<"atxmega32x1",        [FeatureXMEGA]>;
def : Device<"atxmega64a3",        [FeatureXMEGA]>;
def : Device<"atxmega64a3u",       [FeatureXMEGAU]>;
def : Device<"atxmega64a4u",       [FeatureXMEGAU]>;
def : Device<"atxmega64b1",        [FeatureXMEGAU]>;
def : Device<"atxmega64b3",        [FeatureXMEGAU]>;
def : Device<"atxmega64c3",        [FeatureXMEGAU]>;
def : Device<"atxmega64d3",        [FeatureXMEGA]>;
def : Device<"atxmega64d4",        [FeatureXMEGA]>;
def : Device<"atxmega64a1",        [FeatureXMEGA]>;
def : Device<"atxmega64a1u",       [FeatureXMEGAU]>;
def : Device<"atxmega128a3",       [FeatureXMEGA]>;
def : Device<"atxmega128a3u",      [FeatureXMEGAU]>;
def : Device<"atxmega128b1",       [FeatureXMEGAU]>;
def : Device<"atxmega128b3",       [FeatureXMEGAU]>;
def : Device<"atxmega128c3",       [FeatureXMEGAU]>;
def : Device<"atxmega128d3",       [FeatureXMEGA]>;
def : Device<"atxmega128d4",       [FeatureXMEGA]>;
def : Device<"atxmega192a3",       [FeatureXMEGA]>;
def : Device<"atxmega192a3u",      [FeatureXMEGAU]>;
def : Device<"atxmega192c3",       [FeatureXMEGAU]>;
def : Device<"atxmega192d3",       [FeatureXMEGA]>;
def : Device<"atxmega256a3",       [FeatureXMEGA]>;
def : Device<"atxmega256a3u",      [FeatureXMEGAU]>;
def : Device<"atxmega256a3b",      [FeatureXMEGA]>;
def : Device<"atxmega256a3bu",     [FeatureXMEGAU]>;
def : Device<"atxmega256c3",       [FeatureXMEGAU]>;
def : Device<"atxmega256d3",       [FeatureXMEGA]>;
def : Device<"atxmega384c3",       [FeatureXMEGAU]>;
def : Device<"atxmega384d3",       [FeatureXMEGA]>;
def : Device<"atxmega128a1",       [FeatureXMEGA]>;
def : Device<"atxmega128a1u",      [FeatureXMEGAU]>;
def : Device<"atxmega128a4u",      [FeatureXMEGAU]>;
def : Device<"attiny4",            [FeatureAVRTiny]>;
def : Device<"attiny5",            [FeatureAVRTiny]>;
def : Device<"attiny9",            [FeatureAVRTiny]>;
def : Device<"attiny10",           [FeatureAVRTiny]>;
def : Device<"attiny20",           [FeatureAVRTiny]>;
def : Device<"attiny40",           [FeatureAVRTiny]>;

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
// Assembly Parsers
//===----------------------------------------------------------------------===//

def AVRAsmParser : AsmParser {
  let ShouldEmitMatchRegisterName = 0;
}

def AVRAsmParserVariant : AsmParserVariant {
  int Variant = 0;

  // Recognize hard coded registers.
  string RegisterPrefix = "$";
}

//===----------------------------------------------------------------------===//
// Target Declaration
//===----------------------------------------------------------------------===//

def AVR : Target
{
  let InstructionSet = AVRInstrInfo;
  let AssemblyWriters = [AVRAsmWriter];
  
  let AssemblyParsers = [AVRAsmParser];
  let AssemblyParserVariants = [AVRAsmParserVariant];
}
