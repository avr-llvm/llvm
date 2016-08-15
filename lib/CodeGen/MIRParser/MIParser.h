//===- MIParser.h - Machine Instructions Parser ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the function that parses the machine instructions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_CODEGEN_MIRPARSER_MIPARSER_H
#define LLVM_LIB_CODEGEN_MIRPARSER_MIPARSER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"

namespace llvm {

class StringRef;
class BasicBlock;
class MachineBasicBlock;
class MachineFunction;
class MachineInstr;
class MachineRegisterInfo;
class MDNode;
struct SlotMapping;
class SMDiagnostic;
class SourceMgr;

struct PerFunctionMIParsingState {
  MachineFunction &MF;
  SourceMgr *SM;
  const SlotMapping &IRSlots;

  DenseMap<unsigned, MachineBasicBlock *> MBBSlots;
  DenseMap<unsigned, unsigned> VirtualRegisterSlots;
  DenseMap<unsigned, int> FixedStackObjectSlots;
  DenseMap<unsigned, int> StackObjectSlots;
  DenseMap<unsigned, unsigned> ConstantPoolSlots;
  DenseMap<unsigned, unsigned> JumpTableSlots;
  /// Hold the generic virtual registers.
  SmallSet<unsigned, 8> GenericVRegs;

  PerFunctionMIParsingState(MachineFunction &MF, SourceMgr &SM,
                            const SlotMapping &IRSlots);
};

/// Parse the machine basic block definitions, and skip the machine
/// instructions.
///
/// This function runs the first parsing pass on the machine function's body.
/// It parses only the machine basic block definitions and creates the machine
/// basic blocks in the given machine function.
///
/// The machine instructions aren't parsed during the first pass because all
/// the machine basic blocks aren't defined yet - this makes it impossible to
/// resolve the machine basic block references.
///
/// Return true if an error occurred.
bool parseMachineBasicBlockDefinitions(PerFunctionMIParsingState &PFS,
                                       StringRef Src, SMDiagnostic &Error);

/// Parse the machine instructions.
///
/// This function runs the second parsing pass on the machine function's body.
/// It skips the machine basic block definitions and parses only the machine
/// instructions and basic block attributes like liveins and successors.
///
/// The second parsing pass assumes that the first parsing pass already ran
/// on the given source string.
///
/// Return true if an error occurred.
bool parseMachineInstructions(const PerFunctionMIParsingState &PFS,
                              StringRef Src, SMDiagnostic &Error);

bool parseMBBReference(const PerFunctionMIParsingState &PFS,
                       MachineBasicBlock *&MBB, StringRef Src,
                       SMDiagnostic &Error);

bool parseNamedRegisterReference(const PerFunctionMIParsingState &PFS,
                                 unsigned &Reg, StringRef Src,
                                 SMDiagnostic &Error);

bool parseVirtualRegisterReference(const PerFunctionMIParsingState &PFS,
                                   unsigned &Reg, StringRef Src,
                                   SMDiagnostic &Error);

bool parseStackObjectReference(const PerFunctionMIParsingState &PFS,
                               int &FI, StringRef Src, SMDiagnostic &Error);

bool parseMDNode(const PerFunctionMIParsingState &PFS, MDNode *&Node,
                 StringRef Src, SMDiagnostic &Error);

} // end namespace llvm

#endif
