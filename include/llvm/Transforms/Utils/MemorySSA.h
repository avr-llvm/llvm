//===- MemorySSA.h - Build Memory SSA ---------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// \file
// \brief This file exposes an interface to building/using memory SSA to
// walk memory instructions using a use/def graph.
//
// Memory SSA class builds an SSA form that links together memory access
// instructions such loads, stores, atomics, and calls. Additionally, it does a
// trivial form of "heap versioning" Every time the memory state changes in the
// program, we generate a new heap version. It generates MemoryDef/Uses/Phis
// that are overlayed on top of the existing instructions.
//
// As a trivial example,
// define i32 @main() #0 {
// entry:
//   %call = call noalias i8* @_Znwm(i64 4) #2
//   %0 = bitcast i8* %call to i32*
//   %call1 = call noalias i8* @_Znwm(i64 4) #2
//   %1 = bitcast i8* %call1 to i32*
//   store i32 5, i32* %0, align 4
//   store i32 7, i32* %1, align 4
//   %2 = load i32* %0, align 4
//   %3 = load i32* %1, align 4
//   %add = add nsw i32 %2, %3
//   ret i32 %add
// }
//
// Will become
// define i32 @main() #0 {
// entry:
//   ; 1 = MemoryDef(0)
//   %call = call noalias i8* @_Znwm(i64 4) #3
//   %2 = bitcast i8* %call to i32*
//   ; 2 = MemoryDef(1)
//   %call1 = call noalias i8* @_Znwm(i64 4) #3
//   %4 = bitcast i8* %call1 to i32*
//   ; 3 = MemoryDef(2)
//   store i32 5, i32* %2, align 4
//   ; 4 = MemoryDef(3)
//   store i32 7, i32* %4, align 4
//   ; MemoryUse(4)
//   %7 = load i32* %2, align 4
//   ; MemoryUse(3)
//   %8 = load i32* %4, align 4
//   %add = add nsw i32 %7, %8
//   ret i32 %add
// }
//
// Given this form, all the stores that could ever effect the load at %8 can be
// gotten by using the MemoryUse associated with it, and walking from use to def
// until you hit the top of the function.
//
// Each def also has a list of users associated with it, so you can walk from
// both def to users, and users to defs. Note that we disambiguate MemoryUses,
// but not the RHS of MemoryDefs. You can see this above at %8, which would
// otherwise be a MemoryUse(4). Being disambiguated means that for a given
// store, all the MemoryUses on its use lists are may-aliases of that store (but
// the MemoryDefs on its use list may not be).
//
// MemoryDefs are not disambiguated because it would require multiple reaching
// definitions, which would require multiple phis, and multiple memoryaccesses
// per instruction.
//===----------------------------------------------------------------------===//
#ifndef LLVM_TRANSFORMS_UTILS_MEMORYSSA_H
#define LLVM_TRANSFORMS_UTILS_MEMORYSSA_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/ADT/iterator.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/PHITransAddr.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/OperandTraits.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"

namespace llvm {
class BasicBlock;
class DominatorTree;
class Function;
class MemoryAccess;
template <class T> class memoryaccess_def_iterator_base;
using memoryaccess_def_iterator = memoryaccess_def_iterator_base<MemoryAccess>;
using const_memoryaccess_def_iterator =
    memoryaccess_def_iterator_base<const MemoryAccess>;

// \brief The base for all memory accesses. All memory accesses in a block are
// linked together using an intrusive list.
class MemoryAccess : public User, public ilist_node<MemoryAccess> {
  void *operator new(size_t, unsigned) = delete;
  void *operator new(size_t) = delete;

public:
  // Methods for support type inquiry through isa, cast, and
  // dyn_cast
  static inline bool classof(const MemoryAccess *) { return true; }
  static inline bool classof(const Value *V) {
    unsigned ID = V->getValueID();
    return ID == MemoryUseVal || ID == MemoryPhiVal || ID == MemoryDefVal;
  }

  virtual ~MemoryAccess();
  BasicBlock *getBlock() const { return Block; }

  virtual void print(raw_ostream &OS) const = 0;
  virtual void dump() const;

  /// \brief The user iterators for a memory access
  typedef user_iterator iterator;
  typedef const_user_iterator const_iterator;

  /// \brief This iterator walks over all of the defs in a given
  /// MemoryAccess. For MemoryPhi nodes, this walks arguments. For
  /// MemoryUse/MemoryDef, this walks the defining access.
  memoryaccess_def_iterator defs_begin();
  const_memoryaccess_def_iterator defs_begin() const;
  memoryaccess_def_iterator defs_end();
  const_memoryaccess_def_iterator defs_end() const;

protected:
  friend class MemorySSA;
  friend class MemoryUseOrDef;
  friend class MemoryUse;
  friend class MemoryDef;
  friend class MemoryPhi;

  /// \brief Used internally to give IDs to MemoryAccesses for printing
  virtual unsigned getID() const = 0;

  MemoryAccess(LLVMContext &C, unsigned Vty, BasicBlock *BB,
               unsigned NumOperands)
      : User(Type::getVoidTy(C), Vty, nullptr, NumOperands), Block(BB) {}

private:
  MemoryAccess(const MemoryAccess &);
  void operator=(const MemoryAccess &);
  BasicBlock *Block;
};

template <>
struct ilist_traits<MemoryAccess> : public ilist_default_traits<MemoryAccess> {
  /// See details of the instruction class for why this trick works
  /// FIXME: The downcast is UB.
  MemoryAccess *createSentinel() const {
    return static_cast<MemoryAccess *>(&Sentinel);
  }

  static void destroySentinel(MemoryAccess *) {}

  MemoryAccess *provideInitialHead() const { return createSentinel(); }
  MemoryAccess *ensureHead(MemoryAccess *) const { return createSentinel(); }
  static void noteHead(MemoryAccess *, MemoryAccess *) {}

private:
  mutable ilist_half_node<MemoryAccess> Sentinel;
};

inline raw_ostream &operator<<(raw_ostream &OS, const MemoryAccess &MA) {
  MA.print(OS);
  return OS;
}

/// \brief Class that has the common methods + fields of memory uses/defs. It's
/// a little awkward to have, but there are many cases where we want either a
/// use or def, and there are many cases where uses are needed (defs aren't
/// acceptable), and vice-versa.
///
/// This class should never be instantiated directly; make a MemoryUse or
/// MemoryDef instead.
class MemoryUseOrDef : public MemoryAccess {
  void *operator new(size_t, unsigned) = delete;
  void *operator new(size_t) = delete;

public:
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(MemoryAccess);

  /// \brief Get the instruction that this MemoryUse represents.
  Instruction *getMemoryInst() const { return MemoryInst; }

  /// \brief Get the access that produces the memory state used by this Use.
  MemoryAccess *getDefiningAccess() const { return getOperand(0); }

  static inline bool classof(const MemoryUseOrDef *) { return true; }
  static inline bool classof(const Value *MA) {
    return MA->getValueID() == MemoryUseVal || MA->getValueID() == MemoryDefVal;
  }

protected:
  friend class MemorySSA;

  MemoryUseOrDef(LLVMContext &C, MemoryAccess *DMA, unsigned Vty,
                 Instruction *MI, BasicBlock *BB)
      : MemoryAccess(C, Vty, BB, 1), MemoryInst(MI) {
    setDefiningAccess(DMA);
  }

  void setDefiningAccess(MemoryAccess *DMA) { setOperand(0, DMA); }

private:
  Instruction *MemoryInst;
};
template <>
struct OperandTraits<MemoryUseOrDef>
    : public FixedNumOperandTraits<MemoryUseOrDef, 1> {};
DEFINE_TRANSPARENT_OPERAND_ACCESSORS(MemoryUseOrDef, MemoryAccess)

/// \brief Represents read-only accesses to memory
///
/// In particular, the set of Instructions that will be represented by
/// MemoryUse's is exactly the set of Instructions for which
/// AliasAnalysis::getModRefInfo returns "Ref".
class MemoryUse final : public MemoryUseOrDef {
  void *operator new(size_t, unsigned) = delete;

public:
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(MemoryAccess);

  // allocate space for exactly one operand
  void *operator new(size_t s) { return User::operator new(s, 1); }

  MemoryUse(LLVMContext &C, MemoryAccess *DMA, Instruction *MI, BasicBlock *BB)
      : MemoryUseOrDef(C, DMA, MemoryUseVal, MI, BB) {}

  static inline bool classof(const MemoryUse *) { return true; }
  static inline bool classof(const Value *MA) {
    return MA->getValueID() == MemoryUseVal;
  }

  void print(raw_ostream &OS) const override;

protected:
  friend class MemorySSA;

  unsigned getID() const override {
    llvm_unreachable("MemoryUses do not have IDs");
  }
};
template <>
struct OperandTraits<MemoryUse> : public FixedNumOperandTraits<MemoryUse, 1> {};
DEFINE_TRANSPARENT_OPERAND_ACCESSORS(MemoryUse, MemoryAccess)

/// \brief Represents a read-write access to memory, whether it is a must-alias,
/// or a may-alias.
///
/// In particular, the set of Instructions that will be represented by
/// MemoryDef's is exactly the set of Instructions for which
/// AliasAnalysis::getModRefInfo returns "Mod" or "ModRef".
/// Note that, in order to provide def-def chains, all defs also have a use
/// associated with them. This use points to the nearest reaching
/// MemoryDef/MemoryPhi.
class MemoryDef final : public MemoryUseOrDef {
  void *operator new(size_t, unsigned) = delete;

public:
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(MemoryAccess);

  // allocate space for exactly one operand
  void *operator new(size_t s) { return User::operator new(s, 1); }

  MemoryDef(LLVMContext &C, MemoryAccess *DMA, Instruction *MI, BasicBlock *BB,
            unsigned Ver)
      : MemoryUseOrDef(C, DMA, MemoryDefVal, MI, BB), ID(Ver) {}

  static inline bool classof(const MemoryDef *) { return true; }
  static inline bool classof(const Value *MA) {
    return MA->getValueID() == MemoryDefVal;
  }

  void print(raw_ostream &OS) const override;

protected:
  friend class MemorySSA;

  // For debugging only. This gets used to give memory accesses pretty numbers
  // when printing them out
  unsigned getID() const override { return ID; }

private:
  const unsigned ID;
};
template <>
struct OperandTraits<MemoryDef> : public FixedNumOperandTraits<MemoryDef, 1> {};
DEFINE_TRANSPARENT_OPERAND_ACCESSORS(MemoryDef, MemoryAccess)

/// \brief Represents phi nodes for memory accesses.
///
/// These have the same semantic as regular phi nodes, with the exception that
/// only one phi will ever exist in a given basic block.
/// Guaranteeing one phi per block means guaranteeing there is only ever one
/// valid reaching MemoryDef/MemoryPHI along each path to the phi node.
/// This is ensured by not allowing disambiguation of the RHS of a MemoryDef or
/// a MemoryPhi's operands.
/// That is, given
/// if (a) {
///   store %a
///   store %b
/// }
/// it *must* be transformed into
/// if (a) {
///    1 = MemoryDef(liveOnEntry)
///    store %a
///    2 = MemoryDef(1)
///    store %b
/// }
/// and *not*
/// if (a) {
///    1 = MemoryDef(liveOnEntry)
///    store %a
///    2 = MemoryDef(liveOnEntry)
///    store %b
/// }
/// even if the two stores do not conflict. Otherwise, both 1 and 2 reach the
/// end of the branch, and if there are not two phi nodes, one will be
/// disconnected completely from the SSA graph below that point.
/// Because MemoryUse's do not generate new definitions, they do not have this
/// issue.
class MemoryPhi final : public MemoryAccess {
  void *operator new(size_t, unsigned) = delete;
  // allocate space for exactly zero operands
  void *operator new(size_t s) { return User::operator new(s); }

public:
  /// Provide fast operand accessors
  DECLARE_TRANSPARENT_OPERAND_ACCESSORS(MemoryAccess);

  MemoryPhi(LLVMContext &C, BasicBlock *BB, unsigned Ver, unsigned NumPreds = 0)
      : MemoryAccess(C, MemoryPhiVal, BB, 0), ID(Ver), ReservedSpace(NumPreds) {
    allocHungoffUses(ReservedSpace);
  }

  // Block iterator interface. This provides access to the list of incoming
  // basic blocks, which parallels the list of incoming values.
  typedef BasicBlock **block_iterator;
  typedef BasicBlock *const *const_block_iterator;

  block_iterator block_begin() {
    auto *Ref = reinterpret_cast<Use::UserRef *>(op_begin() + ReservedSpace);
    return reinterpret_cast<block_iterator>(Ref + 1);
  }

  const_block_iterator block_begin() const {
    const auto *Ref =
        reinterpret_cast<const Use::UserRef *>(op_begin() + ReservedSpace);
    return reinterpret_cast<const_block_iterator>(Ref + 1);
  }

  block_iterator block_end() { return block_begin() + getNumOperands(); }

  const_block_iterator block_end() const {
    return block_begin() + getNumOperands();
  }

  op_range incoming_values() { return operands(); }

  const_op_range incoming_values() const { return operands(); }

  /// \brief Return the number of incoming edges
  unsigned getNumIncomingValues() const { return getNumOperands(); }

  /// \brief Return incoming value number x
  MemoryAccess *getIncomingValue(unsigned I) const { return getOperand(I); }
  void setIncomingValue(unsigned I, MemoryAccess *V) {
    assert(V && "PHI node got a null value!");
    assert(getType() == V->getType() &&
           "All operands to PHI node must be the same type as the PHI node!");
    setOperand(I, V);
  }
  static unsigned getOperandNumForIncomingValue(unsigned I) { return I; }
  static unsigned getIncomingValueNumForOperand(unsigned I) { return I; }

  /// \brief Return incoming basic block number @p i.
  BasicBlock *getIncomingBlock(unsigned I) const { return block_begin()[I]; }

  /// \brief Return incoming basic block corresponding
  /// to an operand of the PHI.
  BasicBlock *getIncomingBlock(const Use &U) const {
    assert(this == U.getUser() && "Iterator doesn't point to PHI's Uses?");
    return getIncomingBlock(unsigned(&U - op_begin()));
  }

  /// \brief Return incoming basic block corresponding
  /// to value use iterator.
  BasicBlock *getIncomingBlock(MemoryAccess::const_user_iterator I) const {
    return getIncomingBlock(I.getUse());
  }

  void setIncomingBlock(unsigned I, BasicBlock *BB) {
    assert(BB && "PHI node got a null basic block!");
    block_begin()[I] = BB;
  }

  /// \brief Add an incoming value to the end of the PHI list
  void addIncoming(MemoryAccess *V, BasicBlock *BB) {
    if (getNumOperands() == ReservedSpace)
      growOperands(); // Get more space!
    // Initialize some new operands.
    setNumHungOffUseOperands(getNumOperands() + 1);
    setIncomingValue(getNumOperands() - 1, V);
    setIncomingBlock(getNumOperands() - 1, BB);
  }

  /// \brief Return the first index of the specified basic
  /// block in the value list for this PHI.  Returns -1 if no instance.
  int getBasicBlockIndex(const BasicBlock *BB) const {
    for (unsigned I = 0, E = getNumOperands(); I != E; ++I)
      if (block_begin()[I] == BB)
        return I;
    return -1;
  }

  Value *getIncomingValueForBlock(const BasicBlock *BB) const {
    int Idx = getBasicBlockIndex(BB);
    assert(Idx >= 0 && "Invalid basic block argument!");
    return getIncomingValue(Idx);
  }

  static inline bool classof(const MemoryPhi *) { return true; }
  static inline bool classof(const Value *V) {
    return V->getValueID() == MemoryPhiVal;
  }

  void print(raw_ostream &OS) const override;

protected:
  friend class MemorySSA;
  /// \brief this is more complicated than the generic
  /// User::allocHungoffUses, because we have to allocate Uses for the incoming
  /// values and pointers to the incoming blocks, all in one allocation.
  void allocHungoffUses(unsigned N) {
    User::allocHungoffUses(N, /* IsPhi */ true);
  }

  /// For debugging only. This gets used to give memory accesses pretty numbers
  /// when printing them out
  virtual unsigned getID() const final { return ID; }

private:
  // For debugging only
  const unsigned ID;
  unsigned ReservedSpace;

  /// \brief This grows the operand list in response to a push_back style of
  /// operation.  This grows the number of ops by 1.5 times.
  void growOperands() {
    unsigned E = getNumOperands();
    // 2 op PHI nodes are VERY common, so reserve at least enough for that.
    ReservedSpace = std::max(E + E / 2, 2u);
    growHungoffUses(ReservedSpace, /* IsPhi */ true);
  }
};
template <> struct OperandTraits<MemoryPhi> : public HungoffOperandTraits<2> {};

DEFINE_TRANSPARENT_OPERAND_ACCESSORS(MemoryPhi, MemoryAccess)

class MemorySSAWalker;

/// \brief Encapsulates MemorySSA, including all data associated with memory
/// accesses.
class MemorySSA {
public:
  MemorySSA(Function &);
  ~MemorySSA();

  /// \brief Build Memory SSA, and return the walker we used during building,
  /// for later reuse. If MemorySSA is already built, just return the walker.
  MemorySSAWalker *buildMemorySSA(AliasAnalysis *, DominatorTree *);

  /// \brief Returns false if you need to call buildMemorySSA.
  bool isFinishedBuilding() const { return Walker; }

  /// \brief Given a memory Mod/Ref'ing instruction, get the MemorySSA
  /// access associated with it. If passed a basic block gets the memory phi
  /// node that exists for that block, if there is one. Otherwise, this will get
  /// a MemoryUseOrDef.
  MemoryAccess *getMemoryAccess(const Value *) const;
  MemoryPhi *getMemoryAccess(const BasicBlock *BB) const;

  void dump() const;
  void print(raw_ostream &) const;

  /// \brief Return true if \p MA represents the live on entry value
  ///
  /// Loads and stores from pointer arguments and other global values may be
  /// defined by memory operations that do not occur in the current function, so
  /// they may be live on entry to the function. MemorySSA represents such
  /// memory state by the live on entry definition, which is guaranteed to occur
  /// before any other memory access in the function.
  inline bool isLiveOnEntryDef(const MemoryAccess *MA) const {
    return MA == LiveOnEntryDef.get();
  }

  inline MemoryAccess *getLiveOnEntryDef() const {
    return LiveOnEntryDef.get();
  }

  using AccessListType = iplist<MemoryAccess>;

  /// \brief Return the list of MemoryAccess's for a given basic block.
  ///
  /// This list is not modifiable by the user.
  const AccessListType *getBlockAccesses(const BasicBlock *BB) const {
    auto It = PerBlockAccesses.find(BB);
    return It == PerBlockAccesses.end() ? nullptr : It->second.get();
  }

  enum InsertionPlace { Beginning, End };

  /// \brief Given two memory accesses in the same basic block, determine
  /// whether MemoryAccess \p A dominates MemoryAccess \p B.
  bool locallyDominates(const MemoryAccess *A, const MemoryAccess *B) const;

protected:
  // Used by Memory SSA annotater, dumpers, and wrapper pass
  friend class MemorySSAAnnotatedWriter;
  friend class MemorySSAPrinterPass;
  void verifyDefUses(Function &F);
  void verifyDomination(Function &F);

private:
  void verifyUseInDefs(MemoryAccess *, MemoryAccess *);
  using AccessMap =
      DenseMap<const BasicBlock *, std::unique_ptr<AccessListType>>;

  void
  determineInsertionPoint(const SmallPtrSetImpl<BasicBlock *> &DefiningBlocks);
  void computeDomLevels(DenseMap<DomTreeNode *, unsigned> &DomLevels);
  void markUnreachableAsLiveOnEntry(BasicBlock *BB);
  bool dominatesUse(const MemoryAccess *, const MemoryAccess *) const;
  MemoryAccess *createNewAccess(Instruction *, bool ignoreNonMemory = false);
  MemoryAccess *findDominatingDef(BasicBlock *, enum InsertionPlace);

  MemoryAccess *renameBlock(BasicBlock *, MemoryAccess *);
  void renamePass(DomTreeNode *, MemoryAccess *IncomingVal,
                  SmallPtrSet<BasicBlock *, 16> &Visited);
  AccessListType *getOrCreateAccessList(BasicBlock *);
  AliasAnalysis *AA;
  DominatorTree *DT;
  Function &F;

  // Memory SSA mappings
  DenseMap<const Value *, MemoryAccess *> InstructionToMemoryAccess;
  AccessMap PerBlockAccesses;
  std::unique_ptr<MemoryAccess> LiveOnEntryDef;

  // Memory SSA building info
  MemorySSAWalker *Walker;
  unsigned NextID;
};

// This pass does eager building and then printing of MemorySSA. It is used by
// the tests to be able to build, dump, and verify Memory SSA.
class MemorySSAPrinterPass : public FunctionPass {
public:
  MemorySSAPrinterPass();

  static char ID;
  bool doInitialization(Module &M) override;
  bool runOnFunction(Function &) override;
  void releaseMemory() override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  void print(raw_ostream &OS, const Module *M) const override;
  static void registerOptions();
  MemorySSA &getMSSA() { return *MSSA; }

private:
  bool VerifyMemorySSA;

  std::unique_ptr<MemorySSA> MSSA;
  // FIXME(gbiv): It seems that MemorySSA doesn't own the walker it returns?
  std::unique_ptr<MemorySSAWalker> Walker;
  Function *F;
};

class MemorySSALazy : public FunctionPass {
public:
  MemorySSALazy();

  static char ID;
  bool runOnFunction(Function &) override;
  void releaseMemory() override;
  MemorySSA &getMSSA() {
    assert(MSSA);
    return *MSSA;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesAll();
  }

private:
  std::unique_ptr<MemorySSA> MSSA;
};

/// \brief This is the generic walker interface for walkers of MemorySSA.
/// Walkers are used to be able to further disambiguate the def-use chains
/// MemorySSA gives you, or otherwise produce better info than MemorySSA gives
/// you.
/// In particular, while the def-use chains provide basic information, and are
/// guaranteed to give, for example, the nearest may-aliasing MemoryDef for a
/// MemoryUse as AliasAnalysis considers it, a user mant want better or other
/// information. In particular, they may want to use SCEV info to further
/// disambiguate memory accesses, or they may want the nearest dominating
/// may-aliasing MemoryDef for a call or a store. This API enables a
/// standardized interface to getting and using that info.
class MemorySSAWalker {
public:
  MemorySSAWalker(MemorySSA *);
  virtual ~MemorySSAWalker() {}

  using MemoryAccessSet = SmallVector<MemoryAccess *, 8>;

  /// \brief Given a memory Mod/Ref/ModRef'ing instruction, calling this
  /// will give you the nearest dominating MemoryAccess that Mod's the location
  /// the instruction accesses (by skipping any def which AA can prove does not
  /// alias the location(s) accessed by the instruction given).
  ///
  /// Note that this will return a single access, and it must dominate the
  /// Instruction, so if an operand of a MemoryPhi node Mod's the instruction,
  /// this will return the MemoryPhi, not the operand. This means that
  /// given:
  /// if (a) {
  ///   1 = MemoryDef(liveOnEntry)
  ///   store %a
  /// } else {
  ///   2 = MemoryDef(liveOnEntry)
  ///    store %b
  /// }
  /// 3 = MemoryPhi(2, 1)
  /// MemoryUse(3)
  /// load %a
  ///
  /// calling this API on load(%a) will return the MemoryPhi, not the MemoryDef
  /// in the if (a) branch.
  virtual MemoryAccess *getClobberingMemoryAccess(const Instruction *) = 0;

  /// \brief Given a potentially clobbering memory access and a new location,
  /// calling this will give you the nearest dominating clobbering MemoryAccess
  /// (by skipping non-aliasing def links).
  ///
  /// This version of the function is mainly used to disambiguate phi translated
  /// pointers, where the value of a pointer may have changed from the initial
  /// memory access. Note that this expects to be handed either a MemoryUse,
  /// or an already potentially clobbering access. Unlike the above API, if
  /// given a MemoryDef that clobbers the pointer as the starting access, it
  /// will return that MemoryDef, whereas the above would return the clobber
  /// starting from the use side of  the memory def.
  virtual MemoryAccess *getClobberingMemoryAccess(MemoryAccess *,
                                                  MemoryLocation &) = 0;

protected:
  MemorySSA *MSSA;
};

/// \brief A MemorySSAWalker that does no alias queries, or anything else. It
/// simply returns the links as they were constructed by the builder.
class DoNothingMemorySSAWalker final : public MemorySSAWalker {
public:
  MemoryAccess *getClobberingMemoryAccess(const Instruction *) override;
  MemoryAccess *getClobberingMemoryAccess(MemoryAccess *,
                                          MemoryLocation &) override;
};

using MemoryAccessPair = std::pair<MemoryAccess *, MemoryLocation>;
using ConstMemoryAccessPair = std::pair<const MemoryAccess *, MemoryLocation>;

/// \brief A MemorySSAWalker that does AA walks and caching of lookups to
/// disambiguate accesses.
class CachingMemorySSAWalker final : public MemorySSAWalker {
public:
  CachingMemorySSAWalker(MemorySSA *, AliasAnalysis *, DominatorTree *);
  virtual ~CachingMemorySSAWalker();
  MemoryAccess *getClobberingMemoryAccess(const Instruction *) override;
  MemoryAccess *getClobberingMemoryAccess(MemoryAccess *,
                                          MemoryLocation &) override;

protected:
  struct UpwardsMemoryQuery;
  MemoryAccess *doCacheLookup(const MemoryAccess *, const UpwardsMemoryQuery &,
                              const MemoryLocation &);

  void doCacheInsert(const MemoryAccess *, MemoryAccess *,
                     const UpwardsMemoryQuery &, const MemoryLocation &);

  void doCacheRemove(const MemoryAccess *, const UpwardsMemoryQuery &,
                     const MemoryLocation &);

private:
  MemoryAccessPair UpwardsDFSWalk(MemoryAccess *, const MemoryLocation &,
                                  UpwardsMemoryQuery &, bool);
  MemoryAccess *getClobberingMemoryAccess(MemoryAccess *, UpwardsMemoryQuery &);
  bool instructionClobbersQuery(const MemoryDef *, UpwardsMemoryQuery &,
                                const MemoryLocation &Loc) const;
  SmallDenseMap<ConstMemoryAccessPair, MemoryAccess *>
      CachedUpwardsClobberingAccess;
  DenseMap<const MemoryAccess *, MemoryAccess *> CachedUpwardsClobberingCall;
  AliasAnalysis *AA;
  DominatorTree *DT;
};

/// \brief Iterator base class used to implement const and non-const iterators
/// over the defining accesses of a MemoryAccess.
template <class T>
class memoryaccess_def_iterator_base
    : public iterator_facade_base<memoryaccess_def_iterator_base<T>,
                                  std::forward_iterator_tag, T, ptrdiff_t, T *,
                                  T *> {
  using BaseT = typename memoryaccess_def_iterator_base::iterator_facade_base;

public:
  memoryaccess_def_iterator_base(T *Start) : Access(Start), ArgNo(0) {}
  memoryaccess_def_iterator_base() : Access(nullptr), ArgNo(0) {}
  bool operator==(const memoryaccess_def_iterator_base &Other) const {
    return Access == Other.Access && (!Access || ArgNo == Other.ArgNo);
  }

  // This is a bit ugly, but for MemoryPHI's, unlike PHINodes, you can't get the
  // block from the operand in constant time (In a PHINode, the uselist has
  // both, so it's just subtraction). We provide it as part of the
  // iterator to avoid callers having to linear walk to get the block.
  // If the operation becomes constant time on MemoryPHI's, this bit of
  // abstraction breaking should be removed.
  BasicBlock *getPhiArgBlock() const {
    MemoryPhi *MP = dyn_cast<MemoryPhi>(Access);
    assert(MP && "Tried to get phi arg block when not iterating over a PHI");
    return MP->getIncomingBlock(ArgNo);
  }
  typename BaseT::iterator::pointer operator*() const {
    assert(Access && "Tried to access past the end of our iterator");
    // Go to the first argument for phis, and the defining access for everything
    // else.
    if (MemoryPhi *MP = dyn_cast<MemoryPhi>(Access))
      return MP->getIncomingValue(ArgNo);
    return cast<MemoryUseOrDef>(Access)->getDefiningAccess();
  }
  using BaseT::operator++;
  memoryaccess_def_iterator &operator++() {
    assert(Access && "Hit end of iterator");
    if (MemoryPhi *MP = dyn_cast<MemoryPhi>(Access)) {
      if (++ArgNo >= MP->getNumIncomingValues()) {
        ArgNo = 0;
        Access = nullptr;
      }
    } else {
      Access = nullptr;
    }
    return *this;
  }

private:
  T *Access;
  unsigned ArgNo;
};

inline memoryaccess_def_iterator MemoryAccess::defs_begin() {
  return memoryaccess_def_iterator(this);
}
inline const_memoryaccess_def_iterator MemoryAccess::defs_begin() const {
  return const_memoryaccess_def_iterator(this);
}

inline memoryaccess_def_iterator MemoryAccess::defs_end() {
  return memoryaccess_def_iterator();
}

inline const_memoryaccess_def_iterator MemoryAccess::defs_end() const {
  return const_memoryaccess_def_iterator();
}

/// \brief GraphTraits for a MemoryAccess, which walks defs in the normal case,
/// and uses in the inverse case.
template <> struct GraphTraits<MemoryAccess *> {
  using NodeType = MemoryAccess;
  using ChildIteratorType = memoryaccess_def_iterator;

  static NodeType *getEntryNode(NodeType *N) { return N; }
  static inline ChildIteratorType child_begin(NodeType *N) {
    return N->defs_begin();
  }
  static inline ChildIteratorType child_end(NodeType *N) {
    return N->defs_end();
  }
};

template <> struct GraphTraits<Inverse<MemoryAccess *>> {
  using NodeType = MemoryAccess;
  using ChildIteratorType = MemoryAccess::iterator;

  static NodeType *getEntryNode(NodeType *N) { return N; }
  static inline ChildIteratorType child_begin(NodeType *N) {
    return N->user_begin();
  }
  static inline ChildIteratorType child_end(NodeType *N) {
    return N->user_end();
  }
};

/// \brief Provide an iterator that walks defs, giving both the memory access,
/// and the current pointer location, updating the pointer location as it
/// changes due to phi node translation.
///
/// This iterator, while somewhat specialized, is what most clients actually
/// want when walking upwards through MemorySSA def chains. It takes a pair of
/// <MemoryAccess,MemoryLocation>, and walks defs, properly translating the
/// memory location through phi nodes for the user.
class upward_defs_iterator
    : public iterator_facade_base<upward_defs_iterator,
                                  std::forward_iterator_tag,
                                  const MemoryAccessPair> {
  using BaseT = upward_defs_iterator::iterator_facade_base;

public:
  upward_defs_iterator(const MemoryAccessPair &Info)
      : DefIterator(Info.first), Location(Info.second),
        OriginalAccess(Info.first) {
    CurrentPair.first = nullptr;

    WalkingPhi = Info.first && isa<MemoryPhi>(Info.first);
    fillInCurrentPair();
  }

  upward_defs_iterator()
      : DefIterator(), Location(), OriginalAccess(), WalkingPhi(false) {
    CurrentPair.first = nullptr;
  }

  bool operator==(const upward_defs_iterator &Other) const {
    return DefIterator == Other.DefIterator;
  }

  BaseT::iterator::reference operator*() const {
    assert(DefIterator != OriginalAccess->defs_end() &&
           "Tried to access past the end of our iterator");
    return CurrentPair;
  }

  using BaseT::operator++;
  upward_defs_iterator &operator++() {
    assert(DefIterator != OriginalAccess->defs_end() &&
           "Tried to access past the end of the iterator");
    ++DefIterator;
    if (DefIterator != OriginalAccess->defs_end())
      fillInCurrentPair();
    return *this;
  }

  BasicBlock *getPhiArgBlock() const { return DefIterator.getPhiArgBlock(); }

private:
  void fillInCurrentPair() {
    CurrentPair.first = *DefIterator;
    if (WalkingPhi && Location.Ptr) {
      PHITransAddr Translator(
          const_cast<Value *>(Location.Ptr),
          OriginalAccess->getBlock()->getModule()->getDataLayout(), nullptr);
      if (!Translator.PHITranslateValue(OriginalAccess->getBlock(),
                                        DefIterator.getPhiArgBlock(), nullptr,
                                        false))
        if (Translator.getAddr() != Location.Ptr) {
          CurrentPair.second = Location.getWithNewPtr(Translator.getAddr());
          return;
        }
    }
    CurrentPair.second = Location;
  }

  MemoryAccessPair CurrentPair;
  memoryaccess_def_iterator DefIterator;
  MemoryLocation Location;
  MemoryAccess *OriginalAccess;
  bool WalkingPhi;
};

inline upward_defs_iterator upward_defs_begin(const MemoryAccessPair &Pair) {
  return upward_defs_iterator(Pair);
}
inline upward_defs_iterator upward_defs_end() { return upward_defs_iterator(); }
}

#endif
