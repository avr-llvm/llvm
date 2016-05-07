//==-- llvm/CodeGen/GlobalISel/RegisterBankInfo.h ----------------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file This file declares the API for the register bank info.
/// This API is responsible for handling the register banks.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_GLOBALISEL_REGBANKINFO_H
#define LLVM_CODEGEN_GLOBALISEL_REGBANKINFO_H

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/CodeGen/GlobalISel/RegisterBank.h"
#include "llvm/CodeGen/MachineValueType.h" // For SimpleValueType.
#include "llvm/Support/ErrorHandling.h"

#include <cassert>
#include <memory> // For unique_ptr.

namespace llvm {
class MachineInstr;
class MachineRegisterInfo;
class TargetInstrInfo;
class TargetRegisterInfo;
class raw_ostream;

/// Holds all the information related to register banks.
class RegisterBankInfo {
public:
  /// Helper struct that represents how a value is partially mapped
  /// into a register.
  /// The StartIdx and Length represent what region of the orginal
  /// value this partial mapping covers.
  /// This can be represented as a Mask of contiguous bit starting
  /// at StartIdx bit and spanning Length bits.
  /// StartIdx is the number of bits from the less significant bits.
  struct PartialMapping {
    /// Number of bits at which this partial mapping starts in the
    /// original value.  The bits are counted from less significant
    /// bits to most significant bits.
    unsigned StartIdx;
    /// Length of this mapping in bits. This is how many bits this
    /// partial mapping covers in the original value:
    /// from StartIdx to StartIdx + Length -1.
    unsigned Length;
    /// Register bank where the partial value lives.
    const RegisterBank *RegBank;

    PartialMapping() = default;

    /// Provide a shortcut for quickly building PartialMapping.
    PartialMapping(unsigned StartIdx, unsigned Length,
                   const RegisterBank &RegBank)
        : StartIdx(StartIdx), Length(Length), RegBank(&RegBank) {}

    /// \return the index of in the original value of the most
    /// significant bit that this partial mapping covers.
    unsigned getHighBitIdx() const { return StartIdx + Length - 1; }

    /// Print this partial mapping on dbgs() stream.
    void dump() const;

    /// Print this partial mapping on \p OS;
    void print(raw_ostream &OS) const;

    /// Check that the Mask is compatible with the RegBank.
    /// Indeed, if the RegBank cannot accomadate the "active bits" of the mask,
    /// there is no way this mapping is valid.
    ///
    /// \note This method does not check anything when assertions are disabled.
    ///
    /// \return True is the check was successful.
    bool verify() const;
  };

  /// Helper struct that represents how a value is mapped through
  /// different register banks.
  struct ValueMapping {
    /// How the value is broken down between the different register banks.
    SmallVector<PartialMapping, 2> BreakDown;

    /// Verify that this mapping makes sense for a value of \p ExpectedBitWidth.
    /// \note This method does not check anything when assertions are disabled.
    ///
    /// \return True is the check was successful.
    bool verify(unsigned ExpectedBitWidth) const;

    /// Print this on dbgs() stream.
    void dump() const;

    /// Print this on \p OS;
    void print(raw_ostream &OS) const;
  };

  /// Helper class that represents how the value of an instruction may be
  /// mapped and what is the related cost of such mapping.
  class InstructionMapping {
    /// Identifier of the mapping.
    /// This is used to communicate between the target and the optimizers
    /// which mapping should be realized.
    unsigned ID;
    /// Cost of this mapping.
    unsigned Cost;
    /// Mapping of all the operands.
    std::unique_ptr<ValueMapping[]> OperandsMapping;
    /// Number of operands.
    unsigned NumOperands;

    ValueMapping &getOperandMapping(unsigned i) {
      assert(i < getNumOperands() && "Out of bound operand");
      return OperandsMapping[i];
    }

  public:
    /// Constructor for the mapping of an instruction.
    /// \p NumOperands must be equal to number of all the operands of
    /// the related instruction.
    /// The rationale is that it is more efficient for the optimizers
    /// to be able to assume that the mapping of the ith operand is
    /// at the index i.
    ///
    /// \pre ID != InvalidMappingID
    InstructionMapping(unsigned ID, unsigned Cost, unsigned NumOperands)
        : ID(ID), Cost(Cost), NumOperands(NumOperands) {
      assert(getID() != InvalidMappingID &&
             "Use the default constructor for invalid mapping");
      OperandsMapping.reset(new ValueMapping[getNumOperands()]);
    }

    /// Default constructor.
    /// Use this constructor to express that the mapping is invalid.
    InstructionMapping() : ID(InvalidMappingID), Cost(0), NumOperands(0) {}

    /// Get the cost.
    unsigned getCost() const { return Cost; }

    /// Get the ID.
    unsigned getID() const { return ID; }

    /// Get the number of operands.
    unsigned getNumOperands() const { return NumOperands; }

    /// Get the value mapping of the ith operand.
    const ValueMapping &getOperandMapping(unsigned i) const {
      return const_cast<InstructionMapping *>(this)->getOperandMapping(i);
    }

    /// Get the value mapping of the ith operand.
    void setOperandMapping(unsigned i, const ValueMapping &ValMapping) {
      getOperandMapping(i) = ValMapping;
    }

    /// Check whether this object is valid.
    /// This is a lightweight check for obvious wrong instance.
    bool isValid() const { return getID() != InvalidMappingID; }

    /// Set the operand mapping for the \p OpIdx-th operand.
    /// The mapping will consist of only one element in the break down list.
    /// This element will map to \p RegBank and fully define a mask, whose
    /// bitwidth matches the size of \p MaskSize.
    void setOperandMapping(unsigned OpIdx, unsigned MaskSize,
                           const RegisterBank &RegBank);

    /// Verifiy that this mapping makes sense for \p MI.
    /// \pre \p MI must be connected to a MachineFunction.
    ///
    /// \note This method does not check anything when assertions are disabled.
    ///
    /// \return True is the check was successful.
    bool verify(const MachineInstr &MI) const;

    /// Print this on dbgs() stream.
    void dump() const;

    /// Print this on \p OS;
    void print(raw_ostream &OS) const;
  };

  /// Convenient type to represent the alternatives for mapping an
  /// instruction.
  /// \todo When we move to TableGen this should be an array ref.
  typedef SmallVector<InstructionMapping, 4> InstructionMappings;

protected:
  /// Hold the set of supported register banks.
  std::unique_ptr<RegisterBank[]> RegBanks;
  /// Total number of register banks.
  unsigned NumRegBanks;

  /// Mapping from MVT::SimpleValueType to register banks.
  std::unique_ptr<const RegisterBank *[]> VTToRegBank;

  /// Create a RegisterBankInfo that can accomodate up to \p NumRegBanks
  /// RegisterBank instances.
  ///
  /// \note For the verify method to succeed all the \p NumRegBanks
  /// must be initialized by createRegisterBank and updated with
  /// addRegBankCoverage RegisterBank.
  RegisterBankInfo(unsigned NumRegBanks);

  /// This constructor is meaningless.
  /// It just provides a default constructor that can be used at link time
  /// when GlobalISel is not built.
  /// That way, targets can still inherit from this class without doing
  /// crazy gymnastic to avoid link time failures.
  /// \note That works because the constructor is inlined.
  RegisterBankInfo() {
    llvm_unreachable("This constructor should not be executed");
  }

  /// Create a new register bank with the given parameter and add it
  /// to RegBanks.
  /// \pre \p ID must not already be used.
  /// \pre \p ID < NumRegBanks.
  void createRegisterBank(unsigned ID, const char *Name);

  /// Add \p RCId to the set of register class that the register bank,
  /// identified \p ID, covers.
  /// This method transitively adds all the sub classes and the subreg-classes
  /// of \p RCId to the set of covered register classes.
  /// It also adjusts the size of the register bank to reflect the maximal
  /// size of a value that can be hold into that register bank.
  ///
  /// If \p AddTypeMapping is true, this method also records what types can
  /// be mapped to \p ID. Although this done by default, targets may want to
  /// disable it, espicially if a given type may be mapped on different
  /// register bank. Indeed, in such case, this method only records the
  /// first register bank where the type matches.
  /// This information is only used to provide default mapping
  /// (see getInstrMappingImpl).
  ///
  /// \note This method does *not* add the super classes of \p RCId.
  /// The rationale is if \p ID covers the registers of \p RCId, that
  /// does not necessarily mean that \p ID covers the set of registers
  /// of RCId's superclasses.
  /// This method does *not* add the superreg classes as well for consistents.
  /// The expected use is to add the coverage top-down with respect to the
  /// register hierarchy.
  ///
  /// \todo TableGen should just generate the BitSet vector for us.
  void addRegBankCoverage(unsigned ID, unsigned RCId,
                          const TargetRegisterInfo &TRI,
                          bool AddTypeMapping = true);

  /// Get the register bank identified by \p ID.
  RegisterBank &getRegBank(unsigned ID) {
    assert(ID < getNumRegBanks() && "Accessing an unknown register bank");
    return RegBanks[ID];
  }

  /// Get the register bank that has been recorded to cover \p SVT.
  const RegisterBank *getRegBankForType(MVT::SimpleValueType SVT) const {
    if (!VTToRegBank)
      return nullptr;
    assert(SVT < MVT::SimpleValueType::LAST_VALUETYPE && "Out-of-bound access");
    return VTToRegBank.get()[SVT];
  }

  /// Record \p RegBank as the register bank that covers \p SVT.
  /// If a record was already set for \p SVT, the mapping is not
  /// updated, unless \p Force == true
  ///
  /// \post if getRegBankForType(SVT)\@pre == nullptr then
  ///                       getRegBankForType(SVT) == &RegBank
  /// \post if Force == true then getRegBankForType(SVT) == &RegBank
  void recordRegBankForType(const RegisterBank &RegBank,
                            MVT::SimpleValueType SVT, bool Force = false) {
    if (!VTToRegBank)
      VTToRegBank.reset(
          new const RegisterBank *[MVT::SimpleValueType::LAST_VALUETYPE]);
    assert(SVT < MVT::SimpleValueType::LAST_VALUETYPE && "Out-of-bound access");
    // If we want to override the mapping or the mapping does not exits yet,
    // set the register bank for SVT.
    if (Force || !getRegBankForType(SVT))
      VTToRegBank.get()[SVT] = &RegBank;
  }

  /// Try to get the mapping of \p MI.
  /// See getInstrMapping for more details on what a mapping represents.
  ///
  /// Unlike getInstrMapping the returned InstructionMapping may be invalid
  /// (isValid() == false).
  /// This means that the target independent code is not smart enough
  /// to get the mapping of \p MI and thus, the target has to provide the
  /// information for \p MI.
  ///
  /// This implementation is able to get the mapping of:
  /// - Target specific instructions by looking at the encoding constraints.
  /// - Any instruction if all the register operands are already been assigned
  ///   a register, a register class, or a register bank.
  /// - Copies and phis if at least one of the operand has been assigned a
  ///   register, a register class, or a register bank.
  /// In other words, this method will likely fail to find a mapping for
  /// any generic opcode that has not been lowered by target specific code.
  InstructionMapping getInstrMappingImpl(const MachineInstr &MI) const;

  /// Get the register bank for the \p OpIdx-th operand of \p MI form
  /// the encoding constraints, if any.
  ///
  /// \return A register bank that covers the register class of the
  /// related encoding constraints or nullptr if \p MI did not provide
  /// enough information to deduce it.
  const RegisterBank *
  getRegBankFromConstraints(const MachineInstr &MI, unsigned OpIdx,
                            const TargetInstrInfo &TII,
                            const TargetRegisterInfo &TRI) const;

public:
  virtual ~RegisterBankInfo() {}

  /// Get the register bank identified by \p ID.
  const RegisterBank &getRegBank(unsigned ID) const {
    return const_cast<RegisterBankInfo *>(this)->getRegBank(ID);
  }

  /// Get the register bank of \p Reg.
  /// If Reg has not been assigned a register, a register class,
  /// or a register bank, then this returns nullptr.
  ///
  /// \pre Reg != 0 (NoRegister)
  const RegisterBank *getRegBank(unsigned Reg, const MachineRegisterInfo &MRI,
                                 const TargetRegisterInfo &TRI) const;

  /// Get the total number of register banks.
  unsigned getNumRegBanks() const { return NumRegBanks; }

  /// Get a register bank that covers \p RC.
  ///
  /// \pre \p RC is a user-defined register class (as opposed as one
  /// generated by TableGen).
  ///
  /// \note The mapping RC -> RegBank could be built while adding the
  /// coverage for the register banks. However, we do not do it, because,
  /// at least for now, we only need this information for register classes
  /// that are used in the description of instruction. In other words,
  /// there are just a handful of them and we do not want to waste space.
  ///
  /// \todo This should be TableGen'ed.
  virtual const RegisterBank &
  getRegBankFromRegClass(const TargetRegisterClass &RC) const {
    llvm_unreachable("The target must override this method");
  }

  /// Get the cost of a copy from \p B to \p A, or put differently,
  /// get the cost of A = COPY B.
  virtual unsigned copyCost(const RegisterBank &A,
                            const RegisterBank &B) const {
    return 0;
  }

  /// Identifier used when the related instruction mapping instance
  /// is generated by target independent code.
  /// Make sure not to use that identifier to avoid possible collision.
  static const unsigned DefaultMappingID;

  /// Identifier used when the related instruction mapping instance
  /// is generated by the default constructor.
  /// Make sure not to use that identifier.
  static const unsigned InvalidMappingID;

  /// Get the mapping of the different operands of \p MI
  /// on the register bank.
  /// This mapping should be the direct translation of \p MI.
  /// In other words, when \p MI is mapped with the returned mapping,
  /// only the register banks of the operands of \p MI need to be updated.
  /// In particular, neither the opcode or the type of \p MI needs to be
  /// updated for this direct mapping.
  ///
  /// The target independent implementation gives a mapping based on
  /// the register classes for the target specific opcode.
  /// It uses the ID RegisterBankInfo::DefaultMappingID for that mapping.
  /// Make sure you do not use that ID for the alternative mapping
  /// for MI. See getInstrAlternativeMappings for the alternative
  /// mappings.
  ///
  /// For instance, if \p MI is a vector add, the mapping should
  /// not be a scalarization of the add.
  ///
  /// \post returnedVal.verify(MI).
  ///
  /// \note If returnedVal does not verify MI, this would probably mean
  /// that the target does not support that instruction.
  virtual InstructionMapping getInstrMapping(const MachineInstr &MI) const;

  /// Get the alternative mappings for \p MI.
  /// Alternative in the sense different from getInstrMapping.
  virtual InstructionMappings
  getInstrAlternativeMappings(const MachineInstr &MI) const;

  /// Get the possible mapping for \p MI.
  /// A mapping defines where the different operands may live and at what cost.
  /// For instance, let us consider:
  /// v0(16) = G_ADD <2 x i8> v1, v2
  /// The possible mapping could be:
  ///
  /// {/*ID*/VectorAdd, /*Cost*/1, /*v0*/{(0xFFFF, VPR)}, /*v1*/{(0xFFFF, VPR)},
  ///                              /*v2*/{(0xFFFF, VPR)}}
  /// {/*ID*/ScalarAddx2, /*Cost*/2, /*v0*/{(0x00FF, GPR),(0xFF00, GPR)},
  ///                                /*v1*/{(0x00FF, GPR),(0xFF00, GPR)},
  ///                                /*v2*/{(0x00FF, GPR),(0xFF00, GPR)}}
  ///
  /// \note The first alternative of the returned mapping should be the
  /// direct translation of \p MI current form.
  ///
  /// \post !returnedVal.empty().
  InstructionMappings getInstrPossibleMappings(const MachineInstr &MI) const;

  /// Check that information hold by this instance make sense for the
  /// given \p TRI.
  ///
  /// \note This method does not check anything when assertions are disabled.
  ///
  /// \return True is the check was successful.
  bool verify(const TargetRegisterInfo &TRI) const;
};

inline raw_ostream &
operator<<(raw_ostream &OS,
           const RegisterBankInfo::PartialMapping &PartMapping) {
  PartMapping.print(OS);
  return OS;
}

inline raw_ostream &
operator<<(raw_ostream &OS, const RegisterBankInfo::ValueMapping &ValMapping) {
  ValMapping.print(OS);
  return OS;
}

inline raw_ostream &
operator<<(raw_ostream &OS,
           const RegisterBankInfo::InstructionMapping &InstrMapping) {
  InstrMapping.print(OS);
  return OS;
}
} // End namespace llvm.

#endif
