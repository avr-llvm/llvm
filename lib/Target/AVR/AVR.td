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

def FeatureIMOVW        : SubtargetFeature<"movw", "HasMOVW", "true",
                                "Enable 'movw Rd:Rd+1, Rr:Rr+1' instruction">;

def FeatureIDES         : SubtargetFeature<"des", "HasDES", "true",
                                "Enable the 'des' instruction">;


def FeatureIBREAK       : SubtargetFeature<"break", "HasBREAK", "true",
                                "Enable 'break' instruction">;

// The device is self-programmable.
def FeatureISPM         : SubtargetFeature<"spm", "HasSPM", "true",
                                "Enable 'spm' instruction">;

def FeatureISPMX        : SubtargetFeature<"spmx", "HasSPMX", "true",
                                "Enable 'spm Z[+]' instruction">;

def FeatureRMW          : SubtargetFeature<"rmw", "SupportsRMW", "true",
                                "Enable xch,lac,las,lat instructions">;

def FeatureSRAM         : SubtargetFeature<"sram", "HasSRAM", "true",
                                "The device has SRAM">;

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

def Feature94K         : PseudoSubtargetFeature<"94k", "sadsf",
                            [FeatureM603,FeatureIMUL,
                             FeatureIMOVW,FeatureILPMX]>;

def FeatureM323         : PseudoSubtargetFeature<"m323", "sadsf",
                            [FeatureM161, FeatureIBREAK]>;

def FeatureM128         : PseudoSubtargetFeature<"m128", "sadsf",
                            [FeatureM323, FeatureIELPM, FeatureIELPMX]>;

def FeatureM256         : PseudoSubtargetFeature<"m256", "sadsf",
                            [FeatureM128, FeatureEIND]>;

def FeatureXMEGA        : PseudoSubtargetFeature<"xmega", "The device belongs to the XMEGA family",
                            [FeatureM256, FeatureISPMX, FeatureIDES]>;

def FeatureXMEGAU       : PseudoSubtargetFeature<"xmegau", "dsaf",
                            [FeatureXMEGA, FeatureRMW]>;

// Simple CPU core, only assembler support
def FeatureAVR1         : PseudoSubtargetFeature<"avr1", "The device supports AVR architecture 1",
                            [FeatureTiny1]>;

// "Classic" CPU core, up to 8 KB of ROM
def FeatureAVR2         : PseudoSubtargetFeature<"avr2", "The device supports AVR architecture 2",
                            [Feature2xxx]>;

// "Classic" CPU core with 'MOVW' and 'LPM Rx, Z[+]' instruction, up to 8 KB of ROM
def FeatureAVR25        : PseudoSubtargetFeature<"avr25", "The device supports AVR architecture 25",
                            [FeatureTiny2]>;

// "Classic" CPU core, 16 KB to 64 KB of ROM
def FeatureAVR3         : PseudoSubtargetFeature<"avr3", "The device supports AVR architecture 3",
                            [FeatureM603]>;

// "Classic" CPU core, 128 KB of ROM
def FeatureAVR31        : PseudoSubtargetFeature<"avr31", "The device supports AVR architecture 31",
                            [FeatureM103]>;

// "Classic" CPU core with 'MOVW' and 'LPM Rx, Z[+]' instruction, 16 KB to 64 KB of ROM
def FeatureAVR35        : PseudoSubtargetFeature<"avr35", "The device supports AVR architecture 35",
                            [FeatureAVR3, FeatureIMOVW, FeatureILPMX,
                             FeatureISPM, FeatureIBREAK]>;

def FeatureAVR3_ALL     : PseudoSubtargetFeature<"avr3_all", "The device supports AVR architecture 3,31 and 35",
                            [FeatureAVR3, FeatureAVR31, FeatureAVR35]>;

// "Enhanced" CPU core, up to 8 KB of ROM
def FeatureAVR4         : PseudoSubtargetFeature<"avr4", "The device supports AVR architecture 4",
                            [FeaturePWMx]>;

// "Enhanced" CPU core, 16 KB to 64 KB of ROM
def FeatureAVR5         : PseudoSubtargetFeature<"avr5", "The device supports AVR architecture 5",
                            [FeatureM323]>;

// "Enhanced" CPU core, 128 KB of ROM
def FeatureAVR51        : PseudoSubtargetFeature<"avr51", "The device supports AVR architecture 51",
                            [FeatureM128]>;

// "Enhanced" CPU core, 256 KB of ROM
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

// Generic MCUs
def : Device<"avr1",  [FeatureAVR1]>;
def : Device<"avr2",  [FeatureAVR2]>;
def : Device<"avr25", [FeatureAVR25]>;
def : Device<"avr3",  [FeatureAVR3_ALL]>;
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
def : Device<"at90s1200",          []>;
def : Device<"attiny11",           [FeatureAVR1]>;
def : Device<"attiny12",           [FeatureAVR1]>;
def : Device<"attiny15",           [FeatureAVR1]>;
def : Device<"attiny28",           [FeatureAVR1]>;
def : Device<"at90s2313",          [FeatureAVR2]>;
def : Device<"at90s2323",          [FeatureAVR2]>;
def : Device<"at90s2333",          [FeatureAVR2]>;
def : Device<"at90s2343",          [FeatureAVR2]>;
def : Device<"attiny22",           [FeatureAVR2]>;
def : Device<"attiny26",           [Feature2xxe]>;
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
def : Device<"at86rf401",          [FeatureRF401]>;
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
def : Device<"atmega8515",         [FeatureM8]>;
def : Device<"atmega8535",         [FeatureM8]>;
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
def : Device<"atmega161",          [FeatureM161]>;
def : Device<"atmega162",          [FeatureAVR5]>;
def : Device<"atmega163",          [FeatureM161]>;
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
def : Device<"at94k",              [Feature94K]>;
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
