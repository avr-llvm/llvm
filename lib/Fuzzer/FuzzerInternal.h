//===- FuzzerInternal.h - Internal header for the Fuzzer --------*- C++ -* ===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Define the main class fuzzer::Fuzzer and most functions.
//===----------------------------------------------------------------------===//

#ifndef LLVM_FUZZER_INTERNAL_H
#define LLVM_FUZZER_INTERNAL_H

#include <algorithm>
#include <cassert>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <random>
#include <string.h>
#include <string>
#include <unordered_set>
#include <vector>

#include "FuzzerInterface.h"

namespace fuzzer {
using namespace std::chrono;
typedef std::vector<uint8_t> Unit;

// A simple POD sized array of bytes.
template <size_t kMaxSize> class FixedWord {
public:
  FixedWord() {}
  FixedWord(const uint8_t *B, uint8_t S) { Set(B, S); }

  void Set(const uint8_t *B, uint8_t S) {
    assert(S <= kMaxSize);
    memcpy(Data, B, S);
    Size = S;
  }

  bool operator==(const FixedWord<kMaxSize> &w) const {
    return Size == w.Size && 0 == memcmp(Data, w.Data, Size);
  }

  bool operator<(const FixedWord<kMaxSize> &w) const {
    if (Size != w.Size)
      return Size < w.Size;
    return memcmp(Data, w.Data, Size) < 0;
  }

  static size_t GetMaxSize() { return kMaxSize; }
  const uint8_t *data() const { return Data; }
  uint8_t size() const { return Size; }

private:
  uint8_t Size = 0;
  uint8_t Data[kMaxSize];
};

typedef FixedWord<27> Word; // 28 bytes.

bool IsFile(const std::string &Path);
std::string FileToString(const std::string &Path);
Unit FileToVector(const std::string &Path, size_t MaxSize = 0);
void ReadDirToVectorOfUnits(const char *Path, std::vector<Unit> *V,
                            long *Epoch, size_t MaxSize);
void WriteToFile(const Unit &U, const std::string &Path);
void CopyFileToErr(const std::string &Path);
// Returns "Dir/FileName" or equivalent for the current OS.
std::string DirPlusFile(const std::string &DirPath,
                        const std::string &FileName);

void Printf(const char *Fmt, ...);
void PrintHexArray(const Unit &U, const char *PrintAfter = "");
void PrintHexArray(const uint8_t *Data, size_t Size,
                   const char *PrintAfter = "");
void PrintASCII(const uint8_t *Data, size_t Size, const char *PrintAfter = "");
void PrintASCII(const Unit &U, const char *PrintAfter = "");
void PrintASCII(const Word &W, const char *PrintAfter = "");
std::string Hash(const Unit &U);
void SetTimer(int Seconds);
void SetSigSegvHandler();
void SetSigBusHandler();
void SetSigAbrtHandler();
void SetSigIllHandler();
void SetSigFpeHandler();
void SetSigIntHandler();
std::string Base64(const Unit &U);
int ExecuteCommand(const std::string &Command);
size_t GetPeakRSSMb();

// Private copy of SHA1 implementation.
static const int kSHA1NumBytes = 20;
// Computes SHA1 hash of 'Len' bytes in 'Data', writes kSHA1NumBytes to 'Out'.
void ComputeSHA1(const uint8_t *Data, size_t Len, uint8_t *Out);

// Changes U to contain only ASCII (isprint+isspace) characters.
// Returns true iff U has been changed.
bool ToASCII(uint8_t *Data, size_t Size);
bool IsASCII(const Unit &U);

int NumberOfCpuCores();
int GetPid();

// Clears the current PC Map.
void PcMapResetCurrent();
// Merges the current PC Map into the combined one, and clears the former.
void PcMapMergeCurrentToCombined();
// Returns the size of the combined PC Map.
size_t PcMapCombinedSize();

class Random {
 public:
  Random(unsigned int seed) : R(seed) {}
  size_t Rand() { return R(); }
  size_t RandBool() { return Rand() % 2; }
  size_t operator()(size_t n) { return n ? Rand() % n : 0; }
  std::mt19937 &Get_mt19937() { return R; }
 private:
  std::mt19937 R;
};

// Dictionary.

// Parses one dictionary entry.
// If successfull, write the enty to Unit and returns true,
// otherwise returns false.
bool ParseOneDictionaryEntry(const std::string &Str, Unit *U);
// Parses the dictionary file, fills Units, returns true iff all lines
// were parsed succesfully.
bool ParseDictionaryFile(const std::string &Text, std::vector<Unit> *Units);

class DictionaryEntry {
 public:
  DictionaryEntry() {}
  DictionaryEntry(Word W) : W(W) {}
  DictionaryEntry(Word W, size_t PositionHint) : W(W), PositionHint(PositionHint) {}
  const Word &GetW() const { return W; }

  bool HasPositionHint() const { return PositionHint != std::numeric_limits<size_t>::max(); }
  size_t GetPositionHint() const {
    assert(HasPositionHint());
    return PositionHint;
  }
  void IncUseCount() { UseCount++; }
  void IncSuccessCount() { SuccessCount++; }
  size_t GetUseCount() const { return UseCount; }
  size_t GetSuccessCount() const {return SuccessCount; }

private:
  Word W;
  size_t PositionHint = std::numeric_limits<size_t>::max();
  size_t UseCount = 0;
  size_t SuccessCount = 0;
};

class Dictionary {
 public:
  static const size_t kMaxDictSize = 1 << 14;

  bool ContainsWord(const Word &W) const {
    return std::any_of(begin(), end(), [&](const DictionaryEntry &DE) {
      return DE.GetW() == W;
    });
  }
  const DictionaryEntry *begin() const { return &DE[0]; }
  const DictionaryEntry *end() const { return begin() + Size; }
  DictionaryEntry & operator[] (size_t Idx) {
    assert(Idx < Size);
    return DE[Idx];
  }
  void push_back(DictionaryEntry DE) {
    if (Size < kMaxDictSize)
      this->DE[Size++] = DE;
  }
  void clear() { Size = 0; }
  bool empty() const { return Size == 0; }
  size_t size() const { return Size; }

private:
  DictionaryEntry DE[kMaxDictSize];
  size_t Size = 0;
};

class MutationDispatcher {
public:
  MutationDispatcher(Random &Rand) : Rand(Rand) {}
  ~MutationDispatcher() {}
  /// Indicate that we are about to start a new sequence of mutations.
  void StartMutationSequence();
  /// Print the current sequence of mutations.
  void PrintMutationSequence();
  /// Indicate that the current sequence of mutations was successfull.
  void RecordSuccessfulMutationSequence();
  /// Mutates data by shuffling bytes.
  size_t Mutate_ShuffleBytes(uint8_t *Data, size_t Size, size_t MaxSize);
  /// Mutates data by erasing a byte.
  size_t Mutate_EraseByte(uint8_t *Data, size_t Size, size_t MaxSize);
  /// Mutates data by inserting a byte.
  size_t Mutate_InsertByte(uint8_t *Data, size_t Size, size_t MaxSize);
  /// Mutates data by chanding one byte.
  size_t Mutate_ChangeByte(uint8_t *Data, size_t Size, size_t MaxSize);
  /// Mutates data by chanding one bit.
  size_t Mutate_ChangeBit(uint8_t *Data, size_t Size, size_t MaxSize);

  /// Mutates data by adding a word from the manual dictionary.
  size_t Mutate_AddWordFromManualDictionary(uint8_t *Data, size_t Size,
                                            size_t MaxSize);

  /// Mutates data by adding a word from the temporary automatic dictionary.
  size_t Mutate_AddWordFromTemporaryAutoDictionary(uint8_t *Data, size_t Size,
                                                   size_t MaxSize);

  /// Mutates data by adding a word from the persistent automatic dictionary.
  size_t Mutate_AddWordFromPersistentAutoDictionary(uint8_t *Data, size_t Size,
                                                    size_t MaxSize);

  /// Tries to find an ASCII integer in Data, changes it to another ASCII int.
  size_t Mutate_ChangeASCIIInteger(uint8_t *Data, size_t Size, size_t MaxSize);

  /// CrossOver Data with some other element of the corpus.
  size_t Mutate_CrossOver(uint8_t *Data, size_t Size, size_t MaxSize);

  /// Applies one of the above mutations.
  /// Returns the new size of data which could be up to MaxSize.
  size_t Mutate(uint8_t *Data, size_t Size, size_t MaxSize);

  /// Creates a cross-over of two pieces of Data, returns its size.
  size_t CrossOver(const uint8_t *Data1, size_t Size1, const uint8_t *Data2,
                   size_t Size2, uint8_t *Out, size_t MaxOutSize);

  void AddWordToManualDictionary(const Word &W);

  void AddWordToAutoDictionary(const Word &W, size_t PositionHint);
  void ClearAutoDictionary();
  void PrintRecommendedDictionary();

  void SetCorpus(const std::vector<Unit> *Corpus) { this->Corpus = Corpus; }

  Random &GetRand() { return Rand; }

private:

  struct Mutator {
    size_t (MutationDispatcher::*Fn)(uint8_t *Data, size_t Size, size_t Max);
    const char *Name;
  };

  size_t AddWordFromDictionary(Dictionary &D, uint8_t *Data, size_t Size,
                               size_t MaxSize);

  Random &Rand;
  // Dictionary provided by the user via -dict=DICT_FILE.
  Dictionary ManualDictionary;
  // Temporary dictionary modified by the fuzzer itself,
  // recreated periodically.
  Dictionary TempAutoDictionary;
  // Persistent dictionary modified by the fuzzer, consists of
  // entries that led to successfull discoveries in the past mutations.
  Dictionary PersistentAutoDictionary;
  std::vector<Mutator> CurrentMutatorSequence;
  std::vector<DictionaryEntry *> CurrentDictionaryEntrySequence;
  const std::vector<Unit> *Corpus = nullptr;
  std::vector<uint8_t> MutateInPlaceHere;

  static Mutator Mutators[];
};

class Fuzzer {
public:
  struct FuzzingOptions {
    int Verbosity = 1;
    int MaxLen = 0;
    int UnitTimeoutSec = 300;
    int TimeoutExitCode = 77;
    int ErrorExitCode = 77;
    int MaxTotalTimeSec = 0;
    bool DoCrossOver = true;
    int MutateDepth = 5;
    bool UseCounters = false;
    bool UseIndirCalls = true;
    bool UseTraces = false;
    bool UseMemcmp = true;
    bool UseFullCoverageSet = false;
    bool Reload = true;
    bool ShuffleAtStartUp = true;
    int PreferSmallDuringInitialShuffle = -1;
    size_t MaxNumberOfRuns = ULONG_MAX;
    int SyncTimeout = 600;
    int ReportSlowUnits = 10;
    bool OnlyASCII = false;
    std::string OutputCorpus;
    std::string SyncCommand;
    std::string ArtifactPrefix = "./";
    std::string ExactArtifactPath;
    bool SaveArtifacts = true;
    bool PrintNEW = true; // Print a status line when new units are found;
    bool OutputCSV = false;
    bool PrintNewCovPcs = false;
    bool PrintFinalStats = false;
  };
  Fuzzer(UserCallback CB, MutationDispatcher &MD, FuzzingOptions Options);
  void AddToCorpus(const Unit &U) {
    Corpus.push_back(U);
    UpdateCorpusDistribution();
  }
  size_t ChooseUnitIdxToMutate();
  const Unit &ChooseUnitToMutate() { return Corpus[ChooseUnitIdxToMutate()]; };
  void Loop();
  void Drill();
  void ShuffleAndMinimize();
  void InitializeTraceState();
  void AssignTaintLabels(uint8_t *Data, size_t Size);
  size_t CorpusSize() const { return Corpus.size(); }
  void ReadDir(const std::string &Path, long *Epoch, size_t MaxSize) {
    Printf("Loading corpus: %s\n", Path.c_str());
    ReadDirToVectorOfUnits(Path.c_str(), &Corpus, Epoch, MaxSize);
  }
  void RereadOutputCorpus();
  // Save the current corpus to OutputCorpus.
  void SaveCorpus();

  size_t secondsSinceProcessStartUp() {
    return duration_cast<seconds>(system_clock::now() - ProcessStartTime)
        .count();
  }
  size_t execPerSec() {
    size_t Seconds = secondsSinceProcessStartUp();
    return Seconds ? TotalNumberOfRuns / Seconds : 0;
  }

  size_t getTotalNumberOfRuns() { return TotalNumberOfRuns; }

  static void StaticAlarmCallback();
  static void StaticCrashSignalCallback();
  static void StaticInterruptCallback();

  void ExecuteCallback(const uint8_t *Data, size_t Size);

  // Merge Corpora[1:] into Corpora[0].
  void Merge(const std::vector<std::string> &Corpora);
  MutationDispatcher &GetMD() { return MD; }
  void PrintFinalStats();

private:
  void AlarmCallback();
  void CrashCallback();
  void InterruptCallback();
  void MutateAndTestOne();
  void ReportNewCoverage(const Unit &U);
  bool RunOne(const uint8_t *Data, size_t Size);
  bool RunOne(const Unit &U) { return RunOne(U.data(), U.size()); }
  void RunOneAndUpdateCorpus(uint8_t *Data, size_t Size);
  void WriteToOutputCorpus(const Unit &U);
  void WriteUnitToFileWithPrefix(const Unit &U, const char *Prefix);
  void PrintStats(const char *Where, const char *End = "\n");
  void PrintStatusForNewUnit(const Unit &U);
  // Updates the probability distribution for the units in the corpus.
  // Must be called whenever the corpus or unit weights are changed.
  void UpdateCorpusDistribution();

  void SyncCorpus();

  size_t RecordBlockCoverage();
  size_t RecordCallerCalleeCoverage();
  void PrepareCoverageBeforeRun();
  bool CheckCoverageAfterRun();

  // Trace-based fuzzing: we run a unit with some kind of tracing
  // enabled and record potentially useful mutations. Then
  // We apply these mutations one by one to the unit and run it again.

  // Start tracing; forget all previously proposed mutations.
  void StartTraceRecording();
  // Stop tracing.
  void StopTraceRecording();

  void SetDeathCallback();
  static void StaticDeathCallback();
  void DumpCurrentUnit(const char *Prefix);
  void DeathCallback();

  uint8_t *CurrentUnitData;
  size_t CurrentUnitSize;

  size_t TotalNumberOfRuns = 0;
  size_t TotalNumberOfExecutedTraceBasedMutations = 0;
  size_t NumberOfNewUnitsAdded = 0;

  std::vector<Unit> Corpus;
  std::unordered_set<std::string> UnitHashesAddedToCorpus;

  // For UseCounters
  std::vector<uint8_t> CounterBitmap;
  size_t TotalBits() { // Slow. Call it only for printing stats.
    size_t Res = 0;
    for (auto x : CounterBitmap)
      Res += __builtin_popcount(x);
    return Res;
  }

  std::vector<uint8_t> MutateInPlaceHere;

  std::piecewise_constant_distribution<double> CorpusDistribution;
  UserCallback CB;
  MutationDispatcher &MD;
  FuzzingOptions Options;
  system_clock::time_point ProcessStartTime = system_clock::now();
  system_clock::time_point LastExternalSync = system_clock::now();
  system_clock::time_point UnitStartTime;
  long TimeOfLongestUnitInSeconds = 0;
  long EpochOfLastReadOfOutputCorpus = 0;
  size_t LastRecordedBlockCoverage = 0;
  size_t LastRecordedPcMapSize = 0;
  size_t LastRecordedCallerCalleeCoverage = 0;
  size_t LastCoveragePcBufferLen = 0;
};

}; // namespace fuzzer

#endif // LLVM_FUZZER_INTERNAL_H
