//===- FuzzerTracePC.cpp - PC tracing--------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Trace PCs.
// This module implements __sanitizer_cov_trace_pc_guard[_init],
// the callback required for -fsanitize-coverage=trace-pc-guard instrumentation.
//
//===----------------------------------------------------------------------===//

#include "FuzzerCorpus.h"
#include "FuzzerDefs.h"
#include "FuzzerTracePC.h"
#include "FuzzerValueBitMap.h"

namespace fuzzer {

TracePC TPC;

void TracePC::HandleTrace(uint32_t *Guard, uintptr_t PC) {
  uint32_t Idx = *Guard;
  if (!Idx) return;
  uint8_t *CounterPtr = &Counters[Idx % kNumCounters];
  uint8_t Counter = *CounterPtr;
  if (Counter == 0) {
    if (!PCs[Idx]) {
      AddNewPCID(Idx);
      TotalPCCoverage++;
      PCs[Idx] = PC;
    }
  }
  if (UseCounters) {
    if (Counter < 128)
      *CounterPtr = Counter + 1;
    else
      *Guard = 0;
  } else {
    *CounterPtr = 1;
    *Guard = 0;
  }
}

void TracePC::HandleInit(uint32_t *Start, uint32_t *Stop) {
  if (Start == Stop || *Start) return;
  assert(NumModules < sizeof(Modules) / sizeof(Modules[0]));
  for (uint32_t *P = Start; P < Stop; P++)
    *P = ++NumGuards;
  Modules[NumModules].Start = Start;
  Modules[NumModules].Stop = Stop;
  NumModules++;
}

void TracePC::PrintModuleInfo() {
  Printf("INFO: Loaded %zd modules (%zd guards): ", NumModules, NumGuards);
  for (size_t i = 0; i < NumModules; i++)
    Printf("[%p, %p), ", Modules[i].Start, Modules[i].Stop);
  Printf("\n");
}

void TracePC::ResetGuards() {
  uint32_t N = 0;
  for (size_t M = 0; M < NumModules; M++)
    for (uint32_t *X = Modules[M].Start; X < Modules[M].Stop; X++)
      *X = ++N;
  assert(N == NumGuards);
}

size_t TracePC::FinalizeTrace(InputCorpus *C, size_t InputSize, bool Shrink) {
  if (!UsingTracePcGuard()) return 0;
  size_t Res = 0;
  const size_t Step = 8;
  assert(reinterpret_cast<uintptr_t>(Counters) % Step == 0);
  size_t N = Min(kNumCounters, NumGuards + 1);
  N = (N + Step - 1) & ~(Step - 1);  // Round up.
  for (size_t Idx = 0; Idx < N; Idx += Step) {
    uint64_t Bundle = *reinterpret_cast<uint64_t*>(&Counters[Idx]);
    if (!Bundle) continue;
    for (size_t i = Idx; i < Idx + Step; i++) {
      uint8_t Counter = (Bundle >> (i * 8)) & 0xff;
      if (!Counter) continue;
      Counters[i] = 0;
      unsigned Bit = 0;
      /**/ if (Counter >= 128) Bit = 7;
      else if (Counter >= 32) Bit = 6;
      else if (Counter >= 16) Bit = 5;
      else if (Counter >= 8) Bit = 4;
      else if (Counter >= 4) Bit = 3;
      else if (Counter >= 3) Bit = 2;
      else if (Counter >= 2) Bit = 1;
      size_t Feature = (i * 8 + Bit);
      if (C->AddFeature(Feature, InputSize, Shrink))
        Res++;
    }
  }
  if (UseValueProfile)
    ValueProfileMap.ForEach([&](size_t Idx) {
      if (C->AddFeature(NumGuards + Idx, InputSize, Shrink))
        Res++;
    });
  return Res;
}

void TracePC::HandleCallerCallee(uintptr_t Caller, uintptr_t Callee) {
  const uintptr_t kBits = 12;
  const uintptr_t kMask = (1 << kBits) - 1;
  uintptr_t Idx = (Caller & kMask) | ((Callee & kMask) << kBits);
  HandleValueProfile(Idx);
}

void TracePC::PrintCoverage() {
  Printf("COVERAGE:\n");
  for (size_t i = 0; i < Min(NumGuards + 1, kNumPCs); i++) {
    if (PCs[i])
      PrintPC("COVERED: %p %F %L\n", "COVERED: %p\n", PCs[i]);
  }
}

// Value profile.
// We keep track of various values that affect control flow.
// These values are inserted into a bit-set-based hash map.
// Every new bit in the map is treated as a new coverage.
//
// For memcmp/strcmp/etc the interesting value is the length of the common
// prefix of the parameters.
// For cmp instructions the interesting value is a XOR of the parameters.
// The interesting value is mixed up with the PC and is then added to the map.

void TracePC::AddValueForMemcmp(void *caller_pc, const void *s1, const void *s2,
                              size_t n) {
  if (!n) return;
  size_t Len = std::min(n, (size_t)32);
  const uint8_t *A1 = reinterpret_cast<const uint8_t *>(s1);
  const uint8_t *A2 = reinterpret_cast<const uint8_t *>(s2);
  size_t I = 0;
  for (; I < Len; I++)
    if (A1[I] != A2[I])
      break;
  size_t PC = reinterpret_cast<size_t>(caller_pc);
  size_t Idx = I;
  // if (I < Len)
  //  Idx += __builtin_popcountl((A1[I] ^ A2[I])) - 1;
  TPC.HandleValueProfile((PC & 4095) | (Idx << 12));
}

void TracePC::AddValueForStrcmp(void *caller_pc, const char *s1, const char *s2,
                              size_t n) {
  if (!n) return;
  size_t Len = std::min(n, (size_t)32);
  const uint8_t *A1 = reinterpret_cast<const uint8_t *>(s1);
  const uint8_t *A2 = reinterpret_cast<const uint8_t *>(s2);
  size_t I = 0;
  for (; I < Len; I++)
    if (A1[I] != A2[I] || A1[I] == 0)
      break;
  size_t PC = reinterpret_cast<size_t>(caller_pc);
  size_t Idx = I;
  // if (I < Len && A1[I])
  //  Idx += __builtin_popcountl((A1[I] ^ A2[I])) - 1;
  TPC.HandleValueProfile((PC & 4095) | (Idx << 12));
}

ATTRIBUTE_TARGET_POPCNT
static void AddValueForCmp(void *PCptr, uint64_t Arg1, uint64_t Arg2) {
  uintptr_t PC = reinterpret_cast<uintptr_t>(PCptr);
  uint64_t ArgDistance = __builtin_popcountl(Arg1 ^ Arg2) + 1; // [1,65]
  uintptr_t Idx = ((PC & 4095) + 1) * ArgDistance;
  TPC.HandleValueProfile(Idx);
}

static void AddValueForSingleVal(void *PCptr, uintptr_t Val) {
  if (!Val) return;
  uintptr_t PC = reinterpret_cast<uintptr_t>(PCptr);
  uint64_t ArgDistance = __builtin_popcountl(Val) - 1; // [0,63]
  uintptr_t Idx = (PC & 4095) | (ArgDistance << 12);
  TPC.HandleValueProfile(Idx);
}



} // namespace fuzzer

extern "C" {
__attribute__((visibility("default")))
void __sanitizer_cov_trace_pc_guard(uint32_t *Guard) {
  uintptr_t PC = (uintptr_t)__builtin_return_address(0);
  fuzzer::TPC.HandleTrace(Guard, PC);
}

__attribute__((visibility("default")))
void __sanitizer_cov_trace_pc_guard_init(uint32_t *Start, uint32_t *Stop) {
  fuzzer::TPC.HandleInit(Start, Stop);
}

__attribute__((visibility("default")))
void __sanitizer_cov_trace_pc_indir(uintptr_t Callee) {
  uintptr_t PC = (uintptr_t)__builtin_return_address(0);
  fuzzer::TPC.HandleCallerCallee(PC, Callee);
}

// TODO: this one will not be used with the newest clang. Remove it.
__attribute__((visibility("default")))
void __sanitizer_cov_trace_cmp(uint64_t SizeAndType, uint64_t Arg1,
                               uint64_t Arg2) {
  fuzzer::AddValueForCmp(__builtin_return_address(0), Arg1, Arg2);
}

__attribute__((visibility("default")))
void __sanitizer_cov_trace_cmp8(uint64_t Arg1, int64_t Arg2) {
  fuzzer::AddValueForCmp(__builtin_return_address(0), Arg1, Arg2);
}
__attribute__((visibility("default")))
void __sanitizer_cov_trace_cmp4(uint32_t Arg1, int32_t Arg2) {
  fuzzer::AddValueForCmp(__builtin_return_address(0), Arg1, Arg2);
}
__attribute__((visibility("default")))
void __sanitizer_cov_trace_cmp2(uint16_t Arg1, int16_t Arg2) {
  fuzzer::AddValueForCmp(__builtin_return_address(0), Arg1, Arg2);
}
__attribute__((visibility("default")))
void __sanitizer_cov_trace_cmp1(uint8_t Arg1, int8_t Arg2) {
  fuzzer::AddValueForCmp(__builtin_return_address(0), Arg1, Arg2);
}

__attribute__((visibility("default")))
void __sanitizer_cov_trace_switch(uint64_t Val, uint64_t *Cases) {
  // TODO(kcc): support value profile here.
}

__attribute__((visibility("default")))
void __sanitizer_cov_trace_div4(uint32_t Val) {
  fuzzer::AddValueForSingleVal(__builtin_return_address(0), Val);
}
__attribute__((visibility("default")))
void __sanitizer_cov_trace_div8(uint64_t Val) {
  fuzzer::AddValueForSingleVal(__builtin_return_address(0), Val);
}
__attribute__((visibility("default")))
void __sanitizer_cov_trace_gep(uintptr_t Idx) {
  fuzzer::AddValueForSingleVal(__builtin_return_address(0), Idx);
}

}  // extern "C"
