//===-- LoopUnroll.cpp - Loop unroller pass -------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass implements a simple loop unroller.  It works best when loops have
// been canonicalized by the -indvars pass, allowing it to determine the trip
// counts of loops easily.
//===----------------------------------------------------------------------===//

#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/LoopUnrollAnalyzer.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include <climits>

using namespace llvm;

#define DEBUG_TYPE "loop-unroll"

static cl::opt<unsigned>
    UnrollThreshold("unroll-threshold", cl::Hidden,
                    cl::desc("The baseline cost threshold for loop unrolling"));

static cl::opt<unsigned> UnrollPercentDynamicCostSavedThreshold(
    "unroll-percent-dynamic-cost-saved-threshold", cl::Hidden,
    cl::desc("The percentage of estimated dynamic cost which must be saved by "
             "unrolling to allow unrolling up to the max threshold."));

static cl::opt<unsigned> UnrollDynamicCostSavingsDiscount(
    "unroll-dynamic-cost-savings-discount", cl::Hidden,
    cl::desc("This is the amount discounted from the total unroll cost when "
             "the unrolled form has a high dynamic cost savings (triggered by "
             "the '-unroll-perecent-dynamic-cost-saved-threshold' flag)."));

static cl::opt<unsigned> UnrollMaxIterationsCountToAnalyze(
    "unroll-max-iteration-count-to-analyze", cl::init(0), cl::Hidden,
    cl::desc("Don't allow loop unrolling to simulate more than this number of"
             "iterations when checking full unroll profitability"));

static cl::opt<unsigned> UnrollCount(
    "unroll-count", cl::Hidden,
    cl::desc("Use this unroll count for all loops including those with "
             "unroll_count pragma values, for testing purposes"));

static cl::opt<unsigned> UnrollMaxCount(
    "unroll-max-count", cl::Hidden,
    cl::desc("Set the max unroll count for partial and runtime unrolling, for"
             "testing purposes"));

static cl::opt<unsigned> UnrollFullMaxCount(
    "unroll-full-max-count", cl::Hidden,
    cl::desc(
        "Set the max unroll count for full unrolling, for testing purposes"));

static cl::opt<bool>
    UnrollAllowPartial("unroll-allow-partial", cl::Hidden,
                       cl::desc("Allows loops to be partially unrolled until "
                                "-unroll-threshold loop size is reached."));

static cl::opt<bool>
    UnrollRuntime("unroll-runtime", cl::ZeroOrMore, cl::Hidden,
                  cl::desc("Unroll loops with run-time trip counts"));

static cl::opt<unsigned> PragmaUnrollThreshold(
    "pragma-unroll-threshold", cl::init(16 * 1024), cl::Hidden,
    cl::desc("Unrolled size limit for loops with an unroll(full) or "
             "unroll_count pragma."));

/// A magic value for use with the Threshold parameter to indicate
/// that the loop unroll should be performed regardless of how much
/// code expansion would result.
static const unsigned NoThreshold = UINT_MAX;

/// Default unroll count for loops with run-time trip count if
/// -unroll-count is not set
static const unsigned DefaultUnrollRuntimeCount = 8;

/// Gather the various unrolling parameters based on the defaults, compiler
/// flags, TTI overrides, pragmas, and user specified parameters.
static TargetTransformInfo::UnrollingPreferences gatherUnrollingPreferences(
    Loop *L, const TargetTransformInfo &TTI, Optional<unsigned> UserThreshold,
    Optional<unsigned> UserCount, Optional<bool> UserAllowPartial,
    Optional<bool> UserRuntime, unsigned PragmaCount, bool PragmaFullUnroll,
    bool PragmaEnableUnroll, unsigned TripCount) {
  TargetTransformInfo::UnrollingPreferences UP;

  // Set up the defaults
  UP.Threshold = 150;
  UP.PercentDynamicCostSavedThreshold = 20;
  UP.DynamicCostSavingsDiscount = 2000;
  UP.OptSizeThreshold = 0;
  UP.PartialThreshold = UP.Threshold;
  UP.PartialOptSizeThreshold = 0;
  UP.Count = 0;
  UP.MaxCount = UINT_MAX;
  UP.FullUnrollMaxCount = UINT_MAX;
  UP.Partial = false;
  UP.Runtime = false;
  UP.AllowExpensiveTripCount = false;

  // Override with any target specific settings
  TTI.getUnrollingPreferences(L, UP);

  // Apply size attributes
  if (L->getHeader()->getParent()->optForSize()) {
    UP.Threshold = UP.OptSizeThreshold;
    UP.PartialThreshold = UP.PartialOptSizeThreshold;
  }

  // Apply unroll count pragmas
  if (PragmaCount)
    UP.Count = PragmaCount;
  else if (PragmaFullUnroll)
    UP.Count = TripCount;

  // Apply any user values specified by cl::opt
  if (UnrollThreshold.getNumOccurrences() > 0) {
    UP.Threshold = UnrollThreshold;
    UP.PartialThreshold = UnrollThreshold;
  }
  if (UnrollPercentDynamicCostSavedThreshold.getNumOccurrences() > 0)
    UP.PercentDynamicCostSavedThreshold =
        UnrollPercentDynamicCostSavedThreshold;
  if (UnrollDynamicCostSavingsDiscount.getNumOccurrences() > 0)
    UP.DynamicCostSavingsDiscount = UnrollDynamicCostSavingsDiscount;
  if (UnrollCount.getNumOccurrences() > 0)
    UP.Count = UnrollCount;
  if (UnrollMaxCount.getNumOccurrences() > 0)
    UP.MaxCount = UnrollMaxCount;
  if (UnrollFullMaxCount.getNumOccurrences() > 0)
    UP.FullUnrollMaxCount = UnrollFullMaxCount;
  if (UnrollAllowPartial.getNumOccurrences() > 0)
    UP.Partial = UnrollAllowPartial;
  if (UnrollRuntime.getNumOccurrences() > 0)
    UP.Runtime = UnrollRuntime;

  // Apply user values provided by argument
  if (UserThreshold.hasValue()) {
    UP.Threshold = *UserThreshold;
    UP.PartialThreshold = *UserThreshold;
  }
  if (UserCount.hasValue())
    UP.Count = *UserCount;
  if (UserAllowPartial.hasValue())
    UP.Partial = *UserAllowPartial;
  if (UserRuntime.hasValue())
    UP.Runtime = *UserRuntime;

  if (PragmaCount > 0 ||
      ((PragmaFullUnroll || PragmaEnableUnroll) && TripCount != 0)) {
    // If the loop has an unrolling pragma, we want to be more aggressive with
    // unrolling limits. Set thresholds to at least the PragmaTheshold value
    // which is larger than the default limits.
    if (UP.Threshold != NoThreshold)
      UP.Threshold = std::max<unsigned>(UP.Threshold, PragmaUnrollThreshold);
    if (UP.PartialThreshold != NoThreshold)
      UP.PartialThreshold =
          std::max<unsigned>(UP.PartialThreshold, PragmaUnrollThreshold);
  }

  return UP;
}

namespace {
/// A struct to densely store the state of an instruction after unrolling at
/// each iteration.
///
/// This is designed to work like a tuple of <Instruction *, int> for the
/// purposes of hashing and lookup, but to be able to associate two boolean
/// states with each key.
struct UnrolledInstState {
  Instruction *I;
  int Iteration : 30;
  unsigned IsFree : 1;
  unsigned IsCounted : 1;
};

/// Hashing and equality testing for a set of the instruction states.
struct UnrolledInstStateKeyInfo {
  typedef DenseMapInfo<Instruction *> PtrInfo;
  typedef DenseMapInfo<std::pair<Instruction *, int>> PairInfo;
  static inline UnrolledInstState getEmptyKey() {
    return {PtrInfo::getEmptyKey(), 0, 0, 0};
  }
  static inline UnrolledInstState getTombstoneKey() {
    return {PtrInfo::getTombstoneKey(), 0, 0, 0};
  }
  static inline unsigned getHashValue(const UnrolledInstState &S) {
    return PairInfo::getHashValue({S.I, S.Iteration});
  }
  static inline bool isEqual(const UnrolledInstState &LHS,
                             const UnrolledInstState &RHS) {
    return PairInfo::isEqual({LHS.I, LHS.Iteration}, {RHS.I, RHS.Iteration});
  }
};
}

namespace {
struct EstimatedUnrollCost {
  /// \brief The estimated cost after unrolling.
  int UnrolledCost;

  /// \brief The estimated dynamic cost of executing the instructions in the
  /// rolled form.
  int RolledDynamicCost;
};
}

/// \brief Figure out if the loop is worth full unrolling.
///
/// Complete loop unrolling can make some loads constant, and we need to know
/// if that would expose any further optimization opportunities.  This routine
/// estimates this optimization.  It computes cost of unrolled loop
/// (UnrolledCost) and dynamic cost of the original loop (RolledDynamicCost). By
/// dynamic cost we mean that we won't count costs of blocks that are known not
/// to be executed (i.e. if we have a branch in the loop and we know that at the
/// given iteration its condition would be resolved to true, we won't add up the
/// cost of the 'false'-block).
/// \returns Optional value, holding the RolledDynamicCost and UnrolledCost. If
/// the analysis failed (no benefits expected from the unrolling, or the loop is
/// too big to analyze), the returned value is None.
static Optional<EstimatedUnrollCost>
analyzeLoopUnrollCost(const Loop *L, unsigned TripCount, DominatorTree &DT,
                      ScalarEvolution &SE, const TargetTransformInfo &TTI,
                      int MaxUnrolledLoopSize) {
  // We want to be able to scale offsets by the trip count and add more offsets
  // to them without checking for overflows, and we already don't want to
  // analyze *massive* trip counts, so we force the max to be reasonably small.
  assert(UnrollMaxIterationsCountToAnalyze < (INT_MAX / 2) &&
         "The unroll iterations max is too large!");

  // Only analyze inner loops. We can't properly estimate cost of nested loops
  // and we won't visit inner loops again anyway.
  if (!L->empty())
    return None;

  // Don't simulate loops with a big or unknown tripcount
  if (!UnrollMaxIterationsCountToAnalyze || !TripCount ||
      TripCount > UnrollMaxIterationsCountToAnalyze)
    return None;

  SmallSetVector<BasicBlock *, 16> BBWorklist;
  SmallSetVector<std::pair<BasicBlock *, BasicBlock *>, 4> ExitWorklist;
  DenseMap<Value *, Constant *> SimplifiedValues;
  SmallVector<std::pair<Value *, Constant *>, 4> SimplifiedInputValues;

  // The estimated cost of the unrolled form of the loop. We try to estimate
  // this by simplifying as much as we can while computing the estimate.
  int UnrolledCost = 0;

  // We also track the estimated dynamic (that is, actually executed) cost in
  // the rolled form. This helps identify cases when the savings from unrolling
  // aren't just exposing dead control flows, but actual reduced dynamic
  // instructions due to the simplifications which we expect to occur after
  // unrolling.
  int RolledDynamicCost = 0;

  // We track the simplification of each instruction in each iteration. We use
  // this to recursively merge costs into the unrolled cost on-demand so that
  // we don't count the cost of any dead code. This is essentially a map from
  // <instruction, int> to <bool, bool>, but stored as a densely packed struct.
  DenseSet<UnrolledInstState, UnrolledInstStateKeyInfo> InstCostMap;

  // A small worklist used to accumulate cost of instructions from each
  // observable and reached root in the loop.
  SmallVector<Instruction *, 16> CostWorklist;

  // PHI-used worklist used between iterations while accumulating cost.
  SmallVector<Instruction *, 4> PHIUsedList;

  // Helper function to accumulate cost for instructions in the loop.
  auto AddCostRecursively = [&](Instruction &RootI, int Iteration) {
    assert(Iteration >= 0 && "Cannot have a negative iteration!");
    assert(CostWorklist.empty() && "Must start with an empty cost list");
    assert(PHIUsedList.empty() && "Must start with an empty phi used list");
    CostWorklist.push_back(&RootI);
    for (;; --Iteration) {
      do {
        Instruction *I = CostWorklist.pop_back_val();

        // InstCostMap only uses I and Iteration as a key, the other two values
        // don't matter here.
        auto CostIter = InstCostMap.find({I, Iteration, 0, 0});
        if (CostIter == InstCostMap.end())
          // If an input to a PHI node comes from a dead path through the loop
          // we may have no cost data for it here. What that actually means is
          // that it is free.
          continue;
        auto &Cost = *CostIter;
        if (Cost.IsCounted)
          // Already counted this instruction.
          continue;

        // Mark that we are counting the cost of this instruction now.
        Cost.IsCounted = true;

        // If this is a PHI node in the loop header, just add it to the PHI set.
        if (auto *PhiI = dyn_cast<PHINode>(I))
          if (PhiI->getParent() == L->getHeader()) {
            assert(Cost.IsFree && "Loop PHIs shouldn't be evaluated as they "
                                  "inherently simplify during unrolling.");
            if (Iteration == 0)
              continue;

            // Push the incoming value from the backedge into the PHI used list
            // if it is an in-loop instruction. We'll use this to populate the
            // cost worklist for the next iteration (as we count backwards).
            if (auto *OpI = dyn_cast<Instruction>(
                    PhiI->getIncomingValueForBlock(L->getLoopLatch())))
              if (L->contains(OpI))
                PHIUsedList.push_back(OpI);
            continue;
          }

        // First accumulate the cost of this instruction.
        if (!Cost.IsFree) {
          UnrolledCost += TTI.getUserCost(I);
          DEBUG(dbgs() << "Adding cost of instruction (iteration " << Iteration
                       << "): ");
          DEBUG(I->dump());
        }

        // We must count the cost of every operand which is not free,
        // recursively. If we reach a loop PHI node, simply add it to the set
        // to be considered on the next iteration (backwards!).
        for (Value *Op : I->operands()) {
          // Check whether this operand is free due to being a constant or
          // outside the loop.
          auto *OpI = dyn_cast<Instruction>(Op);
          if (!OpI || !L->contains(OpI))
            continue;

          // Otherwise accumulate its cost.
          CostWorklist.push_back(OpI);
        }
      } while (!CostWorklist.empty());

      if (PHIUsedList.empty())
        // We've exhausted the search.
        break;

      assert(Iteration > 0 &&
             "Cannot track PHI-used values past the first iteration!");
      CostWorklist.append(PHIUsedList.begin(), PHIUsedList.end());
      PHIUsedList.clear();
    }
  };

  // Ensure that we don't violate the loop structure invariants relied on by
  // this analysis.
  assert(L->isLoopSimplifyForm() && "Must put loop into normal form first.");
  assert(L->isLCSSAForm(DT) &&
         "Must have loops in LCSSA form to track live-out values.");

  DEBUG(dbgs() << "Starting LoopUnroll profitability analysis...\n");

  // Simulate execution of each iteration of the loop counting instructions,
  // which would be simplified.
  // Since the same load will take different values on different iterations,
  // we literally have to go through all loop's iterations.
  for (unsigned Iteration = 0; Iteration < TripCount; ++Iteration) {
    DEBUG(dbgs() << " Analyzing iteration " << Iteration << "\n");

    // Prepare for the iteration by collecting any simplified entry or backedge
    // inputs.
    for (Instruction &I : *L->getHeader()) {
      auto *PHI = dyn_cast<PHINode>(&I);
      if (!PHI)
        break;

      // The loop header PHI nodes must have exactly two input: one from the
      // loop preheader and one from the loop latch.
      assert(
          PHI->getNumIncomingValues() == 2 &&
          "Must have an incoming value only for the preheader and the latch.");

      Value *V = PHI->getIncomingValueForBlock(
          Iteration == 0 ? L->getLoopPreheader() : L->getLoopLatch());
      Constant *C = dyn_cast<Constant>(V);
      if (Iteration != 0 && !C)
        C = SimplifiedValues.lookup(V);
      if (C)
        SimplifiedInputValues.push_back({PHI, C});
    }

    // Now clear and re-populate the map for the next iteration.
    SimplifiedValues.clear();
    while (!SimplifiedInputValues.empty())
      SimplifiedValues.insert(SimplifiedInputValues.pop_back_val());

    UnrolledInstAnalyzer Analyzer(Iteration, SimplifiedValues, SE, L);

    BBWorklist.clear();
    BBWorklist.insert(L->getHeader());
    // Note that we *must not* cache the size, this loop grows the worklist.
    for (unsigned Idx = 0; Idx != BBWorklist.size(); ++Idx) {
      BasicBlock *BB = BBWorklist[Idx];

      // Visit all instructions in the given basic block and try to simplify
      // it.  We don't change the actual IR, just count optimization
      // opportunities.
      for (Instruction &I : *BB) {
        // Track this instruction's expected baseline cost when executing the
        // rolled loop form.
        RolledDynamicCost += TTI.getUserCost(&I);

        // Visit the instruction to analyze its loop cost after unrolling,
        // and if the visitor returns true, mark the instruction as free after
        // unrolling and continue.
        bool IsFree = Analyzer.visit(I);
        bool Inserted = InstCostMap.insert({&I, (int)Iteration,
                                           (unsigned)IsFree,
                                           /*IsCounted*/ false}).second;
        (void)Inserted;
        assert(Inserted && "Cannot have a state for an unvisited instruction!");

        if (IsFree)
          continue;

        // If the instruction might have a side-effect recursively account for
        // the cost of it and all the instructions leading up to it.
        if (I.mayHaveSideEffects())
          AddCostRecursively(I, Iteration);

        // Can't properly model a cost of a call.
        // FIXME: With a proper cost model we should be able to do it.
        if(isa<CallInst>(&I))
          return None;

        // If unrolled body turns out to be too big, bail out.
        if (UnrolledCost > MaxUnrolledLoopSize) {
          DEBUG(dbgs() << "  Exceeded threshold.. exiting.\n"
                       << "  UnrolledCost: " << UnrolledCost
                       << ", MaxUnrolledLoopSize: " << MaxUnrolledLoopSize
                       << "\n");
          return None;
        }
      }

      TerminatorInst *TI = BB->getTerminator();

      // Add in the live successors by first checking whether we have terminator
      // that may be simplified based on the values simplified by this call.
      if (BranchInst *BI = dyn_cast<BranchInst>(TI)) {
        if (BI->isConditional()) {
          if (Constant *SimpleCond =
                  SimplifiedValues.lookup(BI->getCondition())) {
            BasicBlock *Succ = nullptr;
            // Just take the first successor if condition is undef
            if (isa<UndefValue>(SimpleCond))
              Succ = BI->getSuccessor(0);
            else
              Succ = BI->getSuccessor(
                  cast<ConstantInt>(SimpleCond)->isZero() ? 1 : 0);
            if (L->contains(Succ))
              BBWorklist.insert(Succ);
            else
              ExitWorklist.insert({BB, Succ});
            continue;
          }
        }
      } else if (SwitchInst *SI = dyn_cast<SwitchInst>(TI)) {
        if (Constant *SimpleCond =
                SimplifiedValues.lookup(SI->getCondition())) {
          BasicBlock *Succ = nullptr;
          // Just take the first successor if condition is undef
          if (isa<UndefValue>(SimpleCond))
            Succ = SI->getSuccessor(0);
          else
            Succ = SI->findCaseValue(cast<ConstantInt>(SimpleCond))
                       .getCaseSuccessor();
          if (L->contains(Succ))
            BBWorklist.insert(Succ);
          else
            ExitWorklist.insert({BB, Succ});
          continue;
        }
      }

      // Add BB's successors to the worklist.
      for (BasicBlock *Succ : successors(BB))
        if (L->contains(Succ))
          BBWorklist.insert(Succ);
        else
          ExitWorklist.insert({BB, Succ});
      AddCostRecursively(*TI, Iteration);
    }

    // If we found no optimization opportunities on the first iteration, we
    // won't find them on later ones too.
    if (UnrolledCost == RolledDynamicCost) {
      DEBUG(dbgs() << "  No opportunities found.. exiting.\n"
                   << "  UnrolledCost: " << UnrolledCost << "\n");
      return None;
    }
  }

  while (!ExitWorklist.empty()) {
    BasicBlock *ExitingBB, *ExitBB;
    std::tie(ExitingBB, ExitBB) = ExitWorklist.pop_back_val();

    for (Instruction &I : *ExitBB) {
      auto *PN = dyn_cast<PHINode>(&I);
      if (!PN)
        break;

      Value *Op = PN->getIncomingValueForBlock(ExitingBB);
      if (auto *OpI = dyn_cast<Instruction>(Op))
        if (L->contains(OpI))
          AddCostRecursively(*OpI, TripCount - 1);
    }
  }

  DEBUG(dbgs() << "Analysis finished:\n"
               << "UnrolledCost: " << UnrolledCost << ", "
               << "RolledDynamicCost: " << RolledDynamicCost << "\n");
  return {{UnrolledCost, RolledDynamicCost}};
}

/// ApproximateLoopSize - Approximate the size of the loop.
static unsigned ApproximateLoopSize(const Loop *L, unsigned &NumCalls,
                                    bool &NotDuplicatable, bool &Convergent,
                                    const TargetTransformInfo &TTI,
                                    AssumptionCache *AC) {
  SmallPtrSet<const Value *, 32> EphValues;
  CodeMetrics::collectEphemeralValues(L, AC, EphValues);

  CodeMetrics Metrics;
  for (BasicBlock *BB : L->blocks())
    Metrics.analyzeBasicBlock(BB, TTI, EphValues);
  NumCalls = Metrics.NumInlineCandidates;
  NotDuplicatable = Metrics.notDuplicatable;
  Convergent = Metrics.convergent;

  unsigned LoopSize = Metrics.NumInsts;

  // Don't allow an estimate of size zero.  This would allows unrolling of loops
  // with huge iteration counts, which is a compile time problem even if it's
  // not a problem for code quality. Also, the code using this size may assume
  // that each loop has at least three instructions (likely a conditional
  // branch, a comparison feeding that branch, and some kind of loop increment
  // feeding that comparison instruction).
  LoopSize = std::max(LoopSize, 3u);

  return LoopSize;
}

// Returns the loop hint metadata node with the given name (for example,
// "llvm.loop.unroll.count").  If no such metadata node exists, then nullptr is
// returned.
static MDNode *GetUnrollMetadataForLoop(const Loop *L, StringRef Name) {
  if (MDNode *LoopID = L->getLoopID())
    return GetUnrollMetadata(LoopID, Name);
  return nullptr;
}

// Returns true if the loop has an unroll(full) pragma.
static bool HasUnrollFullPragma(const Loop *L) {
  return GetUnrollMetadataForLoop(L, "llvm.loop.unroll.full");
}

// Returns true if the loop has an unroll(enable) pragma. This metadata is used
// for both "#pragma unroll" and "#pragma clang loop unroll(enable)" directives.
static bool HasUnrollEnablePragma(const Loop *L) {
  return GetUnrollMetadataForLoop(L, "llvm.loop.unroll.enable");
}

// Returns true if the loop has an unroll(disable) pragma.
static bool HasUnrollDisablePragma(const Loop *L) {
  return GetUnrollMetadataForLoop(L, "llvm.loop.unroll.disable");
}

// Returns true if the loop has an runtime unroll(disable) pragma.
static bool HasRuntimeUnrollDisablePragma(const Loop *L) {
  return GetUnrollMetadataForLoop(L, "llvm.loop.unroll.runtime.disable");
}

// If loop has an unroll_count pragma return the (necessarily
// positive) value from the pragma.  Otherwise return 0.
static unsigned UnrollCountPragmaValue(const Loop *L) {
  MDNode *MD = GetUnrollMetadataForLoop(L, "llvm.loop.unroll.count");
  if (MD) {
    assert(MD->getNumOperands() == 2 &&
           "Unroll count hint metadata should have two operands.");
    unsigned Count =
        mdconst::extract<ConstantInt>(MD->getOperand(1))->getZExtValue();
    assert(Count >= 1 && "Unroll count must be positive.");
    return Count;
  }
  return 0;
}

// Remove existing unroll metadata and add unroll disable metadata to
// indicate the loop has already been unrolled.  This prevents a loop
// from being unrolled more than is directed by a pragma if the loop
// unrolling pass is run more than once (which it generally is).
static void SetLoopAlreadyUnrolled(Loop *L) {
  MDNode *LoopID = L->getLoopID();
  if (!LoopID)
    return;

  // First remove any existing loop unrolling metadata.
  SmallVector<Metadata *, 4> MDs;
  // Reserve first location for self reference to the LoopID metadata node.
  MDs.push_back(nullptr);
  for (unsigned i = 1, ie = LoopID->getNumOperands(); i < ie; ++i) {
    bool IsUnrollMetadata = false;
    MDNode *MD = dyn_cast<MDNode>(LoopID->getOperand(i));
    if (MD) {
      const MDString *S = dyn_cast<MDString>(MD->getOperand(0));
      IsUnrollMetadata = S && S->getString().startswith("llvm.loop.unroll.");
    }
    if (!IsUnrollMetadata)
      MDs.push_back(LoopID->getOperand(i));
  }

  // Add unroll(disable) metadata to disable future unrolling.
  LLVMContext &Context = L->getHeader()->getContext();
  SmallVector<Metadata *, 1> DisableOperands;
  DisableOperands.push_back(MDString::get(Context, "llvm.loop.unroll.disable"));
  MDNode *DisableNode = MDNode::get(Context, DisableOperands);
  MDs.push_back(DisableNode);

  MDNode *NewLoopID = MDNode::get(Context, MDs);
  // Set operand 0 to refer to the loop id itself.
  NewLoopID->replaceOperandWith(0, NewLoopID);
  L->setLoopID(NewLoopID);
}

static bool canUnrollCompletely(Loop *L, unsigned Threshold,
                                unsigned PercentDynamicCostSavedThreshold,
                                unsigned DynamicCostSavingsDiscount,
                                uint64_t UnrolledCost,
                                uint64_t RolledDynamicCost) {
  if (Threshold == NoThreshold) {
    DEBUG(dbgs() << "  Can fully unroll, because no threshold is set.\n");
    return true;
  }

  if (UnrolledCost <= Threshold) {
    DEBUG(dbgs() << "  Can fully unroll, because unrolled cost: "
                 << UnrolledCost << "<" << Threshold << "\n");
    return true;
  }

  assert(UnrolledCost && "UnrolledCost can't be 0 at this point.");
  assert(RolledDynamicCost >= UnrolledCost &&
         "Cannot have a higher unrolled cost than a rolled cost!");

  // Compute the percentage of the dynamic cost in the rolled form that is
  // saved when unrolled. If unrolling dramatically reduces the estimated
  // dynamic cost of the loop, we use a higher threshold to allow more
  // unrolling.
  unsigned PercentDynamicCostSaved =
      (uint64_t)(RolledDynamicCost - UnrolledCost) * 100ull / RolledDynamicCost;

  if (PercentDynamicCostSaved >= PercentDynamicCostSavedThreshold &&
      (int64_t)UnrolledCost - (int64_t)DynamicCostSavingsDiscount <=
          (int64_t)Threshold) {
    DEBUG(dbgs() << "  Can fully unroll, because unrolling will reduce the "
                    "expected dynamic cost by "
                 << PercentDynamicCostSaved << "% (threshold: "
                 << PercentDynamicCostSavedThreshold << "%)\n"
                 << "  and the unrolled cost (" << UnrolledCost
                 << ") is less than the max threshold ("
                 << DynamicCostSavingsDiscount << ").\n");
    return true;
  }

  DEBUG(dbgs() << "  Too large to fully unroll:\n");
  DEBUG(dbgs() << "    Threshold: " << Threshold << "\n");
  DEBUG(dbgs() << "    Max threshold: " << DynamicCostSavingsDiscount << "\n");
  DEBUG(dbgs() << "    Percent cost saved threshold: "
               << PercentDynamicCostSavedThreshold << "%\n");
  DEBUG(dbgs() << "    Unrolled cost: " << UnrolledCost << "\n");
  DEBUG(dbgs() << "    Rolled dynamic cost: " << RolledDynamicCost << "\n");
  DEBUG(dbgs() << "    Percent cost saved: " << PercentDynamicCostSaved
               << "\n");
  return false;
}

static bool tryToUnrollLoop(Loop *L, DominatorTree &DT, LoopInfo *LI,
                            ScalarEvolution *SE, const TargetTransformInfo &TTI,
                            AssumptionCache &AC, bool PreserveLCSSA,
                            Optional<unsigned> ProvidedCount,
                            Optional<unsigned> ProvidedThreshold,
                            Optional<bool> ProvidedAllowPartial,
                            Optional<bool> ProvidedRuntime) {
  BasicBlock *Header = L->getHeader();
  DEBUG(dbgs() << "Loop Unroll: F[" << Header->getParent()->getName()
               << "] Loop %" << Header->getName() << "\n");

  if (HasUnrollDisablePragma(L)) {
    return false;
  }
  bool PragmaFullUnroll = HasUnrollFullPragma(L);
  bool PragmaEnableUnroll = HasUnrollEnablePragma(L);
  unsigned PragmaCount = UnrollCountPragmaValue(L);
  bool HasPragma = PragmaFullUnroll || PragmaEnableUnroll || PragmaCount > 0;

  // Find trip count and trip multiple if count is not available
  unsigned TripCount = 0;
  unsigned TripMultiple = 1;
  // If there are multiple exiting blocks but one of them is the latch, use the
  // latch for the trip count estimation. Otherwise insist on a single exiting
  // block for the trip count estimation.
  BasicBlock *ExitingBlock = L->getLoopLatch();
  if (!ExitingBlock || !L->isLoopExiting(ExitingBlock))
    ExitingBlock = L->getExitingBlock();
  if (ExitingBlock) {
    TripCount = SE->getSmallConstantTripCount(L, ExitingBlock);
    TripMultiple = SE->getSmallConstantTripMultiple(L, ExitingBlock);
  }

  TargetTransformInfo::UnrollingPreferences UP = gatherUnrollingPreferences(
      L, TTI, ProvidedThreshold, ProvidedCount, ProvidedAllowPartial,
      ProvidedRuntime, PragmaCount, PragmaFullUnroll, PragmaEnableUnroll,
      TripCount);

  unsigned Count = UP.Count;
  bool CountSetExplicitly = Count != 0;
  // Use a heuristic count if we didn't set anything explicitly.
  if (!CountSetExplicitly)
    Count = TripCount == 0 ? DefaultUnrollRuntimeCount : TripCount;
  if (TripCount && Count > TripCount)
    Count = TripCount;
  Count = std::min(Count, UP.FullUnrollMaxCount);

  unsigned NumInlineCandidates;
  bool NotDuplicatable;
  bool Convergent;
  unsigned LoopSize = ApproximateLoopSize(
      L, NumInlineCandidates, NotDuplicatable, Convergent, TTI, &AC);
  DEBUG(dbgs() << "  Loop Size = " << LoopSize << "\n");

  // When computing the unrolled size, note that the conditional branch on the
  // backedge and the comparison feeding it are not replicated like the rest of
  // the loop body (which is why 2 is subtracted).
  uint64_t UnrolledSize = (uint64_t)(LoopSize - 2) * Count + 2;
  if (NotDuplicatable) {
    DEBUG(dbgs() << "  Not unrolling loop which contains non-duplicatable"
                 << " instructions.\n");
    return false;
  }
  if (NumInlineCandidates != 0) {
    DEBUG(dbgs() << "  Not unrolling loop with inlinable calls.\n");
    return false;
  }

  // Given Count, TripCount and thresholds determine the type of
  // unrolling which is to be performed.
  enum { Full = 0, Partial = 1, Runtime = 2 };
  int Unrolling;
  if (TripCount && Count == TripCount) {
    Unrolling = Partial;
    // If the loop is really small, we don't need to run an expensive analysis.
    if (canUnrollCompletely(L, UP.Threshold, 100, UP.DynamicCostSavingsDiscount,
                            UnrolledSize, UnrolledSize)) {
      Unrolling = Full;
    } else {
      // The loop isn't that small, but we still can fully unroll it if that
      // helps to remove a significant number of instructions.
      // To check that, run additional analysis on the loop.
      if (Optional<EstimatedUnrollCost> Cost = analyzeLoopUnrollCost(
              L, TripCount, DT, *SE, TTI,
              UP.Threshold + UP.DynamicCostSavingsDiscount))
        if (canUnrollCompletely(L, UP.Threshold,
                                UP.PercentDynamicCostSavedThreshold,
                                UP.DynamicCostSavingsDiscount,
                                Cost->UnrolledCost, Cost->RolledDynamicCost)) {
          Unrolling = Full;
        }
    }
  } else if (TripCount && Count < TripCount) {
    Unrolling = Partial;
  } else {
    Unrolling = Runtime;
  }

  // Reduce count based on the type of unrolling and the threshold values.
  unsigned OriginalCount = Count;
  bool AllowRuntime = PragmaEnableUnroll || (PragmaCount > 0) || UP.Runtime;
  // Don't unroll a runtime trip count loop with unroll full pragma.
  if (HasRuntimeUnrollDisablePragma(L) || PragmaFullUnroll) {
    AllowRuntime = false;
  }
  bool DecreasedCountDueToConvergence = false;
  if (Unrolling == Partial) {
    bool AllowPartial = PragmaEnableUnroll || UP.Partial;
    if (!AllowPartial && !CountSetExplicitly) {
      DEBUG(dbgs() << "  will not try to unroll partially because "
                   << "-unroll-allow-partial not given\n");
      return false;
    }
    if (UP.PartialThreshold != NoThreshold && Count > 1) {
      // Reduce unroll count to be modulo of TripCount for partial unrolling.
      if (UnrolledSize > UP.PartialThreshold)
        Count = (std::max(UP.PartialThreshold, 3u) - 2) / (LoopSize - 2);
      if (Count > UP.MaxCount)
        Count = UP.MaxCount;
      while (Count != 0 && TripCount % Count != 0)
        Count--;
      if (AllowRuntime && Count <= 1) {
        // If there is no Count that is modulo of TripCount, set Count to
        // largest power-of-two factor that satisfies the threshold limit.
        // As we'll create fixup loop, do the type of unrolling only if
        // runtime unrolling is allowed.
        Count = DefaultUnrollRuntimeCount;
        UnrolledSize = (LoopSize - 2) * Count + 2;
        while (Count != 0 && UnrolledSize > UP.PartialThreshold) {
          Count >>= 1;
          UnrolledSize = (LoopSize - 2) * Count + 2;
        }
      }
    }
  } else if (Unrolling == Runtime) {
    if (!AllowRuntime && !CountSetExplicitly) {
      DEBUG(dbgs() << "  will not try to unroll loop with runtime trip count "
                   << "-unroll-runtime not given\n");
      return false;
    }

    // Reduce unroll count to be the largest power-of-two factor of
    // the original count which satisfies the threshold limit.
    while (Count != 0 && UnrolledSize > UP.PartialThreshold) {
      Count >>= 1;
      UnrolledSize = (LoopSize - 2) * Count + 2;
    }

    if (Count > UP.MaxCount)
      Count = UP.MaxCount;

    // If the loop contains a convergent operation, the prelude we'd add
    // to do the first few instructions before we hit the unrolled loop
    // is unsafe -- it adds a control-flow dependency to the convergent
    // operation.  Therefore Count must divide TripMultiple.
    //
    // TODO: This is quite conservative.  In practice, convergent_op()
    // is likely to be called unconditionally in the loop.  In this
    // case, the program would be ill-formed (on most architectures)
    // unless n were the same on all threads in a thread group.
    // Assuming n is the same on all threads, any kind of unrolling is
    // safe.  But currently llvm's notion of convergence isn't powerful
    // enough to express this.
    unsigned OrigCount = Count;
    while (Convergent && Count != 0 && TripMultiple % Count != 0) {
      DecreasedCountDueToConvergence = true;
      Count >>= 1;
    }
    if (OrigCount > Count) {
      DEBUG(dbgs() << "  loop contains a convergent instruction, so unroll "
                      "count must divide the trip multiple, "
                   << TripMultiple << ".  Reducing unroll count from "
                   << OrigCount << " to " << Count << ".\n");
    }
    DEBUG(dbgs() << "  partially unrolling with count: " << Count << "\n");
  }

  if (HasPragma) {
    // Emit optimization remarks if we are unable to unroll the loop
    // as directed by a pragma.
    DebugLoc LoopLoc = L->getStartLoc();
    Function *F = Header->getParent();
    LLVMContext &Ctx = F->getContext();
    if (PragmaCount > 0 && DecreasedCountDueToConvergence) {
      emitOptimizationRemarkMissed(
          Ctx, DEBUG_TYPE, *F, LoopLoc,
          Twine("Unable to unroll loop the number of times directed by "
                "unroll_count pragma because the loop contains a convergent "
                "instruction, and so must have an unroll count that divides "
                "the loop trip multiple of ") +
              Twine(TripMultiple) + ".  Unrolling instead " + Twine(Count) +
              " time(s).");
    } else if ((PragmaCount > 0) && Count != OriginalCount) {
      emitOptimizationRemarkMissed(
          Ctx, DEBUG_TYPE, *F, LoopLoc,
          "Unable to unroll loop the number of times directed by "
          "unroll_count pragma because unrolled size is too large.");
    } else if (PragmaFullUnroll && !TripCount) {
      emitOptimizationRemarkMissed(
          Ctx, DEBUG_TYPE, *F, LoopLoc,
          "Unable to fully unroll loop as directed by unroll(full) pragma "
          "because loop has a runtime trip count.");
    } else if (PragmaEnableUnroll && Count != TripCount && Count < 2) {
      emitOptimizationRemarkMissed(
          Ctx, DEBUG_TYPE, *F, LoopLoc,
          "Unable to unroll loop as directed by unroll(enable) pragma because "
          "unrolled size is too large.");
    } else if ((PragmaFullUnroll || PragmaEnableUnroll) && TripCount &&
               Count != TripCount) {
      emitOptimizationRemarkMissed(
          Ctx, DEBUG_TYPE, *F, LoopLoc,
          "Unable to fully unroll loop as directed by unroll pragma because "
          "unrolled size is too large.");
    }
  }

  if (Unrolling != Full && Count < 2) {
    // Partial unrolling by 1 is a nop.  For full unrolling, a factor
    // of 1 makes sense because loop control can be eliminated.
    return false;
  }

  // Unroll the loop.
  if (!UnrollLoop(L, Count, TripCount, AllowRuntime, UP.AllowExpensiveTripCount,
                  TripMultiple, LI, SE, &DT, &AC, PreserveLCSSA))
    return false;

  // If loop has an unroll count pragma mark loop as unrolled to prevent
  // unrolling beyond that requested by the pragma.
  if (HasPragma && PragmaCount != 0)
    SetLoopAlreadyUnrolled(L);
  return true;
}

namespace {
class LoopUnroll : public LoopPass {
public:
  static char ID; // Pass ID, replacement for typeid
  LoopUnroll(Optional<unsigned> Threshold = None,
             Optional<unsigned> Count = None,
             Optional<bool> AllowPartial = None, Optional<bool> Runtime = None)
      : LoopPass(ID), ProvidedCount(Count), ProvidedThreshold(Threshold),
        ProvidedAllowPartial(AllowPartial), ProvidedRuntime(Runtime) {
    initializeLoopUnrollPass(*PassRegistry::getPassRegistry());
  }

  Optional<unsigned> ProvidedCount;
  Optional<unsigned> ProvidedThreshold;
  Optional<bool> ProvidedAllowPartial;
  Optional<bool> ProvidedRuntime;

  bool runOnLoop(Loop *L, LPPassManager &) override {
    if (skipLoop(L))
      return false;

    Function &F = *L->getHeader()->getParent();

    auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    ScalarEvolution *SE = &getAnalysis<ScalarEvolutionWrapperPass>().getSE();
    const TargetTransformInfo &TTI =
        getAnalysis<TargetTransformInfoWrapperPass>().getTTI(F);
    auto &AC = getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);
    bool PreserveLCSSA = mustPreserveAnalysisID(LCSSAID);

    return tryToUnrollLoop(L, DT, LI, SE, TTI, AC, PreserveLCSSA, ProvidedCount,
                           ProvidedThreshold, ProvidedAllowPartial,
                           ProvidedRuntime);
  }

  /// This transformation requires natural loop information & requires that
  /// loop preheaders be inserted into the CFG...
  ///
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<AssumptionCacheTracker>();
    AU.addRequired<TargetTransformInfoWrapperPass>();
    // FIXME: Loop passes are required to preserve domtree, and for now we just
    // recreate dom info if anything gets unrolled.
    getLoopAnalysisUsage(AU);
  }
};
}

char LoopUnroll::ID = 0;
INITIALIZE_PASS_BEGIN(LoopUnroll, "loop-unroll", "Unroll loops", false, false)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(LoopPass)
INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
INITIALIZE_PASS_END(LoopUnroll, "loop-unroll", "Unroll loops", false, false)

Pass *llvm::createLoopUnrollPass(int Threshold, int Count, int AllowPartial,
                                 int Runtime) {
  // TODO: It would make more sense for this function to take the optionals
  // directly, but that's dangerous since it would silently break out of tree
  // callers.
  return new LoopUnroll(Threshold == -1 ? None : Optional<unsigned>(Threshold),
                        Count == -1 ? None : Optional<unsigned>(Count),
                        AllowPartial == -1 ? None
                                           : Optional<bool>(AllowPartial),
                        Runtime == -1 ? None : Optional<bool>(Runtime));
}

Pass *llvm::createSimpleLoopUnrollPass() {
  return llvm::createLoopUnrollPass(-1, -1, 0, 0);
}
