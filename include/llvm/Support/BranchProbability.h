//===- BranchProbability.h - Branch Probability Wrapper ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Definition of BranchProbability shared by IR and Machine Instructions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_BRANCHPROBABILITY_H
#define LLVM_SUPPORT_BRANCHPROBABILITY_H

#include "llvm/Support/DataTypes.h"
#include <algorithm>
#include <cassert>
#include <climits>
#include <numeric>

namespace llvm {

class raw_ostream;

// This class represents Branch Probability as a non-negative fraction that is
// no greater than 1. It uses a fixed-point-like implementation, in which the
// denominator is always a constant value (here we use 1<<31 for maximum
// precision).
class BranchProbability {
  // Numerator
  uint32_t N;

  // Denominator, which is a constant value.
  static const uint32_t D = 1u << 31;

  // Construct a BranchProbability with only numerator assuming the denominator
  // is 1<<31. For internal use only.
  explicit BranchProbability(uint32_t n) : N(n) {}

public:
  BranchProbability() : N(0) {}
  BranchProbability(uint32_t Numerator, uint32_t Denominator);

  bool isZero() const { return N == 0; }

  static BranchProbability getZero() { return BranchProbability(0); }
  static BranchProbability getOne() { return BranchProbability(D); }
  // Create a BranchProbability object with the given numerator and 1<<31
  // as denominator.
  static BranchProbability getRaw(uint32_t N) { return BranchProbability(N); }

  // Normalize given probabilties so that the sum of them becomes approximate
  // one.
  template <class ProbabilityList>
  static void normalizeProbabilities(ProbabilityList &Probs);

  // Normalize a list of weights by scaling them down so that the sum of them
  // doesn't exceed UINT32_MAX.
  template <class WeightListIter>
  static void normalizeEdgeWeights(WeightListIter Begin, WeightListIter End);

  uint32_t getNumerator() const { return N; }
  static uint32_t getDenominator() { return D; }

  // Return (1 - Probability).
  BranchProbability getCompl() const { return BranchProbability(D - N); }

  raw_ostream &print(raw_ostream &OS) const;

  void dump() const;

  /// \brief Scale a large integer.
  ///
  /// Scales \c Num.  Guarantees full precision.  Returns the floor of the
  /// result.
  ///
  /// \return \c Num times \c this.
  uint64_t scale(uint64_t Num) const;

  /// \brief Scale a large integer by the inverse.
  ///
  /// Scales \c Num by the inverse of \c this.  Guarantees full precision.
  /// Returns the floor of the result.
  ///
  /// \return \c Num divided by \c this.
  uint64_t scaleByInverse(uint64_t Num) const;

  BranchProbability &operator+=(BranchProbability RHS) {
    assert(N <= D - RHS.N &&
           "The sum of branch probabilities should not exceed one!");
    N += RHS.N;
    return *this;
  }

  BranchProbability &operator-=(BranchProbability RHS) {
    assert(N >= RHS.N &&
           "Can only subtract a smaller probability from a larger one!");
    N -= RHS.N;
    return *this;
  }

  BranchProbability &operator*=(BranchProbability RHS) {
    N = (static_cast<uint64_t>(N) * RHS.N + D / 2) / D;
    return *this;
  }

  BranchProbability operator+(BranchProbability RHS) const {
    BranchProbability Prob(*this);
    return Prob += RHS;
  }

  BranchProbability operator-(BranchProbability RHS) const {
    BranchProbability Prob(*this);
    return Prob -= RHS;
  }

  BranchProbability operator*(BranchProbability RHS) const {
    BranchProbability Prob(*this);
    return Prob *= RHS;
  }

  bool operator==(BranchProbability RHS) const { return N == RHS.N; }
  bool operator!=(BranchProbability RHS) const { return !(*this == RHS); }
  bool operator<(BranchProbability RHS) const { return N < RHS.N; }
  bool operator>(BranchProbability RHS) const { return RHS < *this; }
  bool operator<=(BranchProbability RHS) const { return !(RHS < *this); }
  bool operator>=(BranchProbability RHS) const { return !(*this < RHS); }
};

inline raw_ostream &operator<<(raw_ostream &OS, BranchProbability Prob) {
  return Prob.print(OS);
}

template <class ProbabilityList>
void BranchProbability::normalizeProbabilities(ProbabilityList &Probs) {
  uint64_t Sum = 0;
  for (auto Prob : Probs)
    Sum += Prob.N;
  assert(Sum > 0);
  for (auto &Prob : Probs)
    Prob.N = (Prob.N * uint64_t(D) + Sum / 2) / Sum;
}

template <class WeightListIter>
void BranchProbability::normalizeEdgeWeights(WeightListIter Begin,
                                             WeightListIter End) {
  // First we compute the sum with 64-bits of precision.
  uint64_t Sum = std::accumulate(Begin, End, uint64_t(0));

  if (Sum > UINT32_MAX) {
    // Compute the scale necessary to cause the weights to fit, and re-sum with
    // that scale applied.
    assert(Sum / UINT32_MAX < UINT32_MAX &&
           "The sum of weights exceeds UINT32_MAX^2!");
    uint32_t Scale = Sum / UINT32_MAX + 1;
    for (auto I = Begin; I != End; ++I)
      *I /= Scale;
    Sum = std::accumulate(Begin, End, uint64_t(0));
  }

  // Eliminate zero weights.
  auto ZeroWeightNum = std::count(Begin, End, 0u);
  if (ZeroWeightNum > 0) {
    // If all weights are zeros, replace them by 1.
    if (Sum == 0)
      std::fill(Begin, End, 1u);
    else {
      // We are converting zeros into ones, and here we need to make sure that
      // after this the sum won't exceed UINT32_MAX.
      if (Sum + ZeroWeightNum > UINT32_MAX) {
        for (auto I = Begin; I != End; ++I)
          *I /= 2;
        ZeroWeightNum = std::count(Begin, End, 0u);
        Sum = std::accumulate(Begin, End, uint64_t(0));
      }
      // Scale up non-zero weights and turn zero weights into ones.
      uint64_t ScalingFactor = (UINT32_MAX - ZeroWeightNum) / Sum;
      assert(ScalingFactor >= 1);
      if (ScalingFactor > 1)
        for (auto I = Begin; I != End; ++I)
          *I *= ScalingFactor;
      std::replace(Begin, End, 0u, 1u);
    }
  }
}

}

#endif
