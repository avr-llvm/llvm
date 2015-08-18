#include "FuzzerInternal.h"
#include "gtest/gtest.h"
#include <set>

using namespace fuzzer;

// For now, have LLVMFuzzerTestOneInput just to make it link.
// Later we may want to make unittests that actually call LLVMFuzzerTestOneInput.
extern "C" void LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  abort();
}

TEST(Fuzzer, CrossOver) {
  FuzzerRandomLibc Rand(0);
  Unit A({0, 1, 2}), B({5, 6, 7});
  Unit C;
  Unit Expected[] = {
       { 0 },
       { 0, 1 },
       { 0, 5 },
       { 0, 1, 2 },
       { 0, 1, 5 },
       { 0, 5, 1 },
       { 0, 5, 6 },
       { 0, 1, 2, 5 },
       { 0, 1, 5, 2 },
       { 0, 1, 5, 6 },
       { 0, 5, 1, 2 },
       { 0, 5, 1, 6 },
       { 0, 5, 6, 1 },
       { 0, 5, 6, 7 },
       { 0, 1, 2, 5, 6 },
       { 0, 1, 5, 2, 6 },
       { 0, 1, 5, 6, 2 },
       { 0, 1, 5, 6, 7 },
       { 0, 5, 1, 2, 6 },
       { 0, 5, 1, 6, 2 },
       { 0, 5, 1, 6, 7 },
       { 0, 5, 6, 1, 2 },
       { 0, 5, 6, 1, 7 },
       { 0, 5, 6, 7, 1 },
       { 0, 1, 2, 5, 6, 7 },
       { 0, 1, 5, 2, 6, 7 },
       { 0, 1, 5, 6, 2, 7 },
       { 0, 1, 5, 6, 7, 2 },
       { 0, 5, 1, 2, 6, 7 },
       { 0, 5, 1, 6, 2, 7 },
       { 0, 5, 1, 6, 7, 2 },
       { 0, 5, 6, 1, 2, 7 },
       { 0, 5, 6, 1, 7, 2 },
       { 0, 5, 6, 7, 1, 2 }
  };
  for (size_t Len = 1; Len < 8; Len++) {
    std::set<Unit> FoundUnits, ExpectedUnitsWitThisLength;
    for (int Iter = 0; Iter < 3000; Iter++) {
      C.resize(Len);
      size_t NewSize = CrossOver(A.data(), A.size(), B.data(), B.size(),
                                 C.data(), C.size(), Rand);
      C.resize(NewSize);
      FoundUnits.insert(C);
    }
    for (const Unit &U : Expected)
      if (U.size() <= Len)
        ExpectedUnitsWitThisLength.insert(U);
    EXPECT_EQ(ExpectedUnitsWitThisLength, FoundUnits);
  }
}

TEST(Fuzzer, Hash) {
  uint8_t A[] = {'a', 'b', 'c'};
  fuzzer::Unit U(A, A + sizeof(A));
  EXPECT_EQ("a9993e364706816aba3e25717850c26c9cd0d89d", fuzzer::Hash(U));
  U.push_back('d');
  EXPECT_EQ("81fe8bfe87576c3ecb22426f8e57847382917acf", fuzzer::Hash(U));
}

typedef size_t (*Mutator)(uint8_t *Data, size_t Size, size_t MaxSize,
                          FuzzerRandomBase &Rand);

void TestEraseByte(Mutator M, int NumIter) {
  uint8_t REM0[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t REM1[8] = {0x00, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t REM2[8] = {0x00, 0x11, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t REM3[8] = {0x00, 0x11, 0x22, 0x44, 0x55, 0x66, 0x77};
  uint8_t REM4[8] = {0x00, 0x11, 0x22, 0x33, 0x55, 0x66, 0x77};
  uint8_t REM5[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x66, 0x77};
  uint8_t REM6[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x77};
  uint8_t REM7[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  FuzzerRandomLibc Rand(0);
  int FoundMask = 0;
  for (int i = 0; i < NumIter; i++) {
    uint8_t T[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    size_t NewSize = M(T, sizeof(T), sizeof(T), Rand);
    if (NewSize == 7 && !memcmp(REM0, T, 7)) FoundMask |= 1 << 0;
    if (NewSize == 7 && !memcmp(REM1, T, 7)) FoundMask |= 1 << 1;
    if (NewSize == 7 && !memcmp(REM2, T, 7)) FoundMask |= 1 << 2;
    if (NewSize == 7 && !memcmp(REM3, T, 7)) FoundMask |= 1 << 3;
    if (NewSize == 7 && !memcmp(REM4, T, 7)) FoundMask |= 1 << 4;
    if (NewSize == 7 && !memcmp(REM5, T, 7)) FoundMask |= 1 << 5;
    if (NewSize == 7 && !memcmp(REM6, T, 7)) FoundMask |= 1 << 6;
    if (NewSize == 7 && !memcmp(REM7, T, 7)) FoundMask |= 1 << 7;
  }
  EXPECT_EQ(FoundMask, 255);
}

TEST(FuzzerMutate, EraseByte1) { TestEraseByte(Mutate_EraseByte, 100); }
TEST(FuzzerMutate, EraseByte2) { TestEraseByte(Mutate, 1000); }

void TestInsertByte(Mutator M, int NumIter) {
  FuzzerRandomLibc Rand(0);
  int FoundMask = 0;
  uint8_t INS0[8] = {0xF1, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  uint8_t INS1[8] = {0x00, 0xF2, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  uint8_t INS2[8] = {0x00, 0x11, 0xF3, 0x22, 0x33, 0x44, 0x55, 0x66};
  uint8_t INS3[8] = {0x00, 0x11, 0x22, 0xF4, 0x33, 0x44, 0x55, 0x66};
  uint8_t INS4[8] = {0x00, 0x11, 0x22, 0x33, 0xF5, 0x44, 0x55, 0x66};
  uint8_t INS5[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0xF6, 0x55, 0x66};
  uint8_t INS6[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0xF7, 0x66};
  uint8_t INS7[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xF8};
  for (int i = 0; i < NumIter; i++) {
    uint8_t T[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    size_t NewSize = M(T, 7, 8, Rand);
    if (NewSize == 8 && !memcmp(INS0, T, 8)) FoundMask |= 1 << 0;
    if (NewSize == 8 && !memcmp(INS1, T, 8)) FoundMask |= 1 << 1;
    if (NewSize == 8 && !memcmp(INS2, T, 8)) FoundMask |= 1 << 2;
    if (NewSize == 8 && !memcmp(INS3, T, 8)) FoundMask |= 1 << 3;
    if (NewSize == 8 && !memcmp(INS4, T, 8)) FoundMask |= 1 << 4;
    if (NewSize == 8 && !memcmp(INS5, T, 8)) FoundMask |= 1 << 5;
    if (NewSize == 8 && !memcmp(INS6, T, 8)) FoundMask |= 1 << 6;
    if (NewSize == 8 && !memcmp(INS7, T, 8)) FoundMask |= 1 << 7;
  }
  EXPECT_EQ(FoundMask, 255);
}

TEST(FuzzerMutate, InsertByte1) { TestInsertByte(Mutate_InsertByte, 1 << 15); }
TEST(FuzzerMutate, InsertByte2) { TestInsertByte(Mutate, 1 << 17); }

void TestChangeByte(Mutator M, int NumIter) {
  FuzzerRandomLibc Rand(0);
  int FoundMask = 0;
  uint8_t CH0[8] = {0xF0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t CH1[8] = {0x00, 0xF1, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t CH2[8] = {0x00, 0x11, 0xF2, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t CH3[8] = {0x00, 0x11, 0x22, 0xF3, 0x44, 0x55, 0x66, 0x77};
  uint8_t CH4[8] = {0x00, 0x11, 0x22, 0x33, 0xF4, 0x55, 0x66, 0x77};
  uint8_t CH5[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0xF5, 0x66, 0x77};
  uint8_t CH6[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0xF5, 0x77};
  uint8_t CH7[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xF7};
  for (int i = 0; i < NumIter; i++) {
    uint8_t T[9] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    size_t NewSize = M(T, 8, 9, Rand);
    if (NewSize == 8 && !memcmp(CH0, T, 8)) FoundMask |= 1 << 0;
    if (NewSize == 8 && !memcmp(CH1, T, 8)) FoundMask |= 1 << 1;
    if (NewSize == 8 && !memcmp(CH2, T, 8)) FoundMask |= 1 << 2;
    if (NewSize == 8 && !memcmp(CH3, T, 8)) FoundMask |= 1 << 3;
    if (NewSize == 8 && !memcmp(CH4, T, 8)) FoundMask |= 1 << 4;
    if (NewSize == 8 && !memcmp(CH5, T, 8)) FoundMask |= 1 << 5;
    if (NewSize == 8 && !memcmp(CH6, T, 8)) FoundMask |= 1 << 6;
    if (NewSize == 8 && !memcmp(CH7, T, 8)) FoundMask |= 1 << 7;
  }
  EXPECT_EQ(FoundMask, 255);
}

TEST(FuzzerMutate, ChangeByte1) { TestChangeByte(Mutate_ChangeByte, 1 << 15); }
TEST(FuzzerMutate, ChangeByte2) { TestChangeByte(Mutate, 1 << 17); }

void TestChangeBit(Mutator M, int NumIter) {
  FuzzerRandomLibc Rand(0);
  int FoundMask = 0;
  uint8_t CH0[8] = {0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t CH1[8] = {0x00, 0x13, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t CH2[8] = {0x00, 0x11, 0x02, 0x33, 0x44, 0x55, 0x66, 0x77};
  uint8_t CH3[8] = {0x00, 0x11, 0x22, 0x37, 0x44, 0x55, 0x66, 0x77};
  uint8_t CH4[8] = {0x00, 0x11, 0x22, 0x33, 0x54, 0x55, 0x66, 0x77};
  uint8_t CH5[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x54, 0x66, 0x77};
  uint8_t CH6[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x76, 0x77};
  uint8_t CH7[8] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0xF7};
  for (int i = 0; i < NumIter; i++) {
    uint8_t T[9] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
    size_t NewSize = M(T, 8, 9, Rand);
    if (NewSize == 8 && !memcmp(CH0, T, 8)) FoundMask |= 1 << 0;
    if (NewSize == 8 && !memcmp(CH1, T, 8)) FoundMask |= 1 << 1;
    if (NewSize == 8 && !memcmp(CH2, T, 8)) FoundMask |= 1 << 2;
    if (NewSize == 8 && !memcmp(CH3, T, 8)) FoundMask |= 1 << 3;
    if (NewSize == 8 && !memcmp(CH4, T, 8)) FoundMask |= 1 << 4;
    if (NewSize == 8 && !memcmp(CH5, T, 8)) FoundMask |= 1 << 5;
    if (NewSize == 8 && !memcmp(CH6, T, 8)) FoundMask |= 1 << 6;
    if (NewSize == 8 && !memcmp(CH7, T, 8)) FoundMask |= 1 << 7;
  }
  EXPECT_EQ(FoundMask, 255);
}

TEST(FuzzerMutate, ChangeBit1) { TestChangeBit(Mutate_ChangeBit, 1 << 16); }
TEST(FuzzerMutate, ChangeBit2) { TestChangeBit(Mutate, 1 << 18); }

void TestShuffleBytes(Mutator M, int NumIter) {
  FuzzerRandomLibc Rand(0);
  int FoundMask = 0;
  uint8_t CH0[7] = {0x00, 0x22, 0x11, 0x33, 0x44, 0x55, 0x66};
  uint8_t CH1[7] = {0x11, 0x00, 0x33, 0x22, 0x44, 0x55, 0x66};
  uint8_t CH2[7] = {0x00, 0x33, 0x11, 0x22, 0x44, 0x55, 0x66};
  uint8_t CH3[7] = {0x00, 0x11, 0x22, 0x44, 0x55, 0x66, 0x33};
  uint8_t CH4[7] = {0x00, 0x11, 0x22, 0x33, 0x55, 0x44, 0x66};
  for (int i = 0; i < NumIter; i++) {
    uint8_t T[7] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    size_t NewSize = M(T, 7, 7, Rand);
    if (NewSize == 7 && !memcmp(CH0, T, 7)) FoundMask |= 1 << 0;
    if (NewSize == 7 && !memcmp(CH1, T, 7)) FoundMask |= 1 << 1;
    if (NewSize == 7 && !memcmp(CH2, T, 7)) FoundMask |= 1 << 2;
    if (NewSize == 7 && !memcmp(CH3, T, 7)) FoundMask |= 1 << 3;
    if (NewSize == 7 && !memcmp(CH4, T, 7)) FoundMask |= 1 << 4;
  }
  EXPECT_EQ(FoundMask, 31);
}

TEST(FuzzerMutate, ShuffleBytes1) { TestShuffleBytes(Mutate_ShuffleBytes, 1 << 15); }
TEST(FuzzerMutate, ShuffleBytes2) { TestShuffleBytes(Mutate, 1 << 16); }
