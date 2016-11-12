//===- FormatVariadicDetails.h - Helpers for FormatVariadic.h ----*- C++-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_FORMATVARIADIC_DETAILS_H
#define LLVM_SUPPORT_FORMATVARIADIC_DETAILS_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <type_traits>

namespace llvm {
template <typename T, typename Enable = void> struct format_provider {};

namespace detail {

class format_wrapper {
protected:
  ~format_wrapper() {}

public:
  virtual void format(llvm::raw_ostream &S, StringRef Options) = 0;
};

template <typename T> class member_format_wrapper : public format_wrapper {
  T Item;

public:
  explicit member_format_wrapper(T &&Item) : Item(Item) {}

  void format(llvm::raw_ostream &S, StringRef Options) override {
    Item.format(S, Options);
  }
};

template <typename T> class provider_format_wrapper : public format_wrapper {
  T Item;

public:
  explicit provider_format_wrapper(T &&Item) : Item(Item) {}

  void format(llvm::raw_ostream &S, StringRef Options) override {
    format_provider<typename std::decay<T>::type>::format(Item, S, Options);
  }
};

template <typename T> class missing_format_wrapper;

// Test if T is a class that contains a member function with the signature:
//
// void format(raw_ostream &, StringRef);
//
template <class T, class Enable = void> class has_FormatMember {
public:
  static bool const value = false;
};

template <class T>
class has_FormatMember<T,
                       typename std::enable_if<std::is_class<T>::value>::type> {
  using Signature_format = void (T::*)(llvm::raw_ostream &S, StringRef Options);

  template <typename U>
  static char test2(SameType<Signature_format, &U::format> *);

  template <typename U> static double test2(...);

public:
  static bool const value = (sizeof(test2<T>(nullptr)) == 1);
};

// Test if format_provider<T> is defined on T and contains a member function
// with the signature:
//   static void format(const T&, raw_stream &, StringRef);
//
template <class T> class has_FormatProvider {
public:
  using Decayed = typename std::decay<T>::type;
  typedef void (*Signature_format)(const Decayed &, llvm::raw_ostream &,
                                   StringRef);

  template <typename U>
  static char test(SameType<Signature_format, &U::format> *);

  template <typename U> static double test(...);

  static bool const value =
      (sizeof(test<llvm::format_provider<Decayed>>(nullptr)) == 1);
};

// Simple template that decides whether a type T should use the member-function
// based format() invocation.
template <typename T>
struct uses_format_member
    : public std::integral_constant<bool, has_FormatMember<T>::value> {};

// Simple template that decides whether a type T should use the format_provider
// based format() invocation.  The member function takes priority, so this test
// will only be true if there is not ALSO a format member.
template <typename T>
struct uses_format_provider
    : public std::integral_constant<bool, !has_FormatMember<T>::value &&
                                              has_FormatProvider<T>::value> {};

// Simple template that decides whether a type T has neither a member-function
// nor format_provider based implementation that it can use.  Mostly used so
// that the compiler spits out a nice diagnostic when a type with no format
// implementation can be located.
template <typename T>
struct uses_missing_provider
    : public std::integral_constant<bool, !has_FormatMember<T>::value &&
                                              !has_FormatProvider<T>::value> {};

template <typename T>
typename std::enable_if<uses_format_member<T>::value,
                        member_format_wrapper<T>>::type
build_format_wrapper(T &&Item) {
  return member_format_wrapper<T>(std::forward<T>(Item));
}

template <typename T>
typename std::enable_if<uses_format_provider<T>::value,
                        provider_format_wrapper<T>>::type
build_format_wrapper(T &&Item) {
  return provider_format_wrapper<T>(std::forward<T>(Item));
}

template <typename T>
typename std::enable_if<uses_missing_provider<T>::value,
                        missing_format_wrapper<T>>::type
build_format_wrapper(T &&Item) {
  return missing_format_wrapper<T>();
}
}
}

#endif
