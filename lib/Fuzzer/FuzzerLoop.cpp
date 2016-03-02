//===- FuzzerLoop.cpp - Fuzzer's main loop --------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Fuzzer's main loop.
//===----------------------------------------------------------------------===//

#include "FuzzerInternal.h"
#include <algorithm>
#include <cstring>
#include <memory>

#if defined(__has_include)
#if __has_include(<sanitizer / coverage_interface.h>)
#include <sanitizer/coverage_interface.h>
#endif
#endif

extern "C" {
// Re-declare some of the sanitizer functions as "weak" so that
// libFuzzer can be linked w/o the sanitizers and sanitizer-coverage
// (in which case it will complain at start-up time).
__attribute__((weak)) void __sanitizer_print_stack_trace();
__attribute__((weak)) void __sanitizer_reset_coverage();
__attribute__((weak)) size_t __sanitizer_get_total_unique_caller_callee_pairs();
__attribute__((weak)) size_t __sanitizer_get_total_unique_coverage();
__attribute__((weak)) void
__sanitizer_set_death_callback(void (*callback)(void));
__attribute__((weak)) size_t __sanitizer_get_number_of_counters();
__attribute__((weak)) uintptr_t
__sanitizer_update_counter_bitset_and_clear_counters(uint8_t *bitset);
__attribute__((weak)) uintptr_t
__sanitizer_get_coverage_pc_buffer(uintptr_t **data);

__attribute__((weak)) size_t LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size,
                                                     size_t MaxSize,
                                                     unsigned int Seed);
}

namespace fuzzer {
static const size_t kMaxUnitSizeToPrint = 256;

static void MissingWeakApiFunction(const char *FnName) {
  Printf("ERROR: %s is not defined. Exiting.\n"
         "Did you use -fsanitize-coverage=... to build your code?\n",
         FnName);
  exit(1);
}

#define CHECK_WEAK_API_FUNCTION(fn)                                            \
  do {                                                                         \
    if (!fn)                                                                   \
      MissingWeakApiFunction(#fn);                                             \
  } while (false)

// Only one Fuzzer per process.
static Fuzzer *F;

size_t Mutate(uint8_t *Data, size_t Size, size_t MaxSize) {
  assert(F);
  return F->GetMD().Mutate(Data, Size, MaxSize);
}

Fuzzer::Fuzzer(UserCallback CB, MutationDispatcher &MD, FuzzingOptions Options)
    : CB(CB), MD(MD), Options(Options) {
  SetDeathCallback();
  InitializeTraceState();
  assert(!F);
  F = this;
}

void Fuzzer::SetDeathCallback() {
  CHECK_WEAK_API_FUNCTION(__sanitizer_set_death_callback);
  __sanitizer_set_death_callback(StaticDeathCallback);
}

void Fuzzer::StaticDeathCallback() {
  assert(F);
  F->DeathCallback();
}

void Fuzzer::DumpCurrentUnit(const char *Prefix) {
  if (CurrentUnitSize <= kMaxUnitSizeToPrint) {
    PrintHexArray(CurrentUnitData, CurrentUnitSize, "\n");
    PrintASCII(CurrentUnitData, CurrentUnitSize, "\n");
  }
  WriteUnitToFileWithPrefix(
      {CurrentUnitData, CurrentUnitData + CurrentUnitSize}, Prefix);
}

void Fuzzer::DeathCallback() {
  if (!CurrentUnitSize) return;
  Printf("DEATH:\n");
  DumpCurrentUnit("crash-");
  PrintFinalStats();
}

void Fuzzer::StaticAlarmCallback() {
  assert(F);
  F->AlarmCallback();
}

void Fuzzer::StaticCrashSignalCallback() {
  assert(F);
  F->CrashCallback();
}

void Fuzzer::StaticInterruptCallback() {
  assert(F);
  F->InterruptCallback();
}

void Fuzzer::CrashCallback() {
  Printf("==%d== ERROR: libFuzzer: deadly signal\n", GetPid());
  if (__sanitizer_print_stack_trace)
    __sanitizer_print_stack_trace();
  Printf("NOTE: libFuzzer has rudimentary signal handlers.\n"
         "      Combine libFuzzer with AddressSanitizer or similar for better "
         "crash reports.\n");
  Printf("SUMMARY: libFuzzer: deadly signal\n");
  DumpCurrentUnit("crash-");
  PrintFinalStats();
  exit(Options.ErrorExitCode);
}

void Fuzzer::InterruptCallback() {
  Printf("==%d== libFuzzer: run interrupted; exiting\n", GetPid());
  PrintFinalStats();
  exit(0);
}

void Fuzzer::AlarmCallback() {
  assert(Options.UnitTimeoutSec > 0);
  if (!CurrentUnitSize)
    return; // We have not started running units yet.
  size_t Seconds =
      duration_cast<seconds>(system_clock::now() - UnitStartTime).count();
  if (Seconds == 0)
    return;
  if (Options.Verbosity >= 2)
    Printf("AlarmCallback %zd\n", Seconds);
  if (Seconds >= (size_t)Options.UnitTimeoutSec) {
    Printf("ALARM: working on the last Unit for %zd seconds\n", Seconds);
    Printf("       and the timeout value is %d (use -timeout=N to change)\n",
           Options.UnitTimeoutSec);
    DumpCurrentUnit("timeout-");
    Printf("==%d== ERROR: libFuzzer: timeout after %d seconds\n", GetPid(),
           Seconds);
    if (__sanitizer_print_stack_trace)
      __sanitizer_print_stack_trace();
    Printf("SUMMARY: libFuzzer: timeout\n");
    PrintFinalStats();
    exit(Options.TimeoutExitCode);
  }
}

void Fuzzer::PrintStats(const char *Where, const char *End) {
  size_t ExecPerSec = execPerSec();
  if (Options.OutputCSV) {
    static bool csvHeaderPrinted = false;
    if (!csvHeaderPrinted) {
      csvHeaderPrinted = true;
      Printf("runs,block_cov,bits,cc_cov,corpus,execs_per_sec,tbms,reason\n");
    }
    Printf("%zd,%zd,%zd,%zd,%zd,%zd,%zd,%s\n", TotalNumberOfRuns,
           LastRecordedBlockCoverage, TotalBits(),
           LastRecordedCallerCalleeCoverage, Corpus.size(), ExecPerSec,
           TotalNumberOfExecutedTraceBasedMutations, Where);
  }

  if (!Options.Verbosity)
    return;
  Printf("#%zd\t%s", TotalNumberOfRuns, Where);
  if (LastRecordedBlockCoverage)
    Printf(" cov: %zd", LastRecordedBlockCoverage);
  if (LastRecordedPcMapSize)
    Printf(" path: %zd", LastRecordedPcMapSize);
  if (auto TB = TotalBits())
    Printf(" bits: %zd", TB);
  if (LastRecordedCallerCalleeCoverage)
    Printf(" indir: %zd", LastRecordedCallerCalleeCoverage);
  Printf(" units: %zd exec/s: %zd", Corpus.size(), ExecPerSec);
  if (TotalNumberOfExecutedTraceBasedMutations)
    Printf(" tbm: %zd", TotalNumberOfExecutedTraceBasedMutations);
  Printf("%s", End);
}

void Fuzzer::PrintFinalStats() {
  if (!Options.PrintFinalStats) return;
  size_t ExecPerSec = execPerSec();
  Printf("stat::number_of_executed_units: %zd\n", TotalNumberOfRuns);
  Printf("stat::average_exec_per_sec:     %zd\n", ExecPerSec);
  Printf("stat::new_units_added:          %zd\n", NumberOfNewUnitsAdded);
  Printf("stat::slowest_unit_time_sec:    %zd\n", TimeOfLongestUnitInSeconds);
  Printf("stat::peak_rss_mb:              %zd\n", GetPeakRSSMb());
}

void Fuzzer::RereadOutputCorpus() {
  if (Options.OutputCorpus.empty())
    return;
  std::vector<Unit> AdditionalCorpus;
  ReadDirToVectorOfUnits(Options.OutputCorpus.c_str(), &AdditionalCorpus,
                         &EpochOfLastReadOfOutputCorpus, Options.MaxLen);
  if (Corpus.empty()) {
    Corpus = AdditionalCorpus;
    return;
  }
  if (!Options.Reload)
    return;
  if (Options.Verbosity >= 2)
    Printf("Reload: read %zd new units.\n", AdditionalCorpus.size());
  for (auto &X : AdditionalCorpus) {
    if (X.size() > (size_t)Options.MaxLen)
      X.resize(Options.MaxLen);
    if (UnitHashesAddedToCorpus.insert(Hash(X)).second) {
      if (RunOne(X)) {
        Corpus.push_back(X);
        UpdateCorpusDistribution();
        PrintStats("RELOAD");
      }
    }
  }
}

void Fuzzer::ShuffleAndMinimize() {
  bool PreferSmall = (Options.PreferSmallDuringInitialShuffle == 1 ||
                      (Options.PreferSmallDuringInitialShuffle == -1 &&
                       MD.GetRand().RandBool()));
  if (Options.Verbosity)
    Printf("PreferSmall: %d\n", PreferSmall);
  PrintStats("READ  ");
  std::vector<Unit> NewCorpus;
  if (Options.ShuffleAtStartUp) {
    std::random_shuffle(Corpus.begin(), Corpus.end(), MD.GetRand());
    if (PreferSmall)
      std::stable_sort(
          Corpus.begin(), Corpus.end(),
          [](const Unit &A, const Unit &B) { return A.size() < B.size(); });
  }
  Unit U;
  for (const auto &C : Corpus) {
    for (size_t First = 0; First < 1; First++) {
      U.clear();
      size_t Last = std::min(First + Options.MaxLen, C.size());
      U.insert(U.begin(), C.begin() + First, C.begin() + Last);
      if (Options.OnlyASCII)
        ToASCII(U.data(), U.size());
      if (RunOne(U)) {
        NewCorpus.push_back(U);
        if (Options.Verbosity >= 2)
          Printf("NEW0: %zd L %zd\n", LastRecordedBlockCoverage, U.size());
      }
    }
  }
  Corpus = NewCorpus;
  UpdateCorpusDistribution();
  for (auto &X : Corpus)
    UnitHashesAddedToCorpus.insert(Hash(X));
  PrintStats("INITED");
}

bool Fuzzer::RunOne(const uint8_t *Data, size_t Size) {
  UnitStartTime = system_clock::now();
  TotalNumberOfRuns++;

  PrepareCoverageBeforeRun();
  ExecuteCallback(Data, Size);
  bool Res = CheckCoverageAfterRun();

  auto UnitStopTime = system_clock::now();
  auto TimeOfUnit =
      duration_cast<seconds>(UnitStopTime - UnitStartTime).count();
  if (!(TotalNumberOfRuns & (TotalNumberOfRuns - 1)) &&
      secondsSinceProcessStartUp() >= 2)
    PrintStats("pulse ");
  if (TimeOfUnit > TimeOfLongestUnitInSeconds &&
      TimeOfUnit >= Options.ReportSlowUnits) {
    TimeOfLongestUnitInSeconds = TimeOfUnit;
    Printf("Slowest unit: %zd s:\n", TimeOfLongestUnitInSeconds);
    WriteUnitToFileWithPrefix({Data, Data + Size}, "slow-unit-");
  }
  return Res;
}

void Fuzzer::RunOneAndUpdateCorpus(uint8_t *Data, size_t Size) {
  if (TotalNumberOfRuns >= Options.MaxNumberOfRuns)
    return;
  if (Options.OnlyASCII)
    ToASCII(Data, Size);
  if (RunOne(Data, Size))
    ReportNewCoverage({Data, Data + Size});
}

void Fuzzer::ExecuteCallback(const uint8_t *Data, size_t Size) {
  // We copy the contents of Unit into a separate heap buffer
  // so that we reliably find buffer overflows in it.
  std::unique_ptr<uint8_t[]> DataCopy(new uint8_t[Size]);
  memcpy(DataCopy.get(), Data, Size);
  AssignTaintLabels(DataCopy.get(), Size);
  CurrentUnitData = DataCopy.get();
  CurrentUnitSize = Size;
  int Res = CB(DataCopy.get(), Size);
  (void)Res;
  assert(Res == 0);
  CurrentUnitData = nullptr;
  CurrentUnitSize = 0;
}

size_t Fuzzer::RecordBlockCoverage() {
  CHECK_WEAK_API_FUNCTION(__sanitizer_get_total_unique_coverage);
  uintptr_t PrevCoverage = LastRecordedBlockCoverage;
  LastRecordedBlockCoverage = __sanitizer_get_total_unique_coverage();

  if (PrevCoverage == LastRecordedBlockCoverage || !Options.PrintNewCovPcs)
    return LastRecordedBlockCoverage;

  uintptr_t PrevBufferLen = LastCoveragePcBufferLen;
  uintptr_t *CoverageBuf;
  LastCoveragePcBufferLen = __sanitizer_get_coverage_pc_buffer(&CoverageBuf);
  assert(CoverageBuf);
  for (size_t i = PrevBufferLen; i < LastCoveragePcBufferLen; ++i) {
    Printf("%p\n", CoverageBuf[i]);
  }

  return LastRecordedBlockCoverage;
}

size_t Fuzzer::RecordCallerCalleeCoverage() {
  if (!Options.UseIndirCalls)
    return 0;
  if (!__sanitizer_get_total_unique_caller_callee_pairs)
    return 0;
  return LastRecordedCallerCalleeCoverage =
             __sanitizer_get_total_unique_caller_callee_pairs();
}

void Fuzzer::PrepareCoverageBeforeRun() {
  if (Options.UseCounters) {
    size_t NumCounters = __sanitizer_get_number_of_counters();
    CounterBitmap.resize(NumCounters);
    __sanitizer_update_counter_bitset_and_clear_counters(0);
  }
  RecordBlockCoverage();
  RecordCallerCalleeCoverage();
}

bool Fuzzer::CheckCoverageAfterRun() {
  size_t OldCoverage = LastRecordedBlockCoverage;
  size_t NewCoverage = RecordBlockCoverage();
  size_t OldCallerCalleeCoverage = LastRecordedCallerCalleeCoverage;
  size_t NewCallerCalleeCoverage = RecordCallerCalleeCoverage();
  size_t NumNewBits = 0;
  size_t OldPcMapSize = LastRecordedPcMapSize;
  PcMapMergeCurrentToCombined();
  size_t NewPcMapSize = PcMapCombinedSize();
  LastRecordedPcMapSize = NewPcMapSize;
  if (NewPcMapSize > OldPcMapSize)
    return true;
  if (Options.UseCounters)
    NumNewBits = __sanitizer_update_counter_bitset_and_clear_counters(
        CounterBitmap.data());
  return NewCoverage > OldCoverage ||
         NewCallerCalleeCoverage > OldCallerCalleeCoverage || NumNewBits;
}

void Fuzzer::WriteToOutputCorpus(const Unit &U) {
  if (Options.OutputCorpus.empty())
    return;
  std::string Path = DirPlusFile(Options.OutputCorpus, Hash(U));
  WriteToFile(U, Path);
  if (Options.Verbosity >= 2)
    Printf("Written to %s\n", Path.c_str());
  assert(!Options.OnlyASCII || IsASCII(U));
}

void Fuzzer::WriteUnitToFileWithPrefix(const Unit &U, const char *Prefix) {
  if (!Options.SaveArtifacts)
    return;
  std::string Path = Options.ArtifactPrefix + Prefix + Hash(U);
  if (!Options.ExactArtifactPath.empty())
    Path = Options.ExactArtifactPath; // Overrides ArtifactPrefix.
  WriteToFile(U, Path);
  Printf("artifact_prefix='%s'; Test unit written to %s\n",
         Options.ArtifactPrefix.c_str(), Path.c_str());
  if (U.size() <= kMaxUnitSizeToPrint)
    Printf("Base64: %s\n", Base64(U).c_str());
}

void Fuzzer::SaveCorpus() {
  if (Options.OutputCorpus.empty())
    return;
  for (const auto &U : Corpus)
    WriteToFile(U, DirPlusFile(Options.OutputCorpus, Hash(U)));
  if (Options.Verbosity)
    Printf("Written corpus of %zd files to %s\n", Corpus.size(),
           Options.OutputCorpus.c_str());
}

void Fuzzer::PrintStatusForNewUnit(const Unit &U) {
  if (!Options.PrintNEW)
    return;
  PrintStats("NEW   ", "");
  if (Options.Verbosity) {
    Printf(" L: %zd ", U.size());
    MD.PrintMutationSequence();
    Printf("\n");
  }
}

void Fuzzer::ReportNewCoverage(const Unit &U) {
  Corpus.push_back(U);
  UpdateCorpusDistribution();
  UnitHashesAddedToCorpus.insert(Hash(U));
  MD.RecordSuccessfulMutationSequence();
  PrintStatusForNewUnit(U);
  WriteToOutputCorpus(U);
  NumberOfNewUnitsAdded++;
}

void Fuzzer::Merge(const std::vector<std::string> &Corpora) {
  if (Corpora.size() <= 1) {
    Printf("Merge requires two or more corpus dirs\n");
    return;
  }
  auto InitialCorpusDir = Corpora[0];
  ReadDir(InitialCorpusDir, nullptr, Options.MaxLen);
  Printf("Merge: running the initial corpus '%s' of %d units\n",
         InitialCorpusDir.c_str(), Corpus.size());
  for (auto &U : Corpus)
    RunOne(U);

  std::vector<std::string> ExtraCorpora(Corpora.begin() + 1, Corpora.end());

  size_t NumTried = 0;
  size_t NumMerged = 0;
  for (auto &C : ExtraCorpora) {
    Corpus.clear();
    ReadDir(C, nullptr, Options.MaxLen);
    Printf("Merge: merging the extra corpus '%s' of %zd units\n", C.c_str(),
           Corpus.size());
    for (auto &U : Corpus) {
      NumTried++;
      if (RunOne(U)) {
        WriteToOutputCorpus(U);
        NumMerged++;
      }
    }
  }
  Printf("Merge: written %zd out of %zd units\n", NumMerged, NumTried);
}

void Fuzzer::MutateAndTestOne() {
  MD.StartMutationSequence();

  auto &U = ChooseUnitToMutate();
  MutateInPlaceHere.resize(Options.MaxLen);
  memcpy(MutateInPlaceHere.data(), U.data(), U.size());
  size_t Size = U.size();

  for (int i = 0; i < Options.MutateDepth; i++) {
    size_t NewSize = 0;
    if (LLVMFuzzerCustomMutator)
      NewSize = LLVMFuzzerCustomMutator(MutateInPlaceHere.data(), Size,
                                        Options.MaxLen, MD.GetRand().Rand());
    else
      NewSize = MD.Mutate(MutateInPlaceHere.data(), Size, Options.MaxLen);
    assert(NewSize > 0 && "Mutator returned empty unit");
    assert(NewSize <= (size_t)Options.MaxLen &&
           "Mutator return overisized unit");
    Size = NewSize;
    if (i == 0)
      StartTraceRecording();
    RunOneAndUpdateCorpus(MutateInPlaceHere.data(), Size);
    StopTraceRecording();
  }
}

// Returns an index of random unit from the corpus to mutate.
// Hypothesis: units added to the corpus last are more likely to be interesting.
// This function gives more weight to the more recent units.
size_t Fuzzer::ChooseUnitIdxToMutate() {
  size_t Idx =
      static_cast<size_t>(CorpusDistribution(MD.GetRand().Get_mt19937()));
  assert(Idx < Corpus.size());
  return Idx;
}

// Experimental search heuristic: drilling.
// - Read, shuffle, execute and minimize the corpus.
// - Choose one random unit.
// - Reset the coverage.
// - Start fuzzing as if the chosen unit was the only element of the corpus.
// - When done, reset the coverage again.
// - Merge the newly created corpus into the original one.
void Fuzzer::Drill() {
  // The corpus is already read, shuffled, and minimized.
  assert(!Corpus.empty());
  Options.PrintNEW = false; // Don't print NEW status lines when drilling.

  Unit U = ChooseUnitToMutate();

  CHECK_WEAK_API_FUNCTION(__sanitizer_reset_coverage);
  __sanitizer_reset_coverage();

  std::vector<Unit> SavedCorpus;
  SavedCorpus.swap(Corpus);
  Corpus.push_back(U);
  UpdateCorpusDistribution();
  assert(Corpus.size() == 1);
  RunOne(U);
  PrintStats("DRILL ");
  std::string SavedOutputCorpusPath; // Don't write new units while drilling.
  SavedOutputCorpusPath.swap(Options.OutputCorpus);
  Loop();

  __sanitizer_reset_coverage();

  PrintStats("REINIT");
  SavedOutputCorpusPath.swap(Options.OutputCorpus);
  for (auto &U : SavedCorpus)
    RunOne(U);
  PrintStats("MERGE ");
  Options.PrintNEW = true;
  size_t NumMerged = 0;
  for (auto &U : Corpus) {
    if (RunOne(U)) {
      PrintStatusForNewUnit(U);
      NumMerged++;
      WriteToOutputCorpus(U);
    }
  }
  PrintStats("MERGED");
  if (NumMerged && Options.Verbosity)
    Printf("Drilling discovered %zd new units\n", NumMerged);
}

void Fuzzer::Loop() {
  system_clock::time_point LastCorpusReload = system_clock::now();
  if (Options.DoCrossOver)
    MD.SetCorpus(&Corpus);
  while (true) {
    SyncCorpus();
    auto Now = system_clock::now();
    if (duration_cast<seconds>(Now - LastCorpusReload).count()) {
      RereadOutputCorpus();
      LastCorpusReload = Now;
    }
    if (TotalNumberOfRuns >= Options.MaxNumberOfRuns)
      break;
    if (Options.MaxTotalTimeSec > 0 &&
        secondsSinceProcessStartUp() >
            static_cast<size_t>(Options.MaxTotalTimeSec))
      break;
    // Perform several mutations and runs.
    MutateAndTestOne();
  }

  PrintStats("DONE  ", "\n");
  MD.PrintRecommendedDictionary();
}

void Fuzzer::SyncCorpus() {
  if (Options.SyncCommand.empty() || Options.OutputCorpus.empty())
    return;
  auto Now = system_clock::now();
  if (duration_cast<seconds>(Now - LastExternalSync).count() <
      Options.SyncTimeout)
    return;
  LastExternalSync = Now;
  ExecuteCommand(Options.SyncCommand + " " + Options.OutputCorpus);
}

void Fuzzer::UpdateCorpusDistribution() {
  size_t N = Corpus.size();
  std::vector<double> Intervals(N + 1);
  std::vector<double> Weights(N);
  std::iota(Intervals.begin(), Intervals.end(), 0);
  std::iota(Weights.begin(), Weights.end(), 1);
  CorpusDistribution = std::piecewise_constant_distribution<double>(
      Intervals.begin(), Intervals.end(), Weights.begin());
}

} // namespace fuzzer
