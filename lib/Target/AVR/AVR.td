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
// :TODO: We define all devices with SRAM to have all variants of LD/ST/LDD/STD.
//        In reality, avr1 (no SRAM) has one variant each of `LD` and `ST`.
//        avr2 (with SRAM) adds the rest of the variants.


// A feature set aggregates features, grouping them. We don't want to create a
// new member in AVRSubtarget (to store a value) for each set because we do not
// care if the set is supported, only the subfeatures inside the set. We fix
// this by simply setting the same dummy member for all feature sets, which is
// then ignored.
class FeatureSet<string name, string desc, list<SubtargetFeature> i>
  : SubtargetFeature<name, "m_FeatureSetDummy", "true", desc, i> { }

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


//===----------------------------------------------------------------------===//
// AVR Families
//===----------------------------------------------------------------------===//

// The device has at least the bare minimum that **every** single AVR device should have.
def FamilyAVR0           : FeatureSet<"avr0", "The device is an AVR", []>;

def FamilyAVR1           : FeatureSet<"avr1", "The device is a part of the avr1 family",
                                 [FamilyAVR0, FeatureLPM]>;

def FamilyAVR2           : FeatureSet<"avr2", "The device is a part of the avr2 family",
                                 [FamilyAVR1, FeatureIJMPCALL, FeatureADDSUBIW, FeatureSRAM]>;

def FamilyAVR25          : FeatureSet<"avr25", "The device is a part of the avr25 family",
                                 [FamilyAVR2, FeatureMOVW, FeatureLPMX, FeatureSPM, FeatureBREAK]>;

def FamilyAVR3           : FeatureSet<"avr3", "The device is a part of the avr3 family",
                                 [FamilyAVR2, FeatureJMPCALL]>;

def FamilyAVR31          : FeatureSet<"avr31", "The device is a part of the avr31 family",
                                 [FamilyAVR3, FeatureELPM]>;

def FamilyAVR35          : FeatureSet<"avr35", "The device is a part of the avr35 family",
                                 [FamilyAVR3, FeatureMOVW, FeatureLPMX, FeatureSPM, FeatureBREAK]>;

def FamilyAVR4           : FeatureSet<"avr4", "The device is a part of the avr4 family",
                                 [FamilyAVR2, FeatureMultiplication, FeatureMOVW,
                                  FeatureLPMX, FeatureSPM,  FeatureBREAK]>;

def FamilyAVR5           : FeatureSet<"avr5", "The device is a part of the avr5 family",
                                 [FamilyAVR3, FeatureMultiplication, FeatureMOVW,
                                  FeatureLPMX, FeatureSPM, FeatureBREAK]>;

def FamilyAVR51          : FeatureSet<"avr51", "The device is part of the avr52 family",
                                 [FamilyAVR5, FeatureELPM, FeatureELPMX]>;

def FamilyAVR6           : FeatureSet<"avr6", "The device is part of the avr6 family",
                                 [FamilyAVR0, FeatureLPM, FeatureLPMX, FeatureSRAM,
                                  FeatureJMPCALL, FeatureMultiplication, FeatureELPM,
                                  FeatureELPMX, FeatureSPM, FeatureBREAK, FeatureEIJMPCALL, FeatureMOVW]>;

def FamilyAVRTiny        : FeatureSet<"avrtiny", "The device is a part of the tiny family",
                                 [FamilyAVR0, FeatureBREAK, FeatureSRAM, FeatureTinyEncoding]>;

def FamilyXMEGA          : FeatureSet<"xmega", "The device is a part of the XMEGA family",
                                 [FamilyAVR51, FeatureEIJMPCALL, FeatureSPMX, FeatureDES]>;

def FamilyXMEGAU         : FeatureSet<"xmegau", "The device is part of the XMEGAU family",
                                  [FamilyXMEGA, FeatureRMW]>;

//===----------------------------------------------------------------------===//
// AVR microcontrollers supported.
//===----------------------------------------------------------------------===//

class Device<string Name, list<SubtargetFeature> Features>
  : Processor<Name, NoItineraries, Features>;

// Generic MCUs
def : Device<"avr1",  [FamilyAVR1]>;
def : Device<"avr2",  [FamilyAVR2]>;
def : Device<"avr25", [FamilyAVR25]>;
def : Device<"avr3",  [FamilyAVR3]>;
def : Device<"avr31", [FamilyAVR31]>;
def : Device<"avr35", [FamilyAVR35]>;
def : Device<"avr4",  [FamilyAVR4]>;
def : Device<"avr5",  [FamilyAVR5]>;
def : Device<"avr51", [FamilyAVR51]>;
def : Device<"avr6",  [FamilyAVR6]>;

// Specific MCUs
def : Device<"avrxmega1",          [FamilyXMEGA]>;
def : Device<"avrxmega2",          [FamilyXMEGA]>;
def : Device<"avrxmega3",          [FamilyXMEGA]>;
def : Device<"avrxmega4",          [FamilyXMEGA]>;
def : Device<"avrxmega5",          [FamilyXMEGA]>;
def : Device<"avrxmega6",          [FamilyXMEGA]>;
def : Device<"avrxmega7",          [FamilyXMEGA]>;
def : Device<"avrtiny",            [FamilyAVRTiny]>;
def : Device<"at90s1200",          [FamilyAVR0]>;
def : Device<"attiny11",           [FamilyAVR1]>;
def : Device<"attiny12",           [FamilyAVR1]>;
def : Device<"attiny15",           [FamilyAVR1]>;
def : Device<"attiny28",           [FamilyAVR1]>;
def : Device<"at90s2313",          [FamilyAVR2]>;
def : Device<"at90s2323",          [FamilyAVR2]>;
def : Device<"at90s2333",          [FamilyAVR2]>;
def : Device<"at90s2343",          [FamilyAVR2]>;
def : Device<"attiny22",           [FamilyAVR2]>;
def : Device<"attiny26",           [FamilyAVR2, FeatureLPMX]>;
def : Device<"at86rf401",          [FamilyAVR2, FeatureMOVW, FeatureLPMX]>;
def : Device<"at90s4414",          [FamilyAVR2]>;
def : Device<"at90s4433",          [FamilyAVR2]>;
def : Device<"at90s4434",          [FamilyAVR2]>;
def : Device<"at90s8515",          [FamilyAVR2]>;
def : Device<"at90c8534",          [FamilyAVR2]>;
def : Device<"at90s8535",          [FamilyAVR2]>;
def : Device<"ata5272",            [FamilyAVR25]>;
def : Device<"attiny13",           [FamilyAVR25]>;
def : Device<"attiny13a",          [FamilyAVR25]>;
def : Device<"attiny2313",         [FamilyAVR25]>;
def : Device<"attiny2313a",        [FamilyAVR25]>;
def : Device<"attiny24",           [FamilyAVR25]>;
def : Device<"attiny24a",          [FamilyAVR25]>;
def : Device<"attiny4313",         [FamilyAVR25]>;
def : Device<"attiny44",           [FamilyAVR25]>;
def : Device<"attiny44a",          [FamilyAVR25]>;
def : Device<"attiny84",           [FamilyAVR25]>;
def : Device<"attiny84a",          [FamilyAVR25]>;
def : Device<"attiny25",           [FamilyAVR25]>;
def : Device<"attiny45",           [FamilyAVR25]>;
def : Device<"attiny85",           [FamilyAVR25]>;
def : Device<"attiny261",          [FamilyAVR25]>;
def : Device<"attiny261a",         [FamilyAVR25]>;
def : Device<"attiny461",          [FamilyAVR25]>;
def : Device<"attiny461a",         [FamilyAVR25]>;
def : Device<"attiny861",          [FamilyAVR25]>;
def : Device<"attiny861a",         [FamilyAVR25]>;
def : Device<"attiny87",           [FamilyAVR25]>;
def : Device<"attiny43u",          [FamilyAVR25]>;
def : Device<"attiny48",           [FamilyAVR25]>;
def : Device<"attiny88",           [FamilyAVR25]>;
def : Device<"attiny828",          [FamilyAVR25]>;
def : Device<"at43usb355",         [FamilyAVR3]>;
def : Device<"at76c711",           [FamilyAVR3]>;
def : Device<"atmega103",          [FamilyAVR31]>;
def : Device<"at43usb320",         [FamilyAVR31]>;
def : Device<"attiny167",          [FamilyAVR35]>;
def : Device<"at90usb82",          [FamilyAVR35]>;
def : Device<"at90usb162",         [FamilyAVR35]>;
def : Device<"ata5505",            [FamilyAVR35]>;
def : Device<"atmega8u2",          [FamilyAVR35]>;
def : Device<"atmega16u2",         [FamilyAVR35]>;
def : Device<"atmega32u2",         [FamilyAVR35]>;
def : Device<"attiny1634",         [FamilyAVR35]>;
def : Device<"atmega8",            [FamilyAVR4]>;
def : Device<"ata6289",            [FamilyAVR4]>;
def : Device<"atmega8a",           [FamilyAVR4]>;
def : Device<"ata6285",            [FamilyAVR4]>;
def : Device<"ata6286",            [FamilyAVR4]>;
def : Device<"atmega48",           [FamilyAVR4]>;
def : Device<"atmega48a",          [FamilyAVR4]>;
def : Device<"atmega48pa",         [FamilyAVR4]>;
def : Device<"atmega48p",          [FamilyAVR4]>;
def : Device<"atmega88",           [FamilyAVR4]>;
def : Device<"atmega88a",          [FamilyAVR4]>;
def : Device<"atmega88p",          [FamilyAVR4]>;
def : Device<"atmega88pa",         [FamilyAVR4]>;
def : Device<"atmega8515",         [FamilyAVR2, FeatureMultiplication, FeatureMOVW, FeatureLPMX, FeatureSPM]>;
def : Device<"atmega8535",         [FamilyAVR2, FeatureMultiplication, FeatureMOVW, FeatureLPMX, FeatureSPM]>;
def : Device<"atmega8hva",         [FamilyAVR4]>;
def : Device<"at90pwm1",           [FamilyAVR4]>;
def : Device<"at90pwm2",           [FamilyAVR4]>;
def : Device<"at90pwm2b",          [FamilyAVR4]>;
def : Device<"at90pwm3",           [FamilyAVR4]>;
def : Device<"at90pwm3b",          [FamilyAVR4]>;
def : Device<"at90pwm81",          [FamilyAVR4]>;
def : Device<"ata5790",            [FamilyAVR5]>;
def : Device<"ata5795",            [FamilyAVR5]>;
def : Device<"atmega16",           [FamilyAVR5]>;
def : Device<"atmega16a",          [FamilyAVR5]>;
def : Device<"atmega161",          [FamilyAVR3, FeatureMultiplication, FeatureMOVW, FeatureLPMX, FeatureSPM]>;
def : Device<"atmega162",          [FamilyAVR5]>;
def : Device<"atmega163",          [FamilyAVR3, FeatureMultiplication, FeatureMOVW, FeatureLPMX, FeatureSPM]>;
def : Device<"atmega164a",         [FamilyAVR5]>;
def : Device<"atmega164p",         [FamilyAVR5]>;
def : Device<"atmega164pa",        [FamilyAVR5]>;
def : Device<"atmega165",          [FamilyAVR5]>;
def : Device<"atmega165a",         [FamilyAVR5]>;
def : Device<"atmega165p",         [FamilyAVR5]>;
def : Device<"atmega165pa",        [FamilyAVR5]>;
def : Device<"atmega168",          [FamilyAVR5]>;
def : Device<"atmega168a",         [FamilyAVR5]>;
def : Device<"atmega168p",         [FamilyAVR5]>;
def : Device<"atmega168pa",        [FamilyAVR5]>;
def : Device<"atmega169",          [FamilyAVR5]>;
def : Device<"atmega169a",         [FamilyAVR5]>;
def : Device<"atmega169p",         [FamilyAVR5]>;
def : Device<"atmega169pa",        [FamilyAVR5]>;
def : Device<"atmega32",           [FamilyAVR5]>;
def : Device<"atmega32a",          [FamilyAVR5]>;
def : Device<"atmega323",          [FamilyAVR5]>;
def : Device<"atmega324a",         [FamilyAVR5]>;
def : Device<"atmega324p",         [FamilyAVR5]>;
def : Device<"atmega324pa",        [FamilyAVR5]>;
def : Device<"atmega325",          [FamilyAVR5]>;
def : Device<"atmega325a",         [FamilyAVR5]>;
def : Device<"atmega325p",         [FamilyAVR5]>;
def : Device<"atmega325pa",        [FamilyAVR5]>;
def : Device<"atmega3250",         [FamilyAVR5]>;
def : Device<"atmega3250a",        [FamilyAVR5]>;
def : Device<"atmega3250p",        [FamilyAVR5]>;
def : Device<"atmega3250pa",       [FamilyAVR5]>;
def : Device<"atmega328",          [FamilyAVR5]>;
def : Device<"atmega328p",         [FamilyAVR5]>;
def : Device<"atmega329",          [FamilyAVR5]>;
def : Device<"atmega329a",         [FamilyAVR5]>;
def : Device<"atmega329p",         [FamilyAVR5]>;
def : Device<"atmega329pa",        [FamilyAVR5]>;
def : Device<"atmega3290",         [FamilyAVR5]>;
def : Device<"atmega3290a",        [FamilyAVR5]>;
def : Device<"atmega3290p",        [FamilyAVR5]>;
def : Device<"atmega3290pa",       [FamilyAVR5]>;
def : Device<"atmega406",          [FamilyAVR5]>;
def : Device<"atmega64",           [FamilyAVR5]>;
def : Device<"atmega64a",          [FamilyAVR5]>;
def : Device<"atmega640",          [FamilyAVR5]>;
def : Device<"atmega644",          [FamilyAVR5]>;
def : Device<"atmega644a",         [FamilyAVR5]>;
def : Device<"atmega644p",         [FamilyAVR5]>;
def : Device<"atmega644pa",        [FamilyAVR5]>;
def : Device<"atmega645",          [FamilyAVR5]>;
def : Device<"atmega645a",         [FamilyAVR5]>;
def : Device<"atmega645p",         [FamilyAVR5]>;
def : Device<"atmega649",          [FamilyAVR5]>;
def : Device<"atmega649a",         [FamilyAVR5]>;
def : Device<"atmega649p",         [FamilyAVR5]>;
def : Device<"atmega6450",         [FamilyAVR5]>;
def : Device<"atmega6450a",        [FamilyAVR5]>;
def : Device<"atmega6450p",        [FamilyAVR5]>;
def : Device<"atmega6490",         [FamilyAVR5]>;
def : Device<"atmega6490a",        [FamilyAVR5]>;
def : Device<"atmega6490p",        [FamilyAVR5]>;
def : Device<"atmega64rfr2",       [FamilyAVR5]>;
def : Device<"atmega644rfr2",      [FamilyAVR5]>;
def : Device<"atmega16hva",        [FamilyAVR5]>;
def : Device<"atmega16hva2",       [FamilyAVR5]>;
def : Device<"atmega16hvb",        [FamilyAVR5]>;
def : Device<"atmega16hvbrevb",    [FamilyAVR5]>;
def : Device<"atmega32hvb",        [FamilyAVR5]>;
def : Device<"atmega32hvbrevb",    [FamilyAVR5]>;
def : Device<"atmega64hve",        [FamilyAVR5]>;
def : Device<"at90can32",          [FamilyAVR5]>;
def : Device<"at90can64",          [FamilyAVR5]>;
def : Device<"at90pwm161",         [FamilyAVR5]>;
def : Device<"at90pwm216",         [FamilyAVR5]>;
def : Device<"at90pwm316",         [FamilyAVR5]>;
def : Device<"atmega32c1",         [FamilyAVR5]>;
def : Device<"atmega64c1",         [FamilyAVR5]>;
def : Device<"atmega16m1",         [FamilyAVR5]>;
def : Device<"atmega32m1",         [FamilyAVR5]>;
def : Device<"atmega64m1",         [FamilyAVR5]>;
def : Device<"atmega16u4",         [FamilyAVR5]>;
def : Device<"atmega32u4",         [FamilyAVR5]>;
def : Device<"atmega32u6",         [FamilyAVR5]>;
def : Device<"at90usb646",         [FamilyAVR5]>;
def : Device<"at90usb647",         [FamilyAVR5]>;
def : Device<"at90scr100",         [FamilyAVR5]>;
def : Device<"at94k",              [FamilyAVR3, FeatureMultiplication, FeatureMOVW, FeatureLPMX]>;
def : Device<"m3000",              [FamilyAVR5]>;
def : Device<"atmega128",          [FamilyAVR51]>;
def : Device<"atmega128a",         [FamilyAVR51]>;
def : Device<"atmega1280",         [FamilyAVR51]>;
def : Device<"atmega1281",         [FamilyAVR51]>;
def : Device<"atmega1284",         [FamilyAVR51]>;
def : Device<"atmega1284p",        [FamilyAVR51]>;
def : Device<"atmega128rfa1",      [FamilyAVR51]>;
def : Device<"atmega128rfr2",      [FamilyAVR51]>;
def : Device<"atmega1284rfr2",     [FamilyAVR51]>;
def : Device<"at90can128",         [FamilyAVR51]>;
def : Device<"at90usb1286",        [FamilyAVR51]>;
def : Device<"at90usb1287",        [FamilyAVR51]>;
def : Device<"atmega2560",         [FamilyAVR6]>;
def : Device<"atmega2561",         [FamilyAVR6]>;
def : Device<"atmega256rfr2",      [FamilyAVR6]>;
def : Device<"atmega2564rfr2",     [FamilyAVR6]>;
def : Device<"atxmega16a4",        [FamilyXMEGA]>;
def : Device<"atxmega16a4u",       [FamilyXMEGAU]>;
def : Device<"atxmega16c4",        [FamilyXMEGAU]>;
def : Device<"atxmega16d4",        [FamilyXMEGA]>;
def : Device<"atxmega32a4",        [FamilyXMEGA]>;
def : Device<"atxmega32a4u",       [FamilyXMEGAU]>;
def : Device<"atxmega32c4",        [FamilyXMEGAU]>;
def : Device<"atxmega32d4",        [FamilyXMEGA]>;
def : Device<"atxmega32e5",        [FamilyXMEGA]>;
def : Device<"atxmega16e5",        [FamilyXMEGA]>;
def : Device<"atxmega8e5",         [FamilyXMEGA]>;
def : Device<"atxmega32x1",        [FamilyXMEGA]>;
def : Device<"atxmega64a3",        [FamilyXMEGA]>;
def : Device<"atxmega64a3u",       [FamilyXMEGAU]>;
def : Device<"atxmega64a4u",       [FamilyXMEGAU]>;
def : Device<"atxmega64b1",        [FamilyXMEGAU]>;
def : Device<"atxmega64b3",        [FamilyXMEGAU]>;
def : Device<"atxmega64c3",        [FamilyXMEGAU]>;
def : Device<"atxmega64d3",        [FamilyXMEGA]>;
def : Device<"atxmega64d4",        [FamilyXMEGA]>;
def : Device<"atxmega64a1",        [FamilyXMEGA]>;
def : Device<"atxmega64a1u",       [FamilyXMEGAU]>;
def : Device<"atxmega128a3",       [FamilyXMEGA]>;
def : Device<"atxmega128a3u",      [FamilyXMEGAU]>;
def : Device<"atxmega128b1",       [FamilyXMEGAU]>;
def : Device<"atxmega128b3",       [FamilyXMEGAU]>;
def : Device<"atxmega128c3",       [FamilyXMEGAU]>;
def : Device<"atxmega128d3",       [FamilyXMEGA]>;
def : Device<"atxmega128d4",       [FamilyXMEGA]>;
def : Device<"atxmega192a3",       [FamilyXMEGA]>;
def : Device<"atxmega192a3u",      [FamilyXMEGAU]>;
def : Device<"atxmega192c3",       [FamilyXMEGAU]>;
def : Device<"atxmega192d3",       [FamilyXMEGA]>;
def : Device<"atxmega256a3",       [FamilyXMEGA]>;
def : Device<"atxmega256a3u",      [FamilyXMEGAU]>;
def : Device<"atxmega256a3b",      [FamilyXMEGA]>;
def : Device<"atxmega256a3bu",     [FamilyXMEGAU]>;
def : Device<"atxmega256c3",       [FamilyXMEGAU]>;
def : Device<"atxmega256d3",       [FamilyXMEGA]>;
def : Device<"atxmega384c3",       [FamilyXMEGAU]>;
def : Device<"atxmega384d3",       [FamilyXMEGA]>;
def : Device<"atxmega128a1",       [FamilyXMEGA]>;
def : Device<"atxmega128a1u",      [FamilyXMEGAU]>;
def : Device<"atxmega128a4u",      [FamilyXMEGAU]>;
def : Device<"attiny4",            [FamilyAVRTiny]>;
def : Device<"attiny5",            [FamilyAVRTiny]>;
def : Device<"attiny9",            [FamilyAVRTiny]>;
def : Device<"attiny10",           [FamilyAVRTiny]>;
def : Device<"attiny20",           [FamilyAVRTiny]>;
def : Device<"attiny40",           [FamilyAVRTiny]>;

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

def AVRAsmWriter : AsmWriter {
  string AsmWriterClassName = "InstPrinter";
  bit isMCAsmWriter = 1;
}

//===----------------------------------------------------------------------===//
// Assembly Parsers
//===----------------------------------------------------------------------===//

def AVRAsmParser : AsmParser {
  let ShouldEmitMatchRegisterName = 1;
  let ShouldEmitMatchRegisterAltName = 1;
}

def AVRAsmParserVariant : AsmParserVariant {
  int Variant = 0;

  // Recognize hard coded registers.
  string RegisterPrefix = "$";
}

//===----------------------------------------------------------------------===//
// Target Declaration
//===----------------------------------------------------------------------===//

def AVR : Target {
  let InstructionSet         = AVRInstrInfo;
  let AssemblyWriters        = [AVRAsmWriter];
  
  let AssemblyParsers        = [AVRAsmParser];
  let AssemblyParserVariants = [AVRAsmParserVariant];
}
