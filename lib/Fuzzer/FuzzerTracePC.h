//===- FuzzerTracePC.h - Internal header for the Fuzzer ---------*- C++ -* ===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// fuzzer::TracePC
//===----------------------------------------------------------------------===//

#ifndef LLVM_FUZZER_TRACE_PC
#define LLVM_FUZZER_TRACE_PC

#include <set>

#include "FuzzerDefs.h"
#include "FuzzerValueBitMap.h"

namespace fuzzer {

// TableOfRecentCompares (TORC) remembers the most recently performed
// comparisons of type T.
// We record the arguments of CMP instructions in this table unconditionally
// because it seems cheaper this way than to compute some expensive
// conditions inside __sanitizer_cov_trace_cmp*.
// After the unit has been executed we may decide to use the contents of
// this table to populate a Dictionary.
template<class T, size_t kSizeT>
struct TableOfRecentCompares {
  static const size_t kSize = kSizeT;
  struct Pair {
    T A, B;
  };
  void Insert(size_t Idx, T Arg1, T Arg2) {
    Idx = Idx % kSize;
    Table[Idx].A = Arg1;
    Table[Idx].B = Arg2;
  }

  Pair Get(size_t I) { return Table[I % kSize]; }

  Pair Table[kSize];
};

class TracePC {
 public:
  static const size_t kFeatureSetSize = ValueBitMap::kNumberOfItems;

  void HandleTrace(uint32_t *guard, uintptr_t PC);
  void HandleInit(uint32_t *start, uint32_t *stop);
  void HandleCallerCallee(uintptr_t Caller, uintptr_t Callee);
  void HandleValueProfile(size_t Value) { ValueProfileMap.AddValue(Value); }
  template <class T> void HandleCmp(void *PC, T Arg1, T Arg2);
  size_t GetTotalPCCoverage();
  void SetUseCounters(bool UC) { UseCounters = UC; }
  void SetUseValueProfile(bool VP) { UseValueProfile = VP; }
  void SetPrintNewPCs(bool P) { DoPrintNewPCs = P; }
  size_t FinalizeTrace(InputCorpus *C, size_t InputSize, bool Shrink);
  bool UpdateValueProfileMap(ValueBitMap *MaxValueProfileMap) {
    return UseValueProfile && MaxValueProfileMap->MergeFrom(ValueProfileMap);
  }

  size_t GetNewPCIDs(uintptr_t **NewPCIDsPtr) {
    *NewPCIDsPtr = NewPCIDs;
    return Min(kMaxNewPCIDs, NumNewPCIDs);
  }

  uintptr_t GetPCbyPCID(uintptr_t PCID) { return PCs[PCID]; }

  void ResetMaps() {
    NumNewPCIDs = 0;
    ValueProfileMap.Reset();
    memset(Counters, 0, sizeof(Counters));
  }

  void UpdateFeatureSet(size_t CurrentElementIdx, size_t CurrentElementSize);
  void PrintFeatureSet();

  void ResetGuards();

  void PrintModuleInfo();

  void PrintCoverage();

  void AddValueForMemcmp(void *caller_pc, const void *s1, const void *s2,
                         size_t n);
  void AddValueForStrcmp(void *caller_pc, const char *s1, const char *s2,
                         size_t n);

  bool UsingTracePcGuard() const {return NumModules; }

  static const size_t kTORCSize = 1 << 5;
  TableOfRecentCompares<uint32_t, kTORCSize> TORC4;
  TableOfRecentCompares<uint64_t, kTORCSize> TORC8;

  void PrintNewPCs();

private:
  bool UseCounters = false;
  bool UseValueProfile = false;
  bool DoPrintNewPCs = false;

  static const size_t kMaxNewPCIDs = 1024;
  uintptr_t NewPCIDs[kMaxNewPCIDs];
  size_t NumNewPCIDs = 0;
  void AddNewPCID(uintptr_t PCID) {
    NewPCIDs[(NumNewPCIDs++) % kMaxNewPCIDs] = PCID;
  }

  struct Module {
    uint32_t *Start, *Stop;
  };

  Module Modules[4096];
  size_t NumModules = 0;
  size_t NumGuards = 0;

  static const size_t kNumCounters = 1 << 14;
  alignas(8) uint8_t Counters[kNumCounters];

  void TORCInsert(size_t Idx, uint8_t Arg1, uint8_t Arg2) {
    // Do nothing, too small to be interesting.
  }
  void TORCInsert(size_t Idx, uint16_t Arg1, uint16_t Arg2) {
    // Do nothing, these don't usually hapen.
  }
  void TORCInsert(size_t Idx, uint32_t Arg1, uint32_t Arg2) {
    TORC4.Insert(Idx, Arg1, Arg2);
  }
  void TORCInsert(size_t Idx, uint64_t Arg1, uint64_t Arg2) {
    TORC8.Insert(Idx, Arg1, Arg2);
  }

  static const size_t kNumPCs = 1 << 24;
  uintptr_t PCs[kNumPCs];

  std::set<uintptr_t> *PrintedPCs;

  ValueBitMap ValueProfileMap;
};

extern TracePC TPC;

}  // namespace fuzzer

#endif  // LLVM_FUZZER_TRACE_PC
