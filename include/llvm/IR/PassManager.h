//===- PassManager.h - Pass management infrastructure -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// This header defines various interfaces for pass management in LLVM. There
/// is no "pass" interface in LLVM per se. Instead, an instance of any class
/// which supports a method to 'run' it over a unit of IR can be used as
/// a pass. A pass manager is generally a tool to collect a sequence of passes
/// which run over a particular IR construct, and run each of them in sequence
/// over each such construct in the containing IR construct. As there is no
/// containing IR construct for a Module, a manager for passes over modules
/// forms the base case which runs its managed passes in sequence over the
/// single module provided.
///
/// The core IR library provides managers for running passes over
/// modules and functions.
///
/// * FunctionPassManager can run over a Module, runs each pass over
///   a Function.
/// * ModulePassManager must be directly run, runs each pass over the Module.
///
/// Note that the implementations of the pass managers use concept-based
/// polymorphism as outlined in the "Value Semantics and Concept-based
/// Polymorphism" talk (or its abbreviated sibling "Inheritance Is The Base
/// Class of Evil") by Sean Parent:
/// * http://github.com/sean-parent/sean-parent.github.com/wiki/Papers-and-Presentations
/// * http://www.youtube.com/watch?v=_BpMYeUFXv8
/// * http://channel9.msdn.com/Events/GoingNative/2013/Inheritance-Is-The-Base-Class-of-Evil
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_IR_PASSMANAGER_H
#define LLVM_IR_PASSMANAGER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManagerInternal.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/TypeName.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/type_traits.h"
#include <list>
#include <memory>
#include <vector>

namespace llvm {

/// \brief An abstract set of preserved analyses following a transformation pass
/// run.
///
/// When a transformation pass is run, it can return a set of analyses whose
/// results were preserved by that transformation. The default set is "none",
/// and preserving analyses must be done explicitly.
///
/// There is also an explicit all state which can be used (for example) when
/// the IR is not mutated at all.
class PreservedAnalyses {
public:
  /// \brief Convenience factory function for the empty preserved set.
  static PreservedAnalyses none() { return PreservedAnalyses(); }

  /// \brief Construct a special preserved set that preserves all passes.
  static PreservedAnalyses all() {
    PreservedAnalyses PA;
    PA.PreservedPassIDs.insert((void *)AllPassesID);
    return PA;
  }

  /// \brief Mark a particular pass as preserved, adding it to the set.
  template <typename PassT> void preserve() { preserve(PassT::ID()); }

  /// \brief Mark an abstract PassID as preserved, adding it to the set.
  void preserve(void *PassID) {
    if (!areAllPreserved())
      PreservedPassIDs.insert(PassID);
  }

  /// \brief Intersect this set with another in place.
  ///
  /// This is a mutating operation on this preserved set, removing all
  /// preserved passes which are not also preserved in the argument.
  void intersect(const PreservedAnalyses &Arg) {
    if (Arg.areAllPreserved())
      return;
    if (areAllPreserved()) {
      PreservedPassIDs = Arg.PreservedPassIDs;
      return;
    }
    for (void *P : PreservedPassIDs)
      if (!Arg.PreservedPassIDs.count(P))
        PreservedPassIDs.erase(P);
  }

  /// \brief Intersect this set with a temporary other set in place.
  ///
  /// This is a mutating operation on this preserved set, removing all
  /// preserved passes which are not also preserved in the argument.
  void intersect(PreservedAnalyses &&Arg) {
    if (Arg.areAllPreserved())
      return;
    if (areAllPreserved()) {
      PreservedPassIDs = std::move(Arg.PreservedPassIDs);
      return;
    }
    for (void *P : PreservedPassIDs)
      if (!Arg.PreservedPassIDs.count(P))
        PreservedPassIDs.erase(P);
  }

  /// \brief Query whether a pass is marked as preserved by this set.
  template <typename PassT> bool preserved() const {
    return preserved(PassT::ID());
  }

  /// \brief Query whether an abstract pass ID is marked as preserved by this
  /// set.
  bool preserved(void *PassID) const {
    return PreservedPassIDs.count((void *)AllPassesID) ||
           PreservedPassIDs.count(PassID);
  }

  /// \brief Query whether all of the analyses in the set are preserved.
  bool preserved(PreservedAnalyses Arg) {
    if (Arg.areAllPreserved())
      return areAllPreserved();
    for (void *P : Arg.PreservedPassIDs)
      if (!preserved(P))
        return false;
    return true;
  }

  /// \brief Test whether all passes are preserved.
  ///
  /// This is used primarily to optimize for the case of no changes which will
  /// common in many scenarios.
  bool areAllPreserved() const {
    return PreservedPassIDs.count((void *)AllPassesID);
  }

private:
  // Note that this must not be -1 or -2 as those are already used by the
  // SmallPtrSet.
  static const uintptr_t AllPassesID = (intptr_t)(-3);

  SmallPtrSet<void *, 2> PreservedPassIDs;
};

// Forward declare the analysis manager template.
template <typename IRUnitT, typename... ExtraArgTs> class AnalysisManager;

/// A CRTP mix-in to automatically provide informational APIs needed for
/// passes.
///
/// This provides some boiler plate for types that are passes.
template <typename DerivedT> struct PassInfoMixin {
  /// Returns the name of the derived pass type.
  static StringRef name() {
    StringRef Name = getTypeName<DerivedT>();
    if (Name.startswith("llvm::"))
      Name = Name.drop_front(strlen("llvm::"));
    return Name;
  }
};

/// A CRTP mix-in to automatically provide informational APIs needed for
/// analysis passes.
///
/// This provides some boiler plate for types that are analysis passes. It
/// automatically mixes in \c PassInfoMixin and adds informational APIs
/// specifically used for analyses.
template <typename DerivedT>
struct AnalysisInfoMixin : PassInfoMixin<DerivedT> {
  /// Returns an opaque, unique ID for this pass type.
  ///
  /// Note that this requires the derived type provide a static member whose
  /// address can be converted to a void pointer.
  ///
  /// FIXME: The only reason the derived type needs to provide this rather than
  /// this mixin providing it is due to broken implementations which cannot
  /// correctly unique a templated static so that they have the same addresses
  /// for each instantiation and are definitively emitted once for each
  /// instantiation. The only currently known platform with this limitation are
  /// Windows DLL builds, specifically building each part of LLVM as a DLL. If
  /// we ever remove that build configuration, this mixin can provide the
  /// static PassID as well.
  static void *ID() { return (void *)&DerivedT::PassID; }
};

/// A class template to provide analysis sets for IR units.
///
/// Analyses operate on units of IR. It is useful to be able to talk about
/// preservation of all analyses for a given unit of IR as a set. This class
/// template can be used with the \c PreservedAnalyses API for that purpose and
/// the \c AnalysisManager will automatically check and use this set to skip
/// invalidation events.
///
/// Note that you must provide an explicit instantiation declaration and
/// definition for this template in order to get the correct behavior on
/// Windows. Otherwise, the address of SetID will not be stable.
template <typename IRUnitT>
class AllAnalysesOn {
public:
  static void *ID() { return (void *)&SetID; }

private:
  static char SetID;
};

template <typename IRUnitT> char AllAnalysesOn<IRUnitT>::SetID;

extern template class AllAnalysesOn<Module>;
extern template class AllAnalysesOn<Function>;

/// \brief Manages a sequence of passes over units of IR.
///
/// A pass manager contains a sequence of passes to run over units of IR. It is
/// itself a valid pass over that unit of IR, and when over some given IR will
/// run each pass in sequence. This is the primary and most basic building
/// block of a pass pipeline.
///
/// If it is run with an \c AnalysisManager<IRUnitT> argument, it will propagate
/// that analysis manager to each pass it runs, as well as calling the analysis
/// manager's invalidation routine with the PreservedAnalyses of each pass it
/// runs.
template <typename IRUnitT,
          typename AnalysisManagerT = AnalysisManager<IRUnitT>,
          typename... ExtraArgTs>
class PassManager : public PassInfoMixin<
                        PassManager<IRUnitT, AnalysisManagerT, ExtraArgTs...>> {
public:
  /// \brief Construct a pass manager.
  ///
  /// It can be passed a flag to get debug logging as the passes are run.
  PassManager(bool DebugLogging = false) : DebugLogging(DebugLogging) {}
  // We have to explicitly define all the special member functions because MSVC
  // refuses to generate them.
  PassManager(PassManager &&Arg)
      : Passes(std::move(Arg.Passes)),
        DebugLogging(std::move(Arg.DebugLogging)) {}
  PassManager &operator=(PassManager &&RHS) {
    Passes = std::move(RHS.Passes);
    DebugLogging = std::move(RHS.DebugLogging);
    return *this;
  }

  /// \brief Run all of the passes in this manager over the IR.
  PreservedAnalyses run(IRUnitT &IR, AnalysisManagerT &AM,
                        ExtraArgTs... ExtraArgs) {
    PreservedAnalyses PA = PreservedAnalyses::all();

    if (DebugLogging)
      dbgs() << "Starting " << getTypeName<IRUnitT>() << " pass manager run.\n";

    for (unsigned Idx = 0, Size = Passes.size(); Idx != Size; ++Idx) {
      if (DebugLogging)
        dbgs() << "Running pass: " << Passes[Idx]->name() << " on "
               << IR.getName() << "\n";

      PreservedAnalyses PassPA = Passes[Idx]->run(IR, AM, ExtraArgs...);

      // Update the analysis manager as each pass runs and potentially
      // invalidates analyses. We also update the preserved set of analyses
      // based on what analyses we have already handled the invalidation for
      // here and don't need to invalidate when finished.
      PassPA = AM.invalidate(IR, std::move(PassPA));

      // Finally, we intersect the final preserved analyses to compute the
      // aggregate preserved set for this pass manager.
      PA.intersect(std::move(PassPA));

      // FIXME: Historically, the pass managers all called the LLVM context's
      // yield function here. We don't have a generic way to acquire the
      // context and it isn't yet clear what the right pattern is for yielding
      // in the new pass manager so it is currently omitted.
      //IR.getContext().yield();
    }

    // Invaliadtion was handled after each pass in the above loop for the
    // current unit of IR. Therefore, the remaining analysis results in the
    // AnalysisManager are preserved. We mark this with a set so that we don't
    // need to inspect each one individually.
    PA.preserve<AllAnalysesOn<IRUnitT>>();

    if (DebugLogging)
      dbgs() << "Finished " << getTypeName<IRUnitT>() << " pass manager run.\n";

    return PA;
  }

  template <typename PassT> void addPass(PassT Pass) {
    typedef detail::PassModel<IRUnitT, PassT, PreservedAnalyses,
                              AnalysisManagerT, ExtraArgTs...>
        PassModelT;
    Passes.emplace_back(new PassModelT(std::move(Pass)));
  }

private:
  typedef detail::PassConcept<IRUnitT, AnalysisManagerT, ExtraArgTs...>
      PassConceptT;

  std::vector<std::unique_ptr<PassConceptT>> Passes;

  /// \brief Flag indicating whether we should do debug logging.
  bool DebugLogging;
};

extern template class PassManager<Module>;
/// \brief Convenience typedef for a pass manager over modules.
typedef PassManager<Module> ModulePassManager;

extern template class PassManager<Function>;
/// \brief Convenience typedef for a pass manager over functions.
typedef PassManager<Function> FunctionPassManager;

/// \brief A generic analysis pass manager with lazy running and caching of
/// results.
///
/// This analysis manager can be used for any IR unit where the address of the
/// IR unit sufficies as its identity. It manages the cache for a unit of IR via
/// the address of each unit of IR cached.
template <typename IRUnitT, typename... ExtraArgTs> class AnalysisManager {
  typedef detail::AnalysisResultConcept<IRUnitT> ResultConceptT;
  typedef detail::AnalysisPassConcept<IRUnitT, ExtraArgTs...> PassConceptT;

public:
  // Most public APIs are inherited from the CRTP base class.

  /// \brief Construct an empty analysis manager.
  ///
  /// A flag can be passed to indicate that the manager should perform debug
  /// logging.
  AnalysisManager(bool DebugLogging = false) : DebugLogging(DebugLogging) {}
  AnalysisManager(AnalysisManager &&) = default;
  AnalysisManager &operator=(AnalysisManager &&) = default;

  /// \brief Returns true if the analysis manager has an empty results cache.
  bool empty() const {
    assert(AnalysisResults.empty() == AnalysisResultLists.empty() &&
           "The storage and index of analysis results disagree on how many "
           "there are!");
    return AnalysisResults.empty();
  }

  /// \brief Clear any results for a single unit of IR.
  ///
  /// This doesn't invalidate but directly clears the results. It is useful
  /// when the IR is being removed and we want to clear out all the memory
  /// pinned for it.
  void clear(IRUnitT &IR) {
    if (DebugLogging)
      dbgs() << "Clearing all analysis results for: " << IR.getName() << "\n";

    // Clear all the invalidated results associated specifically with this
    // function.
    SmallVector<void *, 8> InvalidatedPassIDs;
    auto ResultsListI = AnalysisResultLists.find(&IR);
    if (ResultsListI == AnalysisResultLists.end())
      return;
    // Clear the map pointing into the results list.
    for (auto &PassIDAndResult : ResultsListI->second)
      AnalysisResults.erase(std::make_pair(PassIDAndResult.first, &IR));

    // And actually destroy and erase the results associated with this IR.
    AnalysisResultLists.erase(ResultsListI);
  }

  /// \brief Clear the analysis result cache.
  ///
  /// This routine allows cleaning up when the set of IR units itself has
  /// potentially changed, and thus we can't even look up a a result and
  /// invalidate it directly. Notably, this does *not* call invalidate
  /// functions as there is nothing to be done for them.
  void clear() {
    AnalysisResults.clear();
    AnalysisResultLists.clear();
  }

  /// \brief Get the result of an analysis pass for this module.
  ///
  /// If there is not a valid cached result in the manager already, this will
  /// re-run the analysis to produce a valid result.
  template <typename PassT>
  typename PassT::Result &getResult(IRUnitT &IR, ExtraArgTs... ExtraArgs) {
    assert(AnalysisPasses.count(PassT::ID()) &&
           "This analysis pass was not registered prior to being queried");
    ResultConceptT &ResultConcept =
        getResultImpl(PassT::ID(), IR, ExtraArgs...);
    typedef detail::AnalysisResultModel<IRUnitT, PassT, typename PassT::Result>
        ResultModelT;
    return static_cast<ResultModelT &>(ResultConcept).Result;
  }

  /// \brief Get the cached result of an analysis pass for this module.
  ///
  /// This method never runs the analysis.
  ///
  /// \returns null if there is no cached result.
  template <typename PassT>
  typename PassT::Result *getCachedResult(IRUnitT &IR) const {
    assert(AnalysisPasses.count(PassT::ID()) &&
           "This analysis pass was not registered prior to being queried");

    ResultConceptT *ResultConcept = getCachedResultImpl(PassT::ID(), IR);
    if (!ResultConcept)
      return nullptr;

    typedef detail::AnalysisResultModel<IRUnitT, PassT, typename PassT::Result>
        ResultModelT;
    return &static_cast<ResultModelT *>(ResultConcept)->Result;
  }

  /// \brief Register an analysis pass with the manager.
  ///
  /// The argument is a callable whose result is a pass. This allows passing in
  /// a lambda to construct the pass.
  ///
  /// The pass type registered is the result type of calling the argument. If
  /// that pass has already been registered, then the argument will not be
  /// called and this function will return false. Otherwise, the pass type
  /// becomes registered, with the instance provided by calling the argument
  /// once, and this function returns true.
  ///
  /// While this returns whether or not the pass type was already registered,
  /// there in't an independent way to query that as that would be prone to
  /// risky use when *querying* the analysis manager. Instead, the only
  /// supported use case is avoiding duplicate registry of an analysis. This
  /// interface also lends itself to minimizing the number of times we have to
  /// do lookups for analyses or construct complex passes only to throw them
  /// away.
  template <typename PassBuilderT> bool registerPass(PassBuilderT PassBuilder) {
    typedef decltype(PassBuilder()) PassT;
    typedef detail::AnalysisPassModel<IRUnitT, PassT, ExtraArgTs...> PassModelT;

    auto &PassPtr = AnalysisPasses[PassT::ID()];
    if (PassPtr)
      // Already registered this pass type!
      return false;

    // Construct a new model around the instance returned by the builder.
    PassPtr.reset(new PassModelT(PassBuilder()));
    return true;
  }

  /// \brief Invalidate a specific analysis pass for an IR module.
  ///
  /// Note that the analysis result can disregard invalidation.
  template <typename PassT> void invalidate(IRUnitT &IR) {
    assert(AnalysisPasses.count(PassT::ID()) &&
           "This analysis pass was not registered prior to being invalidated");
    invalidateImpl(PassT::ID(), IR);
  }

  /// \brief Invalidate analyses cached for an IR unit.
  ///
  /// Walk through all of the analyses pertaining to this unit of IR and
  /// invalidate them unless they are preserved by the PreservedAnalyses set.
  /// We accept the PreservedAnalyses set by value and update it with each
  /// analyis pass which has been successfully invalidated and thus can be
  /// preserved going forward. The updated set is returned.
  PreservedAnalyses invalidate(IRUnitT &IR, PreservedAnalyses PA) {
    // Short circuit for common cases of all analyses being preserved.
    if (PA.areAllPreserved() || PA.preserved<AllAnalysesOn<IRUnitT>>())
      return PA;

    if (DebugLogging)
      dbgs() << "Invalidating all non-preserved analyses for: " << IR.getName()
             << "\n";

    // Clear all the invalidated results associated specifically with this
    // function.
    SmallVector<void *, 8> InvalidatedPassIDs;
    AnalysisResultListT &ResultsList = AnalysisResultLists[&IR];
    for (typename AnalysisResultListT::iterator I = ResultsList.begin(),
                                                E = ResultsList.end();
         I != E;) {
      void *PassID = I->first;

      // Pass the invalidation down to the pass itself to see if it thinks it is
      // necessary. The analysis pass can return false if no action on the part
      // of the analysis manager is required for this invalidation event.
      if (I->second->invalidate(IR, PA)) {
        if (DebugLogging)
          dbgs() << "Invalidating analysis: " << this->lookupPass(PassID).name()
                 << "\n";

        InvalidatedPassIDs.push_back(I->first);
        I = ResultsList.erase(I);
      } else {
        ++I;
      }

      // After handling each pass, we mark it as preserved. Once we've
      // invalidated any stale results, the rest of the system is allowed to
      // start preserving this analysis again.
      PA.preserve(PassID);
    }
    while (!InvalidatedPassIDs.empty())
      AnalysisResults.erase(
          std::make_pair(InvalidatedPassIDs.pop_back_val(), &IR));
    if (ResultsList.empty())
      AnalysisResultLists.erase(&IR);

    return PA;
  }

private:
  /// \brief Lookup a registered analysis pass.
  PassConceptT &lookupPass(void *PassID) {
    typename AnalysisPassMapT::iterator PI = AnalysisPasses.find(PassID);
    assert(PI != AnalysisPasses.end() &&
           "Analysis passes must be registered prior to being queried!");
    return *PI->second;
  }

  /// \brief Lookup a registered analysis pass.
  const PassConceptT &lookupPass(void *PassID) const {
    typename AnalysisPassMapT::const_iterator PI = AnalysisPasses.find(PassID);
    assert(PI != AnalysisPasses.end() &&
           "Analysis passes must be registered prior to being queried!");
    return *PI->second;
  }

  /// \brief Get an analysis result, running the pass if necessary.
  ResultConceptT &getResultImpl(void *PassID, IRUnitT &IR,
                                ExtraArgTs... ExtraArgs) {
    typename AnalysisResultMapT::iterator RI;
    bool Inserted;
    std::tie(RI, Inserted) = AnalysisResults.insert(std::make_pair(
        std::make_pair(PassID, &IR), typename AnalysisResultListT::iterator()));

    // If we don't have a cached result for this function, look up the pass and
    // run it to produce a result, which we then add to the cache.
    if (Inserted) {
      auto &P = this->lookupPass(PassID);
      if (DebugLogging)
        dbgs() << "Running analysis: " << P.name() << "\n";
      AnalysisResultListT &ResultList = AnalysisResultLists[&IR];
      ResultList.emplace_back(PassID, P.run(IR, *this, ExtraArgs...));

      // P.run may have inserted elements into AnalysisResults and invalidated
      // RI.
      RI = AnalysisResults.find(std::make_pair(PassID, &IR));
      assert(RI != AnalysisResults.end() && "we just inserted it!");

      RI->second = std::prev(ResultList.end());
    }

    return *RI->second->second;
  }

  /// \brief Get a cached analysis result or return null.
  ResultConceptT *getCachedResultImpl(void *PassID, IRUnitT &IR) const {
    typename AnalysisResultMapT::const_iterator RI =
        AnalysisResults.find(std::make_pair(PassID, &IR));
    return RI == AnalysisResults.end() ? nullptr : &*RI->second->second;
  }

  /// \brief Invalidate a function pass result.
  void invalidateImpl(void *PassID, IRUnitT &IR) {
    typename AnalysisResultMapT::iterator RI =
        AnalysisResults.find(std::make_pair(PassID, &IR));
    if (RI == AnalysisResults.end())
      return;

    if (DebugLogging)
      dbgs() << "Invalidating analysis: " << this->lookupPass(PassID).name()
             << "\n";
    AnalysisResultLists[&IR].erase(RI->second);
    AnalysisResults.erase(RI);
  }

  /// \brief Map type from module analysis pass ID to pass concept pointer.
  typedef DenseMap<void *, std::unique_ptr<PassConceptT>> AnalysisPassMapT;

  /// \brief Collection of module analysis passes, indexed by ID.
  AnalysisPassMapT AnalysisPasses;

  /// \brief List of function analysis pass IDs and associated concept pointers.
  ///
  /// Requires iterators to be valid across appending new entries and arbitrary
  /// erases. Provides both the pass ID and concept pointer such that it is
  /// half of a bijection and provides storage for the actual result concept.
  typedef std::list<std::pair<
      void *, std::unique_ptr<detail::AnalysisResultConcept<IRUnitT>>>>
      AnalysisResultListT;

  /// \brief Map type from function pointer to our custom list type.
  typedef DenseMap<IRUnitT *, AnalysisResultListT> AnalysisResultListMapT;

  /// \brief Map from function to a list of function analysis results.
  ///
  /// Provides linear time removal of all analysis results for a function and
  /// the ultimate storage for a particular cached analysis result.
  AnalysisResultListMapT AnalysisResultLists;

  /// \brief Map type from a pair of analysis ID and function pointer to an
  /// iterator into a particular result list.
  typedef DenseMap<std::pair<void *, IRUnitT *>,
                   typename AnalysisResultListT::iterator>
      AnalysisResultMapT;

  /// \brief Map from an analysis ID and function to a particular cached
  /// analysis result.
  AnalysisResultMapT AnalysisResults;

  /// \brief A flag indicating whether debug logging is enabled.
  bool DebugLogging;
};

extern template class AnalysisManager<Module>;
/// \brief Convenience typedef for the Module analysis manager.
typedef AnalysisManager<Module> ModuleAnalysisManager;

extern template class AnalysisManager<Function>;
/// \brief Convenience typedef for the Function analysis manager.
typedef AnalysisManager<Function> FunctionAnalysisManager;

/// \brief A module analysis which acts as a proxy for a function analysis
/// manager.
///
/// This primarily proxies invalidation information from the module analysis
/// manager and module pass manager to a function analysis manager. You should
/// never use a function analysis manager from within (transitively) a module
/// pass manager unless your parent module pass has received a proxy result
/// object for it.
///
/// Note that the proxy's result is a move-only object and represents ownership
/// of the validity of the analyses in the \c FunctionAnalysisManager it
/// provides.
template <typename AnalysisManagerT, typename IRUnitT, typename... ExtraArgTs>
class InnerAnalysisManagerProxy
    : public AnalysisInfoMixin<
          InnerAnalysisManagerProxy<AnalysisManagerT, IRUnitT>> {
public:
  class Result {
  public:
    explicit Result(AnalysisManagerT &AM) : AM(&AM) {}
    Result(Result &&Arg) : AM(std::move(Arg.AM)) {
      // We have to null out the analysis manager in the moved-from state
      // because we are taking ownership of the responsibilty to clear the
      // analysis state.
      Arg.AM = nullptr;
    }
    Result &operator=(Result &&RHS) {
      AM = RHS.AM;
      // We have to null out the analysis manager in the moved-from state
      // because we are taking ownership of the responsibilty to clear the
      // analysis state.
      RHS.AM = nullptr;
      return *this;
    }
    ~Result() {
      // AM is cleared in a moved from state where there is nothing to do.
      if (!AM)
        return;

      // Clear out the analysis manager if we're being destroyed -- it means we
      // didn't even see an invalidate call when we got invalidated.
      AM->clear();
    }

    /// \brief Accessor for the analysis manager.
    AnalysisManagerT &getManager() { return *AM; }

    /// \brief Handler for invalidation of the module.
    ///
    /// If this analysis itself is preserved, then we assume that the set of \c
    /// Function objects in the \c Module hasn't changed and thus we don't need
    /// to invalidate *all* cached data associated with a \c Function* in the \c
    /// FunctionAnalysisManager.
    ///
    /// Regardless of whether this analysis is marked as preserved, all of the
    /// analyses in the \c FunctionAnalysisManager are potentially invalidated
    /// based on the set of preserved analyses.
    bool invalidate(IRUnitT &IR, const PreservedAnalyses &PA) {
      // If this proxy isn't marked as preserved, then we can't even invalidate
      // individual function analyses, there may be an invalid set of Function
      // objects in the cache making it impossible to incrementally preserve
      // them. Just clear the entire manager.
      if (!PA.preserved(InnerAnalysisManagerProxy::ID()))
        AM->clear();

      // Return false to indicate that this result is still a valid proxy.
      return false;
    }

  private:
    AnalysisManagerT *AM;
  };

  explicit InnerAnalysisManagerProxy(AnalysisManagerT &AM) : AM(&AM) {}

  /// \brief Run the analysis pass and create our proxy result object.
  ///
  /// This doesn't do any interesting work, it is primarily used to insert our
  /// proxy result object into the module analysis cache so that we can proxy
  /// invalidation to the function analysis manager.
  ///
  /// In debug builds, it will also assert that the analysis manager is empty
  /// as no queries should arrive at the function analysis manager prior to
  /// this analysis being requested.
  Result run(IRUnitT &IR, AnalysisManager<IRUnitT, ExtraArgTs...> &,
             ExtraArgTs...) {
    return Result(*AM);
  }

private:
  friend AnalysisInfoMixin<
      InnerAnalysisManagerProxy<AnalysisManagerT, IRUnitT>>;
  static char PassID;

  AnalysisManagerT *AM;
};

template <typename AnalysisManagerT, typename IRUnitT, typename... ExtraArgTs>
char
    InnerAnalysisManagerProxy<AnalysisManagerT, IRUnitT, ExtraArgTs...>::PassID;

extern template class InnerAnalysisManagerProxy<FunctionAnalysisManager,
                                                Module>;
/// Provide the \c FunctionAnalysisManager to \c Module proxy.
typedef InnerAnalysisManagerProxy<FunctionAnalysisManager, Module>
    FunctionAnalysisManagerModuleProxy;

/// \brief A function analysis which acts as a proxy for a module analysis
/// manager.
///
/// This primarily provides an accessor to a parent module analysis manager to
/// function passes. Only the const interface of the module analysis manager is
/// provided to indicate that once inside of a function analysis pass you
/// cannot request a module analysis to actually run. Instead, the user must
/// rely on the \c getCachedResult API.
///
/// This proxy *doesn't* manage the invalidation in any way. That is handled by
/// the recursive return path of each layer of the pass manager and the
/// returned PreservedAnalysis set.
template <typename AnalysisManagerT, typename IRUnitT, typename... ExtraArgTs>
class OuterAnalysisManagerProxy
    : public AnalysisInfoMixin<
          OuterAnalysisManagerProxy<AnalysisManagerT, IRUnitT>> {
public:
  /// \brief Result proxy object for \c OuterAnalysisManagerProxy.
  class Result {
  public:
    explicit Result(const AnalysisManagerT &AM) : AM(&AM) {}

    const AnalysisManagerT &getManager() const { return *AM; }

    /// \brief Handle invalidation by ignoring it, this pass is immutable.
    bool invalidate(IRUnitT &, const PreservedAnalyses &) { return false; }

  private:
    const AnalysisManagerT *AM;
  };

  OuterAnalysisManagerProxy(const AnalysisManagerT &AM) : AM(&AM) {}

  /// \brief Run the analysis pass and create our proxy result object.
  /// Nothing to see here, it just forwards the \c AM reference into the
  /// result.
  Result run(IRUnitT &, AnalysisManager<IRUnitT, ExtraArgTs...> &,
             ExtraArgTs...) {
    return Result(*AM);
  }

private:
  friend AnalysisInfoMixin<
      OuterAnalysisManagerProxy<AnalysisManagerT, IRUnitT>>;
  static char PassID;

  const AnalysisManagerT *AM;
};

template <typename AnalysisManagerT, typename IRUnitT, typename... ExtraArgTs>
char
    OuterAnalysisManagerProxy<AnalysisManagerT, IRUnitT, ExtraArgTs...>::PassID;

extern template class OuterAnalysisManagerProxy<ModuleAnalysisManager,
                                                Function>;
/// Provide the \c ModuleAnalysisManager to \c Fucntion proxy.
typedef OuterAnalysisManagerProxy<ModuleAnalysisManager, Function>
    ModuleAnalysisManagerFunctionProxy;

/// \brief Trivial adaptor that maps from a module to its functions.
///
/// Designed to allow composition of a FunctionPass(Manager) and
/// a ModulePassManager. Note that if this pass is constructed with a pointer
/// to a \c ModuleAnalysisManager it will run the
/// \c FunctionAnalysisManagerModuleProxy analysis prior to running the function
/// pass over the module to enable a \c FunctionAnalysisManager to be used
/// within this run safely.
///
/// Function passes run within this adaptor can rely on having exclusive access
/// to the function they are run over. They should not read or modify any other
/// functions! Other threads or systems may be manipulating other functions in
/// the module, and so their state should never be relied on.
/// FIXME: Make the above true for all of LLVM's actual passes, some still
/// violate this principle.
///
/// Function passes can also read the module containing the function, but they
/// should not modify that module outside of the use lists of various globals.
/// For example, a function pass is not permitted to add functions to the
/// module.
/// FIXME: Make the above true for all of LLVM's actual passes, some still
/// violate this principle.
template <typename FunctionPassT>
class ModuleToFunctionPassAdaptor
    : public PassInfoMixin<ModuleToFunctionPassAdaptor<FunctionPassT>> {
public:
  explicit ModuleToFunctionPassAdaptor(FunctionPassT Pass)
      : Pass(std::move(Pass)) {}

  /// \brief Runs the function pass across every function in the module.
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    // Setup the function analysis manager from its proxy.
    FunctionAnalysisManager &FAM =
        AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();

    PreservedAnalyses PA = PreservedAnalyses::all();
    for (Function &F : M) {
      if (F.isDeclaration())
        continue;

      PreservedAnalyses PassPA = Pass.run(F, FAM);

      // We know that the function pass couldn't have invalidated any other
      // function's analyses (that's the contract of a function pass), so
      // directly handle the function analysis manager's invalidation here and
      // update our preserved set to reflect that these have already been
      // handled.
      PassPA = FAM.invalidate(F, std::move(PassPA));

      // Then intersect the preserved set so that invalidation of module
      // analyses will eventually occur when the module pass completes.
      PA.intersect(std::move(PassPA));
    }

    // By definition we preserve the proxy. This precludes *any* invalidation
    // of function analyses by the proxy, but that's OK because we've taken
    // care to invalidate analyses in the function analysis manager
    // incrementally above.
    PA.preserve<FunctionAnalysisManagerModuleProxy>();
    return PA;
  }

private:
  FunctionPassT Pass;
};

/// \brief A function to deduce a function pass type and wrap it in the
/// templated adaptor.
template <typename FunctionPassT>
ModuleToFunctionPassAdaptor<FunctionPassT>
createModuleToFunctionPassAdaptor(FunctionPassT Pass) {
  return ModuleToFunctionPassAdaptor<FunctionPassT>(std::move(Pass));
}

/// \brief A template utility pass to force an analysis result to be available.
///
/// If there are extra arguments at the pass's run level there may also be
/// extra arguments to the analysis manager's \c getResult routine. We can't
/// guess how to effectively map the arguments from one to the other, and so
/// this specialization just ignores them.
///
/// Specific patterns of run-method extra arguments and analysis manager extra
/// arguments will have to be defined as appropriate specializations.
template <typename AnalysisT, typename IRUnitT,
          typename AnalysisManagerT = AnalysisManager<IRUnitT>,
          typename... ExtraArgTs>
struct RequireAnalysisPass
    : PassInfoMixin<RequireAnalysisPass<AnalysisT, IRUnitT, AnalysisManagerT,
                                        ExtraArgTs...>> {
  /// \brief Run this pass over some unit of IR.
  ///
  /// This pass can be run over any unit of IR and use any analysis manager
  /// provided they satisfy the basic API requirements. When this pass is
  /// created, these methods can be instantiated to satisfy whatever the
  /// context requires.
  PreservedAnalyses run(IRUnitT &Arg, AnalysisManagerT &AM,
                        ExtraArgTs &&... Args) {
    (void)AM.template getResult<AnalysisT>(Arg,
                                           std::forward<ExtraArgTs>(Args)...);

    return PreservedAnalyses::all();
  }
};

/// \brief A template utility pass to force an analysis result to be
/// invalidated.
///
/// This is a no-op pass which simply forces a specific analysis result to be
/// invalidated when it is run.
template <typename AnalysisT>
struct InvalidateAnalysisPass
    : PassInfoMixin<InvalidateAnalysisPass<AnalysisT>> {
  /// \brief Run this pass over some unit of IR.
  ///
  /// This pass can be run over any unit of IR and use any analysis manager
  /// provided they satisfy the basic API requirements. When this pass is
  /// created, these methods can be instantiated to satisfy whatever the
  /// context requires.
  template <typename IRUnitT, typename AnalysisManagerT, typename... ExtraArgTs>
  PreservedAnalyses run(IRUnitT &Arg, AnalysisManagerT &AM, ExtraArgTs &&...) {
    // We have to directly invalidate the analysis result as we can't
    // enumerate all other analyses and use the preserved set to control it.
    AM.template invalidate<AnalysisT>(Arg);

    return PreservedAnalyses::all();
  }
};

/// \brief A utility pass that does nothing but preserves no analyses.
///
/// As a consequence fo not preserving any analyses, this pass will force all
/// analysis passes to be re-run to produce fresh results if any are needed.
struct InvalidateAllAnalysesPass : PassInfoMixin<InvalidateAllAnalysesPass> {
  /// \brief Run this pass over some unit of IR.
  template <typename IRUnitT, typename AnalysisManagerT, typename... ExtraArgTs>
  PreservedAnalyses run(IRUnitT &, AnalysisManagerT &, ExtraArgTs &&...) {
    return PreservedAnalyses::none();
  }
};

/// A utility pass template that simply runs another pass multiple times.
///
/// This can be useful when debugging or testing passes. It also serves as an
/// example of how to extend the pass manager in ways beyond composition.
template <typename PassT>
class RepeatedPass : public PassInfoMixin<RepeatedPass<PassT>> {
public:
  RepeatedPass(int Count, PassT P) : Count(Count), P(std::move(P)) {}

  template <typename IRUnitT, typename AnalysisManagerT, typename... Ts>
  PreservedAnalyses run(IRUnitT &Arg, AnalysisManagerT &AM, Ts &&... Args) {
    auto PA = PreservedAnalyses::all();
    for (int i = 0; i < Count; ++i)
      PA.intersect(P.run(Arg, AM, std::forward<Ts>(Args)...));
    return PA;
  }

private:
  int Count;
  PassT P;
};

template <typename PassT>
RepeatedPass<PassT> createRepeatedPass(int Count, PassT P) {
  return RepeatedPass<PassT>(Count, std::move(P));
}

}

#endif
