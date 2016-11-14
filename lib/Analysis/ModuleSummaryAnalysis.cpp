//===- ModuleSummaryAnalysis.cpp - Module summary index builder -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass builds a ModuleSummaryIndex object for the module, to be written
// to bitcode or LLVM assembly.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/ModuleSummaryAnalysis.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BlockFrequencyInfoImpl.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/IndirectCallPromotionAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/Pass.h"
using namespace llvm;

#define DEBUG_TYPE "module-summary-analysis"

// Walk through the operands of a given User via worklist iteration and populate
// the set of GlobalValue references encountered. Invoked either on an
// Instruction or a GlobalVariable (which walks its initializer).
static void findRefEdges(const User *CurUser, DenseSet<const Value *> &RefEdges,
                         SmallPtrSet<const User *, 8> &Visited) {
  SmallVector<const User *, 32> Worklist;
  Worklist.push_back(CurUser);

  while (!Worklist.empty()) {
    const User *U = Worklist.pop_back_val();

    if (!Visited.insert(U).second)
      continue;

    ImmutableCallSite CS(U);

    for (const auto &OI : U->operands()) {
      const User *Operand = dyn_cast<User>(OI);
      if (!Operand)
        continue;
      if (isa<BlockAddress>(Operand))
        continue;
      if (isa<GlobalValue>(Operand)) {
        // We have a reference to a global value. This should be added to
        // the reference set unless it is a callee. Callees are handled
        // specially by WriteFunction and are added to a separate list.
        if (!(CS && CS.isCallee(&OI)))
          RefEdges.insert(Operand);
        continue;
      }
      Worklist.push_back(Operand);
    }
  }
}

static CalleeInfo::HotnessType getHotness(uint64_t ProfileCount,
                                          ProfileSummaryInfo *PSI) {
  if (!PSI)
    return CalleeInfo::HotnessType::Unknown;
  if (PSI->isHotCount(ProfileCount))
    return CalleeInfo::HotnessType::Hot;
  if (PSI->isColdCount(ProfileCount))
    return CalleeInfo::HotnessType::Cold;
  return CalleeInfo::HotnessType::None;
}

static void computeFunctionSummary(ModuleSummaryIndex &Index, const Module &M,
                                   const Function &F, BlockFrequencyInfo *BFI,
                                   ProfileSummaryInfo *PSI,
                                   bool HasLocalsInUsed) {
  // Summary not currently supported for anonymous functions, they should
  // have been named.
  assert(F.hasName());

  unsigned NumInsts = 0;
  // Map from callee ValueId to profile count. Used to accumulate profile
  // counts for all static calls to a given callee.
  DenseMap<const Value *, CalleeInfo> CallGraphEdges;
  DenseMap<GlobalValue::GUID, CalleeInfo> IndirectCallEdges;
  DenseSet<const Value *> RefEdges;
  ICallPromotionAnalysis ICallAnalysis;

  bool HasInlineAsmMaybeReferencingInternal = false;
  SmallPtrSet<const User *, 8> Visited;
  for (const BasicBlock &BB : F)
    for (const Instruction &I : BB) {
      if (isa<DbgInfoIntrinsic>(I))
        continue;
      ++NumInsts;
      findRefEdges(&I, RefEdges, Visited);
      auto CS = ImmutableCallSite(&I);
      if (!CS)
        continue;

      const auto *CI = dyn_cast<CallInst>(&I);
      // Since we don't know exactly which local values are referenced in inline
      // assembly, conservatively mark the function as possibly referencing
      // a local value from inline assembly to ensure we don't export a
      // reference (which would require renaming and promotion of the
      // referenced value).
      if (HasLocalsInUsed && CI && CI->isInlineAsm())
        HasInlineAsmMaybeReferencingInternal = true;

      auto *CalledValue = CS.getCalledValue();
      auto *CalledFunction = CS.getCalledFunction();
      // Check if this is an alias to a function. If so, get the
      // called aliasee for the checks below.
      if (auto *GA = dyn_cast<GlobalAlias>(CalledValue)) {
        assert(!CalledFunction && "Expected null called function in callsite for alias");
        CalledFunction = dyn_cast<Function>(GA->getBaseObject());
      }
      // Check if this is a direct call to a known function.
      if (CalledFunction) {
        // Skip intrinsics.
        if (CalledFunction->isIntrinsic())
          continue;
        // We should have named any anonymous globals
        assert(CalledFunction->hasName());
        auto ScaledCount = BFI ? BFI->getBlockProfileCount(&BB) : None;
        // Use the original CalledValue, in case it was an alias. We want
        // to record the call edge to the alias in that case. Eventually
        // an alias summary will be created to associate the alias and
        // aliasee.
        auto *CalleeId =
            M.getValueSymbolTable().lookup(CalledValue->getName());

        auto Hotness = ScaledCount ? getHotness(ScaledCount.getValue(), PSI)
                                   : CalleeInfo::HotnessType::Unknown;
        CallGraphEdges[CalleeId].updateHotness(Hotness);
      } else {
        // Skip inline assembly calls.
        if (CI && CI->isInlineAsm())
          continue;
        // Skip direct calls.
        if (!CS.getCalledValue() || isa<Constant>(CS.getCalledValue()))
          continue;

        uint32_t NumVals, NumCandidates;
        uint64_t TotalCount;
        auto CandidateProfileData =
            ICallAnalysis.getPromotionCandidatesForInstruction(
                &I, NumVals, TotalCount, NumCandidates);
        for (auto &Candidate : CandidateProfileData)
          IndirectCallEdges[Candidate.Value].updateHotness(
              getHotness(Candidate.Count, PSI));
      }
    }

  GlobalValueSummary::GVFlags Flags(F);
  std::unique_ptr<FunctionSummary> FuncSummary =
      llvm::make_unique<FunctionSummary>(Flags, NumInsts);
  FuncSummary->addCallGraphEdges(CallGraphEdges);
  FuncSummary->addCallGraphEdges(IndirectCallEdges);
  FuncSummary->addRefEdges(RefEdges);
  if (HasInlineAsmMaybeReferencingInternal)
    FuncSummary->setHasInlineAsmMaybeReferencingInternal();
  Index.addGlobalValueSummary(F.getName(), std::move(FuncSummary));
}

static void computeVariableSummary(ModuleSummaryIndex &Index,
                                   const GlobalVariable &V) {
  DenseSet<const Value *> RefEdges;
  SmallPtrSet<const User *, 8> Visited;
  findRefEdges(&V, RefEdges, Visited);
  GlobalValueSummary::GVFlags Flags(V);
  std::unique_ptr<GlobalVarSummary> GVarSummary =
      llvm::make_unique<GlobalVarSummary>(Flags);
  GVarSummary->addRefEdges(RefEdges);
  Index.addGlobalValueSummary(V.getName(), std::move(GVarSummary));
}

static void computeAliasSummary(ModuleSummaryIndex &Index,
                                const GlobalAlias &A) {
  GlobalValueSummary::GVFlags Flags(A);
  std::unique_ptr<AliasSummary> AS = llvm::make_unique<AliasSummary>(Flags);
  auto *Aliasee = A.getBaseObject();
  auto *AliaseeSummary = Index.getGlobalValueSummary(*Aliasee);
  assert(AliaseeSummary && "Alias expects aliasee summary to be parsed");
  AS->setAliasee(AliaseeSummary);
  Index.addGlobalValueSummary(A.getName(), std::move(AS));
}

ModuleSummaryIndex llvm::buildModuleSummaryIndex(
    const Module &M,
    std::function<BlockFrequencyInfo *(const Function &F)> GetBFICallback,
    ProfileSummaryInfo *PSI) {
  ModuleSummaryIndex Index;

  // Identify the local values in the llvm.used and llvm.compiler.used sets,
  // which should not be exported as they would then require renaming and
  // promotion, but we may have opaque uses e.g. in inline asm. We collect them
  // here because we use this information to mark functions containing inline
  // assembly calls as not importable.
  SmallPtrSet<GlobalValue *, 8> LocalsUsed;
  SmallPtrSet<GlobalValue *, 8> Used;
  // First collect those in the llvm.used set.
  collectUsedGlobalVariables(M, Used, /*CompilerUsed*/ false);
  for (auto *V : Used) {
    if (V->hasLocalLinkage())
      LocalsUsed.insert(V);
  }
  Used.clear();
  // Next collect those in the llvm.compiler.used set.
  collectUsedGlobalVariables(M, Used, /*CompilerUsed*/ true);
  for (auto *V : Used) {
    if (V->hasLocalLinkage())
      LocalsUsed.insert(V);
  }

  // Compute summaries for all functions defined in module, and save in the
  // index.
  for (auto &F : M) {
    if (F.isDeclaration())
      continue;

    BlockFrequencyInfo *BFI = nullptr;
    std::unique_ptr<BlockFrequencyInfo> BFIPtr;
    if (GetBFICallback)
      BFI = GetBFICallback(F);
    else if (F.getEntryCount().hasValue()) {
      LoopInfo LI{DominatorTree(const_cast<Function &>(F))};
      BranchProbabilityInfo BPI{F, LI};
      BFIPtr = llvm::make_unique<BlockFrequencyInfo>(F, BPI, LI);
      BFI = BFIPtr.get();
    }

    computeFunctionSummary(Index, M, F, BFI, PSI, !LocalsUsed.empty());
  }

  // Compute summaries for all variables defined in module, and save in the
  // index.
  for (const GlobalVariable &G : M.globals()) {
    if (G.isDeclaration())
      continue;
    computeVariableSummary(Index, G);
  }

  // Compute summaries for all aliases defined in module, and save in the
  // index.
  for (const GlobalAlias &A : M.aliases())
    computeAliasSummary(Index, A);

  for (auto *V : LocalsUsed) {
    auto *Summary = Index.getGlobalValueSummary(*V);
    assert(Summary && "Missing summary for global value");
    Summary->setNoRename();
  }

  return Index;
}

char ModuleSummaryIndexAnalysis::PassID;

ModuleSummaryIndex
ModuleSummaryIndexAnalysis::run(Module &M, ModuleAnalysisManager &AM) {
  ProfileSummaryInfo &PSI = AM.getResult<ProfileSummaryAnalysis>(M);
  auto &FAM = AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
  return buildModuleSummaryIndex(
      M,
      [&FAM](const Function &F) {
        return &FAM.getResult<BlockFrequencyAnalysis>(
            *const_cast<Function *>(&F));
      },
      &PSI);
}

char ModuleSummaryIndexWrapperPass::ID = 0;
INITIALIZE_PASS_BEGIN(ModuleSummaryIndexWrapperPass, "module-summary-analysis",
                      "Module Summary Analysis", false, true)
INITIALIZE_PASS_DEPENDENCY(BlockFrequencyInfoWrapperPass)
INITIALIZE_PASS_END(ModuleSummaryIndexWrapperPass, "module-summary-analysis",
                    "Module Summary Analysis", false, true)

ModulePass *llvm::createModuleSummaryIndexWrapperPass() {
  return new ModuleSummaryIndexWrapperPass();
}

ModuleSummaryIndexWrapperPass::ModuleSummaryIndexWrapperPass()
    : ModulePass(ID) {
  initializeModuleSummaryIndexWrapperPassPass(*PassRegistry::getPassRegistry());
}

bool ModuleSummaryIndexWrapperPass::runOnModule(Module &M) {
  auto &PSI = *getAnalysis<ProfileSummaryInfoWrapperPass>().getPSI();
  Index = buildModuleSummaryIndex(
      M,
      [this](const Function &F) {
        return &(this->getAnalysis<BlockFrequencyInfoWrapperPass>(
                         *const_cast<Function *>(&F))
                     .getBFI());
      },
      &PSI);
  return false;
}

bool ModuleSummaryIndexWrapperPass::doFinalization(Module &M) {
  Index.reset();
  return false;
}

void ModuleSummaryIndexWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
  AU.addRequired<BlockFrequencyInfoWrapperPass>();
  AU.addRequired<ProfileSummaryInfoWrapperPass>();
}
