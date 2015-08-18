//===- MIRPrinter.cpp - MIR serialization format printer ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the class that prints out the LLVM IR and machine
// functions using the MIR serialization format.
//
//===----------------------------------------------------------------------===//

#include "MIRPrinter.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/MIRYamlMapping.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetSubtargetInfo.h"

using namespace llvm;

namespace {

/// This structure describes how to print out stack object references.
struct FrameIndexOperand {
  std::string Name;
  unsigned ID;
  bool IsFixed;

  FrameIndexOperand(StringRef Name, unsigned ID, bool IsFixed)
      : Name(Name.str()), ID(ID), IsFixed(IsFixed) {}

  /// Return an ordinary stack object reference.
  static FrameIndexOperand create(StringRef Name, unsigned ID) {
    return FrameIndexOperand(Name, ID, /*IsFixed=*/false);
  }

  /// Return a fixed stack object reference.
  static FrameIndexOperand createFixed(unsigned ID) {
    return FrameIndexOperand("", ID, /*IsFixed=*/true);
  }
};

} // end anonymous namespace

namespace llvm {

/// This class prints out the machine functions using the MIR serialization
/// format.
class MIRPrinter {
  raw_ostream &OS;
  DenseMap<const uint32_t *, unsigned> RegisterMaskIds;
  /// Maps from stack object indices to operand indices which will be used when
  /// printing frame index machine operands.
  DenseMap<int, FrameIndexOperand> StackObjectOperandMapping;

public:
  MIRPrinter(raw_ostream &OS) : OS(OS) {}

  void print(const MachineFunction &MF);

  void convert(yaml::MachineFunction &MF, const MachineRegisterInfo &RegInfo,
               const TargetRegisterInfo *TRI);
  void convert(ModuleSlotTracker &MST, yaml::MachineFrameInfo &YamlMFI,
               const MachineFrameInfo &MFI);
  void convert(yaml::MachineFunction &MF,
               const MachineConstantPool &ConstantPool);
  void convert(ModuleSlotTracker &MST, yaml::MachineJumpTable &YamlJTI,
               const MachineJumpTableInfo &JTI);
  void convertStackObjects(yaml::MachineFunction &MF,
                           const MachineFrameInfo &MFI,
                           const TargetRegisterInfo *TRI);

private:
  void initRegisterMaskIds(const MachineFunction &MF);
};

/// This class prints out the machine instructions using the MIR serialization
/// format.
class MIPrinter {
  raw_ostream &OS;
  ModuleSlotTracker &MST;
  const DenseMap<const uint32_t *, unsigned> &RegisterMaskIds;
  const DenseMap<int, FrameIndexOperand> &StackObjectOperandMapping;

public:
  MIPrinter(raw_ostream &OS, ModuleSlotTracker &MST,
            const DenseMap<const uint32_t *, unsigned> &RegisterMaskIds,
            const DenseMap<int, FrameIndexOperand> &StackObjectOperandMapping)
      : OS(OS), MST(MST), RegisterMaskIds(RegisterMaskIds),
        StackObjectOperandMapping(StackObjectOperandMapping) {}

  void print(const MachineBasicBlock &MBB);

  void print(const MachineInstr &MI);
  void printMBBReference(const MachineBasicBlock &MBB);
  void printIRBlockReference(const BasicBlock &BB);
  void printIRValueReference(const Value &V);
  void printStackObjectReference(int FrameIndex);
  void printOffset(int64_t Offset);
  void printTargetFlags(const MachineOperand &Op);
  void print(const MachineOperand &Op, const TargetRegisterInfo *TRI);
  void print(const MachineMemOperand &Op);

  void print(const MCCFIInstruction &CFI, const TargetRegisterInfo *TRI);
};

} // end namespace llvm

namespace llvm {
namespace yaml {

/// This struct serializes the LLVM IR module.
template <> struct BlockScalarTraits<Module> {
  static void output(const Module &Mod, void *Ctxt, raw_ostream &OS) {
    Mod.print(OS, nullptr);
  }
  static StringRef input(StringRef Str, void *Ctxt, Module &Mod) {
    llvm_unreachable("LLVM Module is supposed to be parsed separately");
    return "";
  }
};

} // end namespace yaml
} // end namespace llvm

static void printReg(unsigned Reg, raw_ostream &OS,
                     const TargetRegisterInfo *TRI) {
  // TODO: Print Stack Slots.
  if (!Reg)
    OS << '_';
  else if (TargetRegisterInfo::isVirtualRegister(Reg))
    OS << '%' << TargetRegisterInfo::virtReg2Index(Reg);
  else if (Reg < TRI->getNumRegs())
    OS << '%' << StringRef(TRI->getName(Reg)).lower();
  else
    llvm_unreachable("Can't print this kind of register yet");
}

static void printReg(unsigned Reg, yaml::StringValue &Dest,
                     const TargetRegisterInfo *TRI) {
  raw_string_ostream OS(Dest.Value);
  printReg(Reg, OS, TRI);
}

void MIRPrinter::print(const MachineFunction &MF) {
  initRegisterMaskIds(MF);

  yaml::MachineFunction YamlMF;
  YamlMF.Name = MF.getName();
  YamlMF.Alignment = MF.getAlignment();
  YamlMF.ExposesReturnsTwice = MF.exposesReturnsTwice();
  YamlMF.HasInlineAsm = MF.hasInlineAsm();
  convert(YamlMF, MF.getRegInfo(), MF.getSubtarget().getRegisterInfo());
  ModuleSlotTracker MST(MF.getFunction()->getParent());
  MST.incorporateFunction(*MF.getFunction());
  convert(MST, YamlMF.FrameInfo, *MF.getFrameInfo());
  convertStackObjects(YamlMF, *MF.getFrameInfo(),
                      MF.getSubtarget().getRegisterInfo());
  if (const auto *ConstantPool = MF.getConstantPool())
    convert(YamlMF, *ConstantPool);
  if (const auto *JumpTableInfo = MF.getJumpTableInfo())
    convert(MST, YamlMF.JumpTableInfo, *JumpTableInfo);
  raw_string_ostream StrOS(YamlMF.Body.Value.Value);
  bool IsNewlineNeeded = false;
  for (const auto &MBB : MF) {
    if (IsNewlineNeeded)
      StrOS << "\n";
    MIPrinter(StrOS, MST, RegisterMaskIds, StackObjectOperandMapping)
        .print(MBB);
    IsNewlineNeeded = true;
  }
  StrOS.flush();
  yaml::Output Out(OS);
  Out << YamlMF;
}

void MIRPrinter::convert(yaml::MachineFunction &MF,
                         const MachineRegisterInfo &RegInfo,
                         const TargetRegisterInfo *TRI) {
  MF.IsSSA = RegInfo.isSSA();
  MF.TracksRegLiveness = RegInfo.tracksLiveness();
  MF.TracksSubRegLiveness = RegInfo.subRegLivenessEnabled();

  // Print the virtual register definitions.
  for (unsigned I = 0, E = RegInfo.getNumVirtRegs(); I < E; ++I) {
    unsigned Reg = TargetRegisterInfo::index2VirtReg(I);
    yaml::VirtualRegisterDefinition VReg;
    VReg.ID = I;
    VReg.Class =
        StringRef(TRI->getRegClassName(RegInfo.getRegClass(Reg))).lower();
    unsigned PreferredReg = RegInfo.getSimpleHint(Reg);
    if (PreferredReg)
      printReg(PreferredReg, VReg.PreferredRegister, TRI);
    MF.VirtualRegisters.push_back(VReg);
  }

  // Print the live ins.
  for (auto I = RegInfo.livein_begin(), E = RegInfo.livein_end(); I != E; ++I) {
    yaml::MachineFunctionLiveIn LiveIn;
    printReg(I->first, LiveIn.Register, TRI);
    if (I->second)
      printReg(I->second, LiveIn.VirtualRegister, TRI);
    MF.LiveIns.push_back(LiveIn);
  }
  // The used physical register mask is printed as an inverted callee saved
  // register mask.
  const BitVector &UsedPhysRegMask = RegInfo.getUsedPhysRegsMask();
  if (UsedPhysRegMask.none())
    return;
  std::vector<yaml::FlowStringValue> CalleeSavedRegisters;
  for (unsigned I = 0, E = UsedPhysRegMask.size(); I != E; ++I) {
    if (!UsedPhysRegMask[I]) {
      yaml::FlowStringValue Reg;
      printReg(I, Reg, TRI);
      CalleeSavedRegisters.push_back(Reg);
    }
  }
  MF.CalleeSavedRegisters = CalleeSavedRegisters;
}

void MIRPrinter::convert(ModuleSlotTracker &MST,
                         yaml::MachineFrameInfo &YamlMFI,
                         const MachineFrameInfo &MFI) {
  YamlMFI.IsFrameAddressTaken = MFI.isFrameAddressTaken();
  YamlMFI.IsReturnAddressTaken = MFI.isReturnAddressTaken();
  YamlMFI.HasStackMap = MFI.hasStackMap();
  YamlMFI.HasPatchPoint = MFI.hasPatchPoint();
  YamlMFI.StackSize = MFI.getStackSize();
  YamlMFI.OffsetAdjustment = MFI.getOffsetAdjustment();
  YamlMFI.MaxAlignment = MFI.getMaxAlignment();
  YamlMFI.AdjustsStack = MFI.adjustsStack();
  YamlMFI.HasCalls = MFI.hasCalls();
  YamlMFI.MaxCallFrameSize = MFI.getMaxCallFrameSize();
  YamlMFI.HasOpaqueSPAdjustment = MFI.hasOpaqueSPAdjustment();
  YamlMFI.HasVAStart = MFI.hasVAStart();
  YamlMFI.HasMustTailInVarArgFunc = MFI.hasMustTailInVarArgFunc();
  if (MFI.getSavePoint()) {
    raw_string_ostream StrOS(YamlMFI.SavePoint.Value);
    MIPrinter(StrOS, MST, RegisterMaskIds, StackObjectOperandMapping)
        .printMBBReference(*MFI.getSavePoint());
  }
  if (MFI.getRestorePoint()) {
    raw_string_ostream StrOS(YamlMFI.RestorePoint.Value);
    MIPrinter(StrOS, MST, RegisterMaskIds, StackObjectOperandMapping)
        .printMBBReference(*MFI.getRestorePoint());
  }
}

void MIRPrinter::convertStackObjects(yaml::MachineFunction &MF,
                                     const MachineFrameInfo &MFI,
                                     const TargetRegisterInfo *TRI) {
  // Process fixed stack objects.
  unsigned ID = 0;
  for (int I = MFI.getObjectIndexBegin(); I < 0; ++I) {
    if (MFI.isDeadObjectIndex(I))
      continue;

    yaml::FixedMachineStackObject YamlObject;
    YamlObject.ID = ID;
    YamlObject.Type = MFI.isSpillSlotObjectIndex(I)
                          ? yaml::FixedMachineStackObject::SpillSlot
                          : yaml::FixedMachineStackObject::DefaultType;
    YamlObject.Offset = MFI.getObjectOffset(I);
    YamlObject.Size = MFI.getObjectSize(I);
    YamlObject.Alignment = MFI.getObjectAlignment(I);
    YamlObject.IsImmutable = MFI.isImmutableObjectIndex(I);
    YamlObject.IsAliased = MFI.isAliasedObjectIndex(I);
    MF.FixedStackObjects.push_back(YamlObject);
    StackObjectOperandMapping.insert(
        std::make_pair(I, FrameIndexOperand::createFixed(ID++)));
  }

  // Process ordinary stack objects.
  ID = 0;
  for (int I = 0, E = MFI.getObjectIndexEnd(); I < E; ++I) {
    if (MFI.isDeadObjectIndex(I))
      continue;

    yaml::MachineStackObject YamlObject;
    YamlObject.ID = ID;
    if (const auto *Alloca = MFI.getObjectAllocation(I))
      YamlObject.Name.Value =
          Alloca->hasName() ? Alloca->getName() : "<unnamed alloca>";
    YamlObject.Type = MFI.isSpillSlotObjectIndex(I)
                          ? yaml::MachineStackObject::SpillSlot
                          : MFI.isVariableSizedObjectIndex(I)
                                ? yaml::MachineStackObject::VariableSized
                                : yaml::MachineStackObject::DefaultType;
    YamlObject.Offset = MFI.getObjectOffset(I);
    YamlObject.Size = MFI.getObjectSize(I);
    YamlObject.Alignment = MFI.getObjectAlignment(I);

    MF.StackObjects.push_back(YamlObject);
    StackObjectOperandMapping.insert(std::make_pair(
        I, FrameIndexOperand::create(YamlObject.Name.Value, ID++)));
  }

  for (const auto &CSInfo : MFI.getCalleeSavedInfo()) {
    yaml::StringValue Reg;
    printReg(CSInfo.getReg(), Reg, TRI);
    auto StackObjectInfo = StackObjectOperandMapping.find(CSInfo.getFrameIdx());
    assert(StackObjectInfo != StackObjectOperandMapping.end() &&
           "Invalid stack object index");
    const FrameIndexOperand &StackObject = StackObjectInfo->second;
    if (StackObject.IsFixed)
      MF.FixedStackObjects[StackObject.ID].CalleeSavedRegister = Reg;
    else
      MF.StackObjects[StackObject.ID].CalleeSavedRegister = Reg;
  }
}

void MIRPrinter::convert(yaml::MachineFunction &MF,
                         const MachineConstantPool &ConstantPool) {
  unsigned ID = 0;
  for (const MachineConstantPoolEntry &Constant : ConstantPool.getConstants()) {
    // TODO: Serialize target specific constant pool entries.
    if (Constant.isMachineConstantPoolEntry())
      llvm_unreachable("Can't print target specific constant pool entries yet");

    yaml::MachineConstantPoolValue YamlConstant;
    std::string Str;
    raw_string_ostream StrOS(Str);
    Constant.Val.ConstVal->printAsOperand(StrOS);
    YamlConstant.ID = ID++;
    YamlConstant.Value = StrOS.str();
    YamlConstant.Alignment = Constant.getAlignment();
    MF.Constants.push_back(YamlConstant);
  }
}

void MIRPrinter::convert(ModuleSlotTracker &MST,
                         yaml::MachineJumpTable &YamlJTI,
                         const MachineJumpTableInfo &JTI) {
  YamlJTI.Kind = JTI.getEntryKind();
  unsigned ID = 0;
  for (const auto &Table : JTI.getJumpTables()) {
    std::string Str;
    yaml::MachineJumpTable::Entry Entry;
    Entry.ID = ID++;
    for (const auto *MBB : Table.MBBs) {
      raw_string_ostream StrOS(Str);
      MIPrinter(StrOS, MST, RegisterMaskIds, StackObjectOperandMapping)
          .printMBBReference(*MBB);
      Entry.Blocks.push_back(StrOS.str());
      Str.clear();
    }
    YamlJTI.Entries.push_back(Entry);
  }
}

void MIRPrinter::initRegisterMaskIds(const MachineFunction &MF) {
  const auto *TRI = MF.getSubtarget().getRegisterInfo();
  unsigned I = 0;
  for (const uint32_t *Mask : TRI->getRegMasks())
    RegisterMaskIds.insert(std::make_pair(Mask, I++));
}

void MIPrinter::print(const MachineBasicBlock &MBB) {
  assert(MBB.getNumber() >= 0 && "Invalid MBB number");
  OS << "bb." << MBB.getNumber();
  bool HasAttributes = false;
  if (const auto *BB = MBB.getBasicBlock()) {
    if (BB->hasName()) {
      OS << "." << BB->getName();
    } else {
      HasAttributes = true;
      OS << " (";
      int Slot = MST.getLocalSlot(BB);
      if (Slot == -1)
        OS << "<ir-block badref>";
      else
        OS << (Twine("%ir-block.") + Twine(Slot)).str();
    }
  }
  if (MBB.hasAddressTaken()) {
    OS << (HasAttributes ? ", " : " (");
    OS << "address-taken";
    HasAttributes = true;
  }
  if (MBB.isLandingPad()) {
    OS << (HasAttributes ? ", " : " (");
    OS << "landing-pad";
    HasAttributes = true;
  }
  if (MBB.getAlignment()) {
    OS << (HasAttributes ? ", " : " (");
    OS << "align " << MBB.getAlignment();
    HasAttributes = true;
  }
  if (HasAttributes)
    OS << ")";
  OS << ":\n";

  bool HasLineAttributes = false;
  // Print the successors
  if (!MBB.succ_empty()) {
    OS.indent(2) << "successors: ";
    for (auto I = MBB.succ_begin(), E = MBB.succ_end(); I != E; ++I) {
      if (I != MBB.succ_begin())
        OS << ", ";
      printMBBReference(**I);
      if (MBB.hasSuccessorWeights())
        OS << '(' << MBB.getSuccWeight(I) << ')';
    }
    OS << "\n";
    HasLineAttributes = true;
  }

  // Print the live in registers.
  const auto *TRI = MBB.getParent()->getSubtarget().getRegisterInfo();
  assert(TRI && "Expected target register info");
  if (!MBB.livein_empty()) {
    OS.indent(2) << "liveins: ";
    for (auto I = MBB.livein_begin(), E = MBB.livein_end(); I != E; ++I) {
      if (I != MBB.livein_begin())
        OS << ", ";
      printReg(*I, OS, TRI);
    }
    OS << "\n";
    HasLineAttributes = true;
  }

  if (HasLineAttributes)
    OS << "\n";
  bool IsInBundle = false;
  for (auto I = MBB.instr_begin(), E = MBB.instr_end(); I != E; ++I) {
    const MachineInstr &MI = *I;
    if (IsInBundle && !MI.isInsideBundle()) {
      OS.indent(2) << "}\n";
      IsInBundle = false;
    }
    OS.indent(IsInBundle ? 4 : 2);
    print(MI);
    if (!IsInBundle && MI.getFlag(MachineInstr::BundledSucc)) {
      OS << " {";
      IsInBundle = true;
    }
    OS << "\n";
  }
  if (IsInBundle)
    OS.indent(2) << "}\n";
}

void MIPrinter::print(const MachineInstr &MI) {
  const auto &SubTarget = MI.getParent()->getParent()->getSubtarget();
  const auto *TRI = SubTarget.getRegisterInfo();
  assert(TRI && "Expected target register info");
  const auto *TII = SubTarget.getInstrInfo();
  assert(TII && "Expected target instruction info");
  if (MI.isCFIInstruction())
    assert(MI.getNumOperands() == 1 && "Expected 1 operand in CFI instruction");

  unsigned I = 0, E = MI.getNumOperands();
  for (; I < E && MI.getOperand(I).isReg() && MI.getOperand(I).isDef() &&
         !MI.getOperand(I).isImplicit();
       ++I) {
    if (I)
      OS << ", ";
    print(MI.getOperand(I), TRI);
  }

  if (I)
    OS << " = ";
  if (MI.getFlag(MachineInstr::FrameSetup))
    OS << "frame-setup ";
  OS << TII->getName(MI.getOpcode());
  if (I < E)
    OS << ' ';

  bool NeedComma = false;
  for (; I < E; ++I) {
    if (NeedComma)
      OS << ", ";
    print(MI.getOperand(I), TRI);
    NeedComma = true;
  }

  if (MI.getDebugLoc()) {
    if (NeedComma)
      OS << ',';
    OS << " debug-location ";
    MI.getDebugLoc()->printAsOperand(OS, MST);
  }

  if (!MI.memoperands_empty()) {
    OS << " :: ";
    bool NeedComma = false;
    for (const auto *Op : MI.memoperands()) {
      if (NeedComma)
        OS << ", ";
      print(*Op);
      NeedComma = true;
    }
  }
}

void MIPrinter::printMBBReference(const MachineBasicBlock &MBB) {
  OS << "%bb." << MBB.getNumber();
  if (const auto *BB = MBB.getBasicBlock()) {
    if (BB->hasName())
      OS << '.' << BB->getName();
  }
}

void MIPrinter::printIRBlockReference(const BasicBlock &BB) {
  OS << "%ir-block.";
  if (BB.hasName()) {
    printLLVMNameWithoutPrefix(OS, BB.getName());
    return;
  }
  const Function *F = BB.getParent();
  int Slot;
  if (F == MST.getCurrentFunction()) {
    Slot = MST.getLocalSlot(&BB);
  } else {
    ModuleSlotTracker CustomMST(F->getParent(),
                                /*ShouldInitializeAllMetadata=*/false);
    CustomMST.incorporateFunction(*F);
    Slot = CustomMST.getLocalSlot(&BB);
  }
  if (Slot == -1)
    OS << "<badref>";
  else
    OS << Slot;
}

void MIPrinter::printIRValueReference(const Value &V) {
  OS << "%ir.";
  if (V.hasName()) {
    printLLVMNameWithoutPrefix(OS, V.getName());
    return;
  }
  // TODO: Serialize the unnamed IR value references.
  OS << "<unserializable ir value>";
}

void MIPrinter::printStackObjectReference(int FrameIndex) {
  auto ObjectInfo = StackObjectOperandMapping.find(FrameIndex);
  assert(ObjectInfo != StackObjectOperandMapping.end() &&
         "Invalid frame index");
  const FrameIndexOperand &Operand = ObjectInfo->second;
  if (Operand.IsFixed) {
    OS << "%fixed-stack." << Operand.ID;
    return;
  }
  OS << "%stack." << Operand.ID;
  if (!Operand.Name.empty())
    OS << '.' << Operand.Name;
}

void MIPrinter::printOffset(int64_t Offset) {
  if (Offset == 0)
    return;
  if (Offset < 0) {
    OS << " - " << -Offset;
    return;
  }
  OS << " + " << Offset;
}

static const char *getTargetFlagName(const TargetInstrInfo *TII, unsigned TF) {
  auto Flags = TII->getSerializableDirectMachineOperandTargetFlags();
  for (const auto &I : Flags) {
    if (I.first == TF) {
      return I.second;
    }
  }
  return nullptr;
}

void MIPrinter::printTargetFlags(const MachineOperand &Op) {
  if (!Op.getTargetFlags())
    return;
  const auto *TII =
      Op.getParent()->getParent()->getParent()->getSubtarget().getInstrInfo();
  assert(TII && "expected instruction info");
  auto Flags = TII->decomposeMachineOperandsTargetFlags(Op.getTargetFlags());
  OS << "target-flags(";
  if (const auto *Name = getTargetFlagName(TII, Flags.first))
    OS << Name;
  else
    OS << "<unknown target flag>";
  // TODO: Print the target's bit flags.
  OS << ") ";
}

static const char *getTargetIndexName(const MachineFunction &MF, int Index) {
  const auto *TII = MF.getSubtarget().getInstrInfo();
  assert(TII && "expected instruction info");
  auto Indices = TII->getSerializableTargetIndices();
  for (const auto &I : Indices) {
    if (I.first == Index) {
      return I.second;
    }
  }
  return nullptr;
}

void MIPrinter::print(const MachineOperand &Op, const TargetRegisterInfo *TRI) {
  printTargetFlags(Op);
  switch (Op.getType()) {
  case MachineOperand::MO_Register:
    // FIXME: Serialize the tied register.
    if (Op.isImplicit())
      OS << (Op.isDef() ? "implicit-def " : "implicit ");
    if (Op.isInternalRead())
      OS << "internal ";
    if (Op.isDead())
      OS << "dead ";
    if (Op.isKill())
      OS << "killed ";
    if (Op.isUndef())
      OS << "undef ";
    if (Op.isEarlyClobber())
      OS << "early-clobber ";
    if (Op.isDebug())
      OS << "debug-use ";
    printReg(Op.getReg(), OS, TRI);
    // Print the sub register.
    if (Op.getSubReg() != 0)
      OS << ':' << TRI->getSubRegIndexName(Op.getSubReg());
    break;
  case MachineOperand::MO_Immediate:
    OS << Op.getImm();
    break;
  case MachineOperand::MO_CImmediate:
    Op.getCImm()->printAsOperand(OS, /*PrintType=*/true, MST);
    break;
  case MachineOperand::MO_FPImmediate:
    Op.getFPImm()->printAsOperand(OS, /*PrintType=*/true, MST);
    break;
  case MachineOperand::MO_MachineBasicBlock:
    printMBBReference(*Op.getMBB());
    break;
  case MachineOperand::MO_FrameIndex:
    printStackObjectReference(Op.getIndex());
    break;
  case MachineOperand::MO_ConstantPoolIndex:
    OS << "%const." << Op.getIndex();
    printOffset(Op.getOffset());
    break;
  case MachineOperand::MO_TargetIndex: {
    OS << "target-index(";
    if (const auto *Name = getTargetIndexName(
            *Op.getParent()->getParent()->getParent(), Op.getIndex()))
      OS << Name;
    else
      OS << "<unknown>";
    OS << ')';
    printOffset(Op.getOffset());
    break;
  }
  case MachineOperand::MO_JumpTableIndex:
    OS << "%jump-table." << Op.getIndex();
    break;
  case MachineOperand::MO_ExternalSymbol:
    OS << '$';
    printLLVMNameWithoutPrefix(OS, Op.getSymbolName());
    printOffset(Op.getOffset());
    break;
  case MachineOperand::MO_GlobalAddress:
    Op.getGlobal()->printAsOperand(OS, /*PrintType=*/false, MST);
    printOffset(Op.getOffset());
    break;
  case MachineOperand::MO_BlockAddress:
    OS << "blockaddress(";
    Op.getBlockAddress()->getFunction()->printAsOperand(OS, /*PrintType=*/false,
                                                        MST);
    OS << ", ";
    printIRBlockReference(*Op.getBlockAddress()->getBasicBlock());
    OS << ')';
    printOffset(Op.getOffset());
    break;
  case MachineOperand::MO_RegisterMask: {
    auto RegMaskInfo = RegisterMaskIds.find(Op.getRegMask());
    if (RegMaskInfo != RegisterMaskIds.end())
      OS << StringRef(TRI->getRegMaskNames()[RegMaskInfo->second]).lower();
    else
      llvm_unreachable("Can't print this machine register mask yet.");
    break;
  }
  case MachineOperand::MO_RegisterLiveOut: {
    const uint32_t *RegMask = Op.getRegLiveOut();
    OS << "liveout(";
    bool IsCommaNeeded = false;
    for (unsigned Reg = 0, E = TRI->getNumRegs(); Reg < E; ++Reg) {
      if (RegMask[Reg / 32] & (1U << (Reg % 32))) {
        if (IsCommaNeeded)
          OS << ", ";
        printReg(Reg, OS, TRI);
        IsCommaNeeded = true;
      }
    }
    OS << ")";
    break;
  }
  case MachineOperand::MO_Metadata:
    Op.getMetadata()->printAsOperand(OS, MST);
    break;
  case MachineOperand::MO_CFIIndex: {
    const auto &MMI = Op.getParent()->getParent()->getParent()->getMMI();
    print(MMI.getFrameInstructions()[Op.getCFIIndex()], TRI);
    break;
  }
  default:
    // TODO: Print the other machine operands.
    llvm_unreachable("Can't print this machine operand at the moment");
  }
}

void MIPrinter::print(const MachineMemOperand &Op) {
  OS << '(';
  // TODO: Print operand's target specific flags.
  if (Op.isVolatile())
    OS << "volatile ";
  if (Op.isNonTemporal())
    OS << "non-temporal ";
  if (Op.isInvariant())
    OS << "invariant ";
  if (Op.isLoad())
    OS << "load ";
  else {
    assert(Op.isStore() && "Non load machine operand must be a store");
    OS << "store ";
  }
  OS << Op.getSize() << (Op.isLoad() ? " from " : " into ");
  if (const Value *Val = Op.getValue()) {
    printIRValueReference(*Val);
  } else {
    const PseudoSourceValue *PVal = Op.getPseudoValue();
    assert(PVal && "Expected a pseudo source value");
    switch (PVal->kind()) {
    case PseudoSourceValue::Stack:
      OS << "stack";
      break;
    case PseudoSourceValue::GOT:
      OS << "got";
      break;
    case PseudoSourceValue::JumpTable:
      OS << "jump-table";
      break;
    case PseudoSourceValue::ConstantPool:
      OS << "constant-pool";
      break;
    case PseudoSourceValue::FixedStack:
      printStackObjectReference(
          cast<FixedStackPseudoSourceValue>(PVal)->getFrameIndex());
      break;
    case PseudoSourceValue::GlobalValueCallEntry:
      cast<GlobalValuePseudoSourceValue>(PVal)->getValue()->printAsOperand(
          OS, /*PrintType=*/false, MST);
      break;
    case PseudoSourceValue::ExternalSymbolCallEntry:
      OS << '$';
      printLLVMNameWithoutPrefix(
          OS, cast<ExternalSymbolPseudoSourceValue>(PVal)->getSymbol());
      break;
    }
  }
  printOffset(Op.getOffset());
  if (Op.getBaseAlignment() != Op.getSize())
    OS << ", align " << Op.getBaseAlignment();
  auto AAInfo = Op.getAAInfo();
  if (AAInfo.TBAA) {
    OS << ", !tbaa ";
    AAInfo.TBAA->printAsOperand(OS, MST);
  }
  if (AAInfo.Scope) {
    OS << ", !alias.scope ";
    AAInfo.Scope->printAsOperand(OS, MST);
  }
  // TODO: Print AA NoAlias metadata.
  // TODO: Print the ranges metadata.
  OS << ')';
}

static void printCFIRegister(unsigned DwarfReg, raw_ostream &OS,
                             const TargetRegisterInfo *TRI) {
  int Reg = TRI->getLLVMRegNum(DwarfReg, true);
  if (Reg == -1) {
    OS << "<badreg>";
    return;
  }
  printReg(Reg, OS, TRI);
}

void MIPrinter::print(const MCCFIInstruction &CFI,
                      const TargetRegisterInfo *TRI) {
  switch (CFI.getOperation()) {
  case MCCFIInstruction::OpSameValue:
    OS << ".cfi_same_value ";
    if (CFI.getLabel())
      OS << "<mcsymbol> ";
    printCFIRegister(CFI.getRegister(), OS, TRI);
    break;
  case MCCFIInstruction::OpOffset:
    OS << ".cfi_offset ";
    if (CFI.getLabel())
      OS << "<mcsymbol> ";
    printCFIRegister(CFI.getRegister(), OS, TRI);
    OS << ", " << CFI.getOffset();
    break;
  case MCCFIInstruction::OpDefCfaRegister:
    OS << ".cfi_def_cfa_register ";
    if (CFI.getLabel())
      OS << "<mcsymbol> ";
    printCFIRegister(CFI.getRegister(), OS, TRI);
    break;
  case MCCFIInstruction::OpDefCfaOffset:
    OS << ".cfi_def_cfa_offset ";
    if (CFI.getLabel())
      OS << "<mcsymbol> ";
    OS << CFI.getOffset();
    break;
  case MCCFIInstruction::OpDefCfa:
    OS << ".cfi_def_cfa ";
    if (CFI.getLabel())
      OS << "<mcsymbol> ";
    printCFIRegister(CFI.getRegister(), OS, TRI);
    OS << ", " << CFI.getOffset();
    break;
  default:
    // TODO: Print the other CFI Operations.
    OS << "<unserializable cfi operation>";
    break;
  }
}

void llvm::printMIR(raw_ostream &OS, const Module &M) {
  yaml::Output Out(OS);
  Out << const_cast<Module &>(M);
}

void llvm::printMIR(raw_ostream &OS, const MachineFunction &MF) {
  MIRPrinter Printer(OS);
  Printer.print(MF);
}
