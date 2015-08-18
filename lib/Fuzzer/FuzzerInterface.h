//===- FuzzerInterface.h - Interface header for the Fuzzer ------*- C++ -* ===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Define the interface between the Fuzzer and the library being tested.
//===----------------------------------------------------------------------===//

// WARNING: keep the interface free of STL or any other header-based C++ lib,
// to avoid bad interactions between the code used in the fuzzer and
// the code used in the target function.

#ifndef LLVM_FUZZER_INTERFACE_H
#define LLVM_FUZZER_INTERFACE_H

#include <cstddef>
#include <cstdint>

namespace fuzzer {

typedef void (*UserCallback)(const uint8_t *Data, size_t Size);
/** Simple C-like interface with a single user-supplied callback.

Usage:

#\code
#include "FuzzerInterface.h"

void LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  DoStuffWithData(Data, Size);
}

// Implement your own main() or use the one from FuzzerMain.cpp.
int main(int argc, char **argv) {
  InitializeMeIfNeeded();
  return fuzzer::FuzzerDriver(argc, argv, LLVMFuzzerTestOneInput);
}
#\endcode
*/
int FuzzerDriver(int argc, char **argv, UserCallback Callback);

class FuzzerRandomBase {
 public:
  FuzzerRandomBase(){}
  virtual ~FuzzerRandomBase(){};
  virtual void ResetSeed(int seed) = 0;
  // Return a random number.
  virtual size_t Rand() = 0;
  // Return a random number in range [0,n).
  size_t operator()(size_t n) { return n ? Rand() % n : 0; }
  bool RandBool() { return Rand() % 2; }
};

class FuzzerRandomLibc : public FuzzerRandomBase {
 public:
  FuzzerRandomLibc(int seed) { ResetSeed(seed); }
  void ResetSeed(int seed) override;
  ~FuzzerRandomLibc() override {}
  size_t Rand() override;
};


/// Mutates data by shuffling bytes.
size_t Mutate_ShuffleBytes(uint8_t *Data, size_t Size, size_t MaxSize,
                           FuzzerRandomBase &Rand);
/// Mutates data by erasing a byte.
size_t Mutate_EraseByte(uint8_t *Data, size_t Size, size_t MaxSize,
                        FuzzerRandomBase &Rand);
/// Mutates data by inserting a byte.
size_t Mutate_InsertByte(uint8_t *Data, size_t Size, size_t MaxSize,
                         FuzzerRandomBase &Rand);
/// Mutates data by chanding one byte.
size_t Mutate_ChangeByte(uint8_t *Data, size_t Size, size_t MaxSize,
                         FuzzerRandomBase &Rand);
/// Mutates data by chanding one bit.
size_t Mutate_ChangeBit(uint8_t *Data, size_t Size, size_t MaxSize,
                       FuzzerRandomBase &Rand);

/// Applies one of the above mutations.
/// Returns the new size of data which could be up to MaxSize.
size_t Mutate(uint8_t *Data, size_t Size, size_t MaxSize,
              FuzzerRandomBase &Rand);

/// Creates a cross-over of two pieces of Data, returns its size.
size_t CrossOver(const uint8_t *Data1, size_t Size1, const uint8_t *Data2,
                 size_t Size2, uint8_t *Out, size_t MaxOutSize,
                 FuzzerRandomBase &Rand);


/** An abstract class that allows to use user-supplied mutators with libFuzzer.

Usage:

#\code
#include "FuzzerInterface.h"
class MyFuzzer : public fuzzer::UserSuppliedFuzzer {
 public:
  MyFuzzer(fuzzer::FuzzerRandomBase *Rand);
  // Must define the target function.
  void TargetFunction(...) { ... }
  // Optionally define the mutator.
  size_t Mutate(...) { ... }
  // Optionally define the CrossOver method.
  size_t CrossOver(...) { ... }
};

int main(int argc, char **argv) {
  MyFuzzer F;
  fuzzer::FuzzerDriver(argc, argv, F);
}
#\endcode
*/
class UserSuppliedFuzzer {
 public:
  UserSuppliedFuzzer();  // Deprecated, don't use.
  UserSuppliedFuzzer(FuzzerRandomBase *Rand);
  /// Executes the target function on 'Size' bytes of 'Data'.
  virtual void TargetFunction(const uint8_t *Data, size_t Size) = 0;
  /// Mutates 'Size' bytes of data in 'Data' inplace into up to 'MaxSize' bytes,
  /// returns the new size of the data, which should be positive.
  virtual size_t Mutate(uint8_t *Data, size_t Size, size_t MaxSize) {
    return ::fuzzer::Mutate(Data, Size, MaxSize, GetRand());
  }
  /// Crosses 'Data1' and 'Data2', writes up to 'MaxOutSize' bytes into Out,
  /// returns the number of bytes written, which should be positive.
  virtual size_t CrossOver(const uint8_t *Data1, size_t Size1,
                           const uint8_t *Data2, size_t Size2,
                           uint8_t *Out, size_t MaxOutSize) {
    return ::fuzzer::CrossOver(Data1, Size1, Data2, Size2, Out, MaxOutSize,
                               GetRand());
  }
  virtual ~UserSuppliedFuzzer();

  FuzzerRandomBase &GetRand() { return *Rand; }

 private:
  bool OwnRand = false;
  FuzzerRandomBase *Rand;
};

/// Runs the fuzzing with the UserSuppliedFuzzer.
int FuzzerDriver(int argc, char **argv, UserSuppliedFuzzer &USF);

}  // namespace fuzzer

#endif  // LLVM_FUZZER_INTERFACE_H
