//===-- AVRMCTargetDesc.h - AVR Target Descriptions -------------*- C++ -*-===//
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

#ifndef __INCLUDE_AVRMCTARGETDESC_H__
#define __INCLUDE_AVRMCTARGETDESC_H__

#include "llvm/Support/DataTypes.h"

namespace llvm
{
    class MCCodeEmitter;
    class MCAsmBackend;
    class MCRegisterInfo;
    class MCObjectWriter;
    class MCInstrInfo;
    class MCContext;
    class StringRef;
    class Target;
    class raw_ostream;

    extern Target TheAVRTarget;


    MCCodeEmitter *createAVRMCCodeEmitter(const MCInstrInfo &MCII,
                                          const MCRegisterInfo &MRI,
                                          MCContext &Ctx);
    /*!
     * \brief Creates a little endian AVR assembly backend.
     */
    MCAsmBackend *createAVRAsmBackendEL(const Target &T, const MCRegisterInfo &MRI,
                                        StringRef TT, StringRef CPU);
    
    /*!
     * \brief Creates a big endian AVR assembly backend.
     */
    MCAsmBackend *createAVRAsmBackendEB(const Target &T, const MCRegisterInfo &MRI,
                                        StringRef TT, StringRef CPU);
                                        
    MCObjectWriter *createAVRELFObjectWriter(raw_ostream &OS,
                                             uint8_t OSABI,
                                             bool IsLittleEndian);

} // end namespace llvm

// Defines symbolic names for AVR registers.  This defines a mapping from
// register name to register number.
#define GET_REGINFO_ENUM
#include "AVRGenRegisterInfo.inc"

// Defines symbolic names for the AVR instructions.
#define GET_INSTRINFO_ENUM
#include "AVRGenInstrInfo.inc"

#define GET_SUBTARGETINFO_ENUM
#include "AVRGenSubtargetInfo.inc"

#endif //__INCLUDE_AVRMCTARGETDESC_H__
