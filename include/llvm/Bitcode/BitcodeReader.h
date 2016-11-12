//===-- llvm/Bitcode/BitcodeReader.h - Bitcode reader ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header defines interfaces to read LLVM bitcode files/streams.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_BITCODE_BITCODEREADER_H
#define LLVM_BITCODE_BITCODEREADER_H

#include "llvm/Bitcode/BitCodes.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/ModuleSummaryIndex.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"
#include <memory>

namespace llvm {
  class LLVMContext;
  class Module;

  // These functions are for converting Expected/Error values to
  // ErrorOr/std::error_code for compatibility with legacy clients. FIXME:
  // Remove these functions once no longer needed by the C and libLTO APIs.

  std::error_code errorToErrorCodeAndEmitErrors(LLVMContext &Ctx, Error Err);

  template <typename T>
  ErrorOr<T> expectedToErrorOrAndEmitErrors(LLVMContext &Ctx, Expected<T> Val) {
    if (!Val)
      return errorToErrorCodeAndEmitErrors(Ctx, Val.takeError());
    return std::move(*Val);
  }

  /// Read the header of the specified bitcode buffer and prepare for lazy
  /// deserialization of function bodies. If ShouldLazyLoadMetadata is true,
  /// lazily load metadata as well.
  ErrorOr<std::unique_ptr<Module>>
  getLazyBitcodeModule(MemoryBufferRef Buffer, LLVMContext &Context,
                       bool ShouldLazyLoadMetadata = false);

  /// Like getLazyBitcodeModule, except that the module takes ownership of
  /// the memory buffer if successful. If successful, this moves Buffer. On
  /// error, this *does not* move Buffer.
  ErrorOr<std::unique_ptr<Module>>
  getOwningLazyBitcodeModule(std::unique_ptr<MemoryBuffer> &&Buffer,
                             LLVMContext &Context,
                             bool ShouldLazyLoadMetadata = false);

  /// Read the header of the specified bitcode buffer and extract just the
  /// triple information. If successful, this returns a string. On error, this
  /// returns "".
  Expected<std::string> getBitcodeTargetTriple(MemoryBufferRef Buffer);

  /// Return true if \p Buffer contains a bitcode file with ObjC code (category
  /// or class) in it.
  Expected<bool> isBitcodeContainingObjCCategory(MemoryBufferRef Buffer);

  /// Read the header of the specified bitcode buffer and extract just the
  /// producer string information. If successful, this returns a string. On
  /// error, this returns "".
  Expected<std::string> getBitcodeProducerString(MemoryBufferRef Buffer);

  /// Read the specified bitcode file, returning the module.
  ErrorOr<std::unique_ptr<Module>> parseBitcodeFile(MemoryBufferRef Buffer,
                                                    LLVMContext &Context);

  /// Check if the given bitcode buffer contains a summary block.
  Expected<bool> hasGlobalValueSummary(MemoryBufferRef Buffer);

  /// Parse the specified bitcode buffer, returning the module summary index.
  Expected<std::unique_ptr<ModuleSummaryIndex>>
  getModuleSummaryIndex(MemoryBufferRef Buffer);

  /// isBitcodeWrapper - Return true if the given bytes are the magic bytes
  /// for an LLVM IR bitcode wrapper.
  ///
  inline bool isBitcodeWrapper(const unsigned char *BufPtr,
                               const unsigned char *BufEnd) {
    // See if you can find the hidden message in the magic bytes :-).
    // (Hint: it's a little-endian encoding.)
    return BufPtr != BufEnd &&
           BufPtr[0] == 0xDE &&
           BufPtr[1] == 0xC0 &&
           BufPtr[2] == 0x17 &&
           BufPtr[3] == 0x0B;
  }

  /// isRawBitcode - Return true if the given bytes are the magic bytes for
  /// raw LLVM IR bitcode (without a wrapper).
  ///
  inline bool isRawBitcode(const unsigned char *BufPtr,
                           const unsigned char *BufEnd) {
    // These bytes sort of have a hidden message, but it's not in
    // little-endian this time, and it's a little redundant.
    return BufPtr != BufEnd &&
           BufPtr[0] == 'B' &&
           BufPtr[1] == 'C' &&
           BufPtr[2] == 0xc0 &&
           BufPtr[3] == 0xde;
  }

  /// isBitcode - Return true if the given bytes are the magic bytes for
  /// LLVM IR bitcode, either with or without a wrapper.
  ///
  inline bool isBitcode(const unsigned char *BufPtr,
                        const unsigned char *BufEnd) {
    return isBitcodeWrapper(BufPtr, BufEnd) ||
           isRawBitcode(BufPtr, BufEnd);
  }

  /// SkipBitcodeWrapperHeader - Some systems wrap bc files with a special
  /// header for padding or other reasons.  The format of this header is:
  ///
  /// struct bc_header {
  ///   uint32_t Magic;         // 0x0B17C0DE
  ///   uint32_t Version;       // Version, currently always 0.
  ///   uint32_t BitcodeOffset; // Offset to traditional bitcode file.
  ///   uint32_t BitcodeSize;   // Size of traditional bitcode file.
  ///   ... potentially other gunk ...
  /// };
  ///
  /// This function is called when we find a file with a matching magic number.
  /// In this case, skip down to the subsection of the file that is actually a
  /// BC file.
  /// If 'VerifyBufferSize' is true, check that the buffer is large enough to
  /// contain the whole bitcode file.
  inline bool SkipBitcodeWrapperHeader(const unsigned char *&BufPtr,
                                       const unsigned char *&BufEnd,
                                       bool VerifyBufferSize) {
    // Must contain the offset and size field!
    if (unsigned(BufEnd - BufPtr) < BWH_SizeField + 4)
      return true;

    unsigned Offset = support::endian::read32le(&BufPtr[BWH_OffsetField]);
    unsigned Size = support::endian::read32le(&BufPtr[BWH_SizeField]);
    uint64_t BitcodeOffsetEnd = (uint64_t)Offset + (uint64_t)Size;

    // Verify that Offset+Size fits in the file.
    if (VerifyBufferSize && BitcodeOffsetEnd > uint64_t(BufEnd-BufPtr))
      return true;
    BufPtr += Offset;
    BufEnd = BufPtr+Size;
    return false;
  }

  const std::error_category &BitcodeErrorCategory();
  enum class BitcodeError { CorruptedBitcode = 1 };
  inline std::error_code make_error_code(BitcodeError E) {
    return std::error_code(static_cast<int>(E), BitcodeErrorCategory());
  }

} // End llvm namespace

namespace std {
template <> struct is_error_code_enum<llvm::BitcodeError> : std::true_type {};
}

#endif
