//===- StreamReader.cpp - Reads bytes and objects from a stream -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/CodeView/StreamReader.h"

#include "llvm/DebugInfo/CodeView/CodeViewError.h"
#include "llvm/DebugInfo/CodeView/StreamRef.h"

using namespace llvm;
using namespace llvm::codeview;

StreamReader::StreamReader(StreamRef S) : Stream(S), Offset(0) {}

Error StreamReader::readLongestContiguousChunk(ArrayRef<uint8_t> &Buffer) {
  if (auto EC = Stream.readLongestContiguousChunk(Offset, Buffer))
    return EC;
  Offset += Buffer.size();
  return Error::success();
}

Error StreamReader::readBytes(ArrayRef<uint8_t> &Buffer, uint32_t Size) {
  if (auto EC = Stream.readBytes(Offset, Size, Buffer))
    return EC;
  Offset += Size;
  return Error::success();
}

Error StreamReader::readInteger(uint16_t &Dest) {
  const support::ulittle16_t *P;
  if (auto EC = readObject(P))
    return EC;
  Dest = *P;
  return Error::success();
}

Error StreamReader::readInteger(uint32_t &Dest) {
  const support::ulittle32_t *P;
  if (auto EC = readObject(P))
    return EC;
  Dest = *P;
  return Error::success();
}

Error StreamReader::readZeroString(StringRef &Dest) {
  uint32_t Length = 0;
  // First compute the length of the string by reading 1 byte at a time.
  uint32_t OriginalOffset = getOffset();
  const char *C;
  do {
    if (auto EC = readObject(C))
      return EC;
    if (*C != '\0')
      ++Length;
  } while (*C != '\0');
  // Now go back and request a reference for that many bytes.
  uint32_t NewOffset = getOffset();
  setOffset(OriginalOffset);

  ArrayRef<uint8_t> Data;
  if (auto EC = readBytes(Data, Length))
    return EC;
  Dest = StringRef(reinterpret_cast<const char *>(Data.begin()), Data.size());

  // Now set the offset back to where it was after we calculated the length.
  setOffset(NewOffset);
  return Error::success();
}

Error StreamReader::readFixedString(StringRef &Dest, uint32_t Length) {
  ArrayRef<uint8_t> Bytes;
  if (auto EC = readBytes(Bytes, Length))
    return EC;
  Dest = StringRef(reinterpret_cast<const char *>(Bytes.begin()), Bytes.size());
  return Error::success();
}

Error StreamReader::readStreamRef(StreamRef &Ref) {
  return readStreamRef(Ref, bytesRemaining());
}

Error StreamReader::readStreamRef(StreamRef &Ref, uint32_t Length) {
  if (bytesRemaining() < Length)
    return make_error<CodeViewError>(cv_error_code::insufficient_buffer);
  Ref = Stream.slice(Offset, Length);
  Offset += Length;
  return Error::success();
}
