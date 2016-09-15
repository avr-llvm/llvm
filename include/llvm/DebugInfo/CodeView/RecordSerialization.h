//===- RecordSerialization.h ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_CODEVIEW_RECORDSERIALIZATION_H
#define LLVM_DEBUGINFO_CODEVIEW_RECORDSERIALIZATION_H

#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/DebugInfo/CodeView/CodeView.h"
#include "llvm/DebugInfo/CodeView/CodeViewError.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/Error.h"
#include <cinttypes>
#include <tuple>

namespace llvm {
namespace codeview {
using llvm::support::little32_t;
using llvm::support::ulittle16_t;
using llvm::support::ulittle32_t;

/// Limit on the size of all codeview symbol and type records, including the
/// RecordPrefix. MSVC does not emit any records larger than this.
enum : unsigned { MaxRecordLength = 0xFF00 };

struct RecordPrefix {
  ulittle16_t RecordLen;  // Record length, starting from &Leaf.
  ulittle16_t RecordKind; // Record kind enum (SymRecordKind or TypeRecordKind)
};

/// Reinterpret a byte array as an array of characters. Does not interpret as
/// a C string, as StringRef has several helpers (split) that make that easy.
StringRef getBytesAsCharacters(ArrayRef<uint8_t> LeafData);
StringRef getBytesAsCString(ArrayRef<uint8_t> LeafData);

/// Consumes sizeof(T) bytes from the given byte sequence. Returns an error if
/// there are not enough bytes remaining. Reinterprets the consumed bytes as a
/// T object and points 'Res' at them.
template <typename T, typename U>
inline Error consumeObject(U &Data, const T *&Res) {
  if (Data.size() < sizeof(*Res))
    return make_error<CodeViewError>(
        cv_error_code::insufficient_buffer,
        "Insufficient bytes for expected object type");
  Res = reinterpret_cast<const T *>(Data.data());
  Data = Data.drop_front(sizeof(*Res));
  return Error::success();
}

inline Error consume(ArrayRef<uint8_t> &Data) { return Error::success(); }

/// Decodes a numeric "leaf" value. These are integer literals encountered in
/// the type stream. If the value is positive and less than LF_NUMERIC (1 <<
/// 15), it is emitted directly in Data. Otherwise, it has a tag like LF_CHAR
/// that indicates the bitwidth and sign of the numeric data.
Error consume(ArrayRef<uint8_t> &Data, APSInt &Num);
Error consume(StringRef &Data, APSInt &Num);

/// Decodes a numeric leaf value that is known to be a particular type.
Error consume_numeric(ArrayRef<uint8_t> &Data, uint64_t &Value);

/// Decodes signed and unsigned fixed-length integers.
Error consume(ArrayRef<uint8_t> &Data, uint32_t &Item);
Error consume(StringRef &Data, uint32_t &Item);
Error consume(ArrayRef<uint8_t> &Data, int32_t &Item);

/// Decodes a null terminated string.
Error consume(ArrayRef<uint8_t> &Data, StringRef &Item);

/// Decodes an arbitrary object whose layout matches that of the underlying
/// byte sequence, and returns a pointer to the object.
template <typename T> Error consume(ArrayRef<uint8_t> &Data, T *&Item) {
  return consumeObject(Data, Item);
}

template <typename T, typename U> struct serialize_conditional_impl {
  serialize_conditional_impl(T &Item, U Func) : Item(Item), Func(Func) {}

  Error deserialize(ArrayRef<uint8_t> &Data) const {
    if (!Func())
      return Error::success();
    return consume(Data, Item);
  }

  T &Item;
  U Func;
};

template <typename T, typename U>
serialize_conditional_impl<T, U> serialize_conditional(T &Item, U Func) {
  return serialize_conditional_impl<T, U>(Item, Func);
}

template <typename T, typename U> struct serialize_array_impl {
  serialize_array_impl(ArrayRef<T> &Item, U Func) : Item(Item), Func(Func) {}

  Error deserialize(ArrayRef<uint8_t> &Data) const {
    uint32_t N = Func();
    if (N == 0)
      return Error::success();

    uint32_t Size = sizeof(T) * N;

    if (Size / sizeof(T) != N)
      return make_error<CodeViewError>(
          cv_error_code::corrupt_record,
          "Array<T> length is not a multiple of sizeof(T)");

    if (Data.size() < Size)
      return make_error<CodeViewError>(
          cv_error_code::corrupt_record,
          "Array<T> does not contain enough data for all elements");

    Item = ArrayRef<T>(reinterpret_cast<const T *>(Data.data()), N);
    Data = Data.drop_front(Size);
    return Error::success();
  }

  ArrayRef<T> &Item;
  U Func;
};

template <typename T> struct serialize_vector_tail_impl {
  serialize_vector_tail_impl(std::vector<T> &Item) : Item(Item) {}

  Error deserialize(ArrayRef<uint8_t> &Data) const {
    T Field;
    // Stop when we run out of bytes or we hit record padding bytes.
    while (!Data.empty() && Data.front() < LF_PAD0) {
      if (auto EC = consume(Data, Field))
        return EC;
      Item.push_back(Field);
    }
    return Error::success();
  }

  std::vector<T> &Item;
};

struct serialize_null_term_string_array_impl {
  serialize_null_term_string_array_impl(std::vector<StringRef> &Item)
      : Item(Item) {}

  Error deserialize(ArrayRef<uint8_t> &Data) const {
    if (Data.empty())
      return make_error<CodeViewError>(cv_error_code::insufficient_buffer,
                                       "Null terminated string is empty!");

    StringRef Field;
    // Stop when we run out of bytes or we hit record padding bytes.
    while (Data.front() != 0) {
      if (auto EC = consume(Data, Field))
        return EC;
      Item.push_back(Field);
      if (Data.empty())
        return make_error<CodeViewError>(
            cv_error_code::insufficient_buffer,
            "Null terminated string has no null terminator!");
    }
    Data = Data.drop_front(1);
    return Error::success();
  }

  std::vector<StringRef> &Item;
};

template <typename T> struct serialize_arrayref_tail_impl {
  serialize_arrayref_tail_impl(ArrayRef<T> &Item) : Item(Item) {}

  Error deserialize(ArrayRef<uint8_t> &Data) const {
    uint32_t Count = Data.size() / sizeof(T);
    Item = ArrayRef<T>(reinterpret_cast<const T *>(Data.begin()), Count);
    return Error::success();
  }

  ArrayRef<T> &Item;
};

template <typename T> struct serialize_numeric_impl {
  serialize_numeric_impl(T &Item) : Item(Item) {}

  Error deserialize(ArrayRef<uint8_t> &Data) const {
    return consume_numeric(Data, Item);
  }

  T &Item;
};

template <typename T, typename U>
serialize_array_impl<T, U> serialize_array(ArrayRef<T> &Item, U Func) {
  return serialize_array_impl<T, U>(Item, Func);
}

inline serialize_null_term_string_array_impl
serialize_null_term_string_array(std::vector<StringRef> &Item) {
  return serialize_null_term_string_array_impl(Item);
}

template <typename T>
serialize_vector_tail_impl<T> serialize_array_tail(std::vector<T> &Item) {
  return serialize_vector_tail_impl<T>(Item);
}

template <typename T>
serialize_arrayref_tail_impl<T> serialize_array_tail(ArrayRef<T> &Item) {
  return serialize_arrayref_tail_impl<T>(Item);
}

template <typename T> serialize_numeric_impl<T> serialize_numeric(T &Item) {
  return serialize_numeric_impl<T>(Item);
}

// This field is only present in the byte record if the condition is true.  The
// condition is evaluated lazily, so it can depend on items that were
// deserialized
// earlier.
#define CV_CONDITIONAL_FIELD(I, C)                                             \
  serialize_conditional(I, [&]() { return !!(C); })

// This is an array of N items, where N is evaluated lazily, so it can refer
// to a field deserialized earlier.
#define CV_ARRAY_FIELD_N(I, N) serialize_array(I, [&]() { return N; })

// This is an array that exhausts the remainder of the input buffer.
#define CV_ARRAY_FIELD_TAIL(I) serialize_array_tail(I)

// This is an array that consumes null terminated strings until a double null
// is encountered.
#define CV_STRING_ARRAY_NULL_TERM(I) serialize_null_term_string_array(I)

#define CV_NUMERIC_FIELD(I) serialize_numeric(I)

template <typename T, typename U>
Error consume(ArrayRef<uint8_t> &Data,
              const serialize_conditional_impl<T, U> &Item) {
  return Item.deserialize(Data);
}

template <typename T, typename U>
Error consume(ArrayRef<uint8_t> &Data, const serialize_array_impl<T, U> &Item) {
  return Item.deserialize(Data);
}

inline Error consume(ArrayRef<uint8_t> &Data,
                     const serialize_null_term_string_array_impl &Item) {
  return Item.deserialize(Data);
}

template <typename T>
Error consume(ArrayRef<uint8_t> &Data,
              const serialize_vector_tail_impl<T> &Item) {
  return Item.deserialize(Data);
}

template <typename T>
Error consume(ArrayRef<uint8_t> &Data,
              const serialize_arrayref_tail_impl<T> &Item) {
  return Item.deserialize(Data);
}

template <typename T>
Error consume(ArrayRef<uint8_t> &Data, const serialize_numeric_impl<T> &Item) {
  return Item.deserialize(Data);
}

template <typename T, typename U, typename... Args>
Error consume(ArrayRef<uint8_t> &Data, T &&X, U &&Y, Args &&... Rest) {
  if (auto EC = consume(Data, X))
    return EC;
  return consume(Data, Y, std::forward<Args>(Rest)...);
}

#define CV_DESERIALIZE(...)                                                    \
  if (auto EC = consume(__VA_ARGS__))                                          \
    return std::move(EC);
}
}

#endif
