//===-- AVRMCTargetDesc.cpp - AVR Target Descriptions ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides AVR specific target descriptions.
//
//===----------------------------------------------------------------------===//

#include "AVRMCTargetDesc.h"
#include "AVRMCAsmInfo.h"
#include "InstPrinter/AVRInstPrinter.h"
#include "llvm/MC/MCCodeGenInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCELFStreamer.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "AVRGenInstrInfo.inc"

#define GET_SUBTARGETINFO_MC_DESC
#include "AVRGenSubtargetInfo.inc"

#define GET_REGINFO_MC_DESC
#include "AVRGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createAVRMCInstrInfo()
{
  MCInstrInfo *X = new MCInstrInfo();
  InitAVRMCInstrInfo(X);

  return X;
}

static MCRegisterInfo *createAVRMCRegisterInfo(StringRef TT)
{
  MCRegisterInfo *X = new MCRegisterInfo();
  InitAVRMCRegisterInfo(X, 0);

  return X;
}

static MCSubtargetInfo *createAVRMCSubtargetInfo(StringRef TT, StringRef CPU,
                                                 StringRef FS)
{
  MCSubtargetInfo *X = new MCSubtargetInfo();
  InitAVRMCSubtargetInfo(X, TT, CPU, FS);

  return X;
}

static MCCodeGenInfo *createAVRMCCodeGenInfo(StringRef TT, Reloc::Model RM,
                                             CodeModel::Model CM,
                                             CodeGenOpt::Level OL)
{
  MCCodeGenInfo *X = new MCCodeGenInfo();
  X->InitMCCodeGenInfo(RM, CM, OL);

  return X;
}

static MCInstPrinter *createAVRMCInstPrinter(const Target &T,
                                             unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI,
                                             const MCInstrInfo &MII,
                                             const MCRegisterInfo &MRI,
                                             const MCSubtargetInfo &STI)
{
  if (SyntaxVariant == 0)
  {
    return new AVRInstPrinter(MAI, MII, MRI);
  }

  return 0;
}

static MCStreamer *createMCStreamer(const Target &T, StringRef TT,
                                    MCContext &Context, MCAsmBackend &MAB,
                                    raw_ostream &OS, MCCodeEmitter *Emitter,
                                    const MCSubtargetInfo &STI,
                                    bool RelaxAll) {
  return createELFStreamer(Context, MAB, OS, Emitter, RelaxAll);
}

static MCStreamer *
createMCAsmStreamer(MCContext &Ctx, formatted_raw_ostream &OS,
                    bool isVerboseAsm,
                    bool useDwarfDirectory, MCInstPrinter *InstPrint,
                    MCCodeEmitter *CE, MCAsmBackend *TAB, bool ShowInst) {
  return llvm::createAsmStreamer(Ctx, OS, isVerboseAsm,
                                 useDwarfDirectory, InstPrint, CE, TAB,
                                 ShowInst);
}

extern "C" void LLVMInitializeAVRTargetMC()
{
  // Register the MC asm info.
  RegisterMCAsmInfo<AVRMCAsmInfo> X(TheAVRTarget);

  // Register the MC codegen info.
  TargetRegistry::RegisterMCCodeGenInfo(TheAVRTarget, createAVRMCCodeGenInfo);

  // Register the MC instruction info.
  TargetRegistry::RegisterMCInstrInfo(TheAVRTarget, createAVRMCInstrInfo);

  // Register the MC register info.
  TargetRegistry::RegisterMCRegInfo(TheAVRTarget, createAVRMCRegisterInfo);

  // Register the MC subtarget info.
  TargetRegistry::RegisterMCSubtargetInfo(TheAVRTarget,
                                          createAVRMCSubtargetInfo);

  // Register the MCInstPrinter.
  TargetRegistry::RegisterMCInstPrinter(TheAVRTarget, createAVRMCInstPrinter);
  
  // Register the MC Code Emitter
  TargetRegistry::RegisterMCCodeEmitter(TheAVRTarget,
                                        createAVRMCCodeEmitter);

  // Register the object streamer.
  TargetRegistry::RegisterMCObjectStreamer(TheAVRTarget, createMCStreamer);

  // Register the asm streamer.
  TargetRegistry::RegisterAsmStreamer(TheAVRTarget, createMCAsmStreamer);

  // Register the asm backend (as little endian).
  TargetRegistry::RegisterMCAsmBackend(TheAVRTarget,
                                       createAVRAsmBackendEL);
}
