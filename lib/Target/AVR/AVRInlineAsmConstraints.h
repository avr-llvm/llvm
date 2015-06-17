//===-- AVRConstraints - Inline Assembly Constraints ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_AVR_INLINE_ASM_CONSTRAINTS_H
# define LLVM_AVR_INLINE_ASM_CONSTRAINTS_H

# include "llvm/CodeGen/SelectionDAG.h"

namespace llvm { namespace AVR { namespace Constraints {

struct Integer {};
struct Float   {};

template <typename T> struct SelectConstantType;
template <> struct SelectConstantType<Integer> { typedef ConstantInt Type; };
template <> struct SelectConstantType<Float>   { typedef ConstantFP Type; };

template <typename T> struct SelectNodeType;
template <> struct SelectNodeType<Integer> { typedef ConstantSDNode Type; };
template <> struct SelectNodeType<Float>   { typedef ConstantFPSDNode Type; };

template <char Char, typename ConstType, typename This>
struct ConstantConstraint {
  typedef typename SelectConstantType<ConstType>::Type ConstantType;
  typedef typename SelectNodeType<ConstType>::Type     NodeType;
  static const char Code = Char;

  static bool check(Value const* Constant) {
    if (ConstantType const* C = dyn_cast<ConstantType>(Constant))
      return This::checkValue(C);
    return false;
  }
  static void getConstant(SDValue Op, SelectionDAG & DAG, SDValue & Result) {
    NodeType const* C = dyn_cast<NodeType>(Op);
    if (not C or not This::checkValue(C))
      return;

    Result = DAG.getTargetConstant(This::get(C), SDLoc(Op), This::getValueType(Op));
  }
  static EVT getValueType(SDValue const& Op) { return Op.getValueType(); }
};

struct G : ConstantConstraint<'G', Float, G> {
  template <typename T> static bool
  checkValue(T const* C) { return C->isZero(); }
  template <typename T> static uint64_t get(T const* C) { return 0; }
  // Soften float to i8 0
  static EVT getValueType(SDValue const& Op) { return MVT::i8; }
};

struct I : ConstantConstraint<'I', Integer, I> {
  template <typename T> static bool
  checkValue(T const* C) { return isUInt<6>(get(C)); }
  template <typename T> static uint64_t get(T const* C) { return C->getZExtValue(); }
};

struct J : ConstantConstraint<'J', Integer, J> {
  template <typename T> static bool
  checkValue(T const* C) {
    return (get(C) >= -63) && (get(C) <= 0);
  }
  template <typename T> static int64_t get(T const* C) { return C->getSExtValue(); }
};

struct K : ConstantConstraint<'K', Integer, K> {
  template <typename T> static bool
  checkValue(T const* C) { return get(C) == 2; }
  template <typename T> static uint64_t get(T const* C) { return C->getZExtValue(); }
};

struct L : ConstantConstraint<'L', Integer, L> {
  template <typename T> static bool
  checkValue(T const* C) { return get(C) == 0; }
  template <typename T> static uint64_t get(T const* C) { return C->getZExtValue(); }
};

struct M : ConstantConstraint<'M', Integer, M> {
  template <typename T> static bool
  checkValue(T const* C) { return isUInt<8>(get(C)); }
  template <typename T> static uint64_t get(T const* C) { return C->getZExtValue(); }
  static EVT getValueType(SDValue const& Op) {
    return Op.getValueType() == MVT::i8 ? MVT::i16 : Op.getValueType();
  }
};

struct N : ConstantConstraint<'N', Integer, N> {
  template <typename T> static bool
  checkValue(T const* C) { return get(C) == -1; }
  template <typename T> static int64_t get(T const* C) { return C->getSExtValue(); }
};

struct O : ConstantConstraint<'O', Integer, O> {
  template <typename T> static bool
  checkValue(T const* C) { return (get(C) ==  8) || (get(C) == 16) || (get(C) == 24); }
  template <typename T> static uint64_t get(T const* C) { return C->getZExtValue(); }
};

struct P : ConstantConstraint<'P', Integer, P> {
  template <typename T> static bool
  checkValue(T const* C) { return C->getZExtValue() == 1; }
  template <typename T> static uint64_t get(T const* C) { return C->getZExtValue(); }
};

struct R : ConstantConstraint<'R', Integer, R> {
  template <typename T> static bool
  checkValue(T const* C) { return (get(C) >= -6) && (get(C) <= 5); }
  template <typename T> static int64_t get(T const* C) { return C->getSExtValue(); }
};
  
}}} // end of namespace llvm::AVR::Constraints

#endif // LLVM_AVR_INLINE_ASM_CONSTRAINTS_H

