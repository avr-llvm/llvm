//===- FunctionIndexObjectFile.cpp - Function index file implementation ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Part of the FunctionIndexObjectFile class implementation.
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/FunctionIndexObjectFile.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/FunctionInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;
using namespace object;

FunctionIndexObjectFile::FunctionIndexObjectFile(
    MemoryBufferRef Object, std::unique_ptr<FunctionInfoIndex> I)
    : SymbolicFile(Binary::ID_FunctionIndex, Object), Index(std::move(I)) {}

FunctionIndexObjectFile::~FunctionIndexObjectFile() {}

std::unique_ptr<FunctionInfoIndex> FunctionIndexObjectFile::takeIndex() {
  return std::move(Index);
}

ErrorOr<MemoryBufferRef> FunctionIndexObjectFile::findBitcodeInObject(
    const ObjectFile &Obj) {
  for (const SectionRef &Sec : Obj.sections()) {
    StringRef SecName;
    if (std::error_code EC = Sec.getName(SecName)) return EC;
    if (SecName == ".llvmbc") {
      StringRef SecContents;
      if (std::error_code EC = Sec.getContents(SecContents)) return EC;
      return MemoryBufferRef(SecContents, Obj.getFileName());
    }
  }

  return object_error::bitcode_section_not_found;
}

ErrorOr<MemoryBufferRef> FunctionIndexObjectFile::findBitcodeInMemBuffer(
    MemoryBufferRef Object) {
  sys::fs::file_magic Type = sys::fs::identify_magic(Object.getBuffer());
  switch (Type) {
    case sys::fs::file_magic::bitcode:
      return Object;
    case sys::fs::file_magic::elf_relocatable:
    case sys::fs::file_magic::macho_object:
    case sys::fs::file_magic::coff_object: {
      ErrorOr<std::unique_ptr<ObjectFile>> ObjFile =
          ObjectFile::createObjectFile(Object, Type);
      if (!ObjFile) return ObjFile.getError();
      return findBitcodeInObject(*ObjFile->get());
    }
    default:
      return object_error::invalid_file_type;
  }
}

// Looks for function index in the given memory buffer.
// returns true if found, else false.
bool FunctionIndexObjectFile::hasFunctionSummaryInMemBuffer(
    MemoryBufferRef Object, LLVMContext &Context) {
  ErrorOr<MemoryBufferRef> BCOrErr = findBitcodeInMemBuffer(Object);
  if (!BCOrErr) return false;

  return hasFunctionSummary(BCOrErr.get(), Context, nullptr);
}

// Parse function index in the given memory buffer.
// Return new FunctionIndexObjectFile instance containing parsed
// function summary/index.
ErrorOr<std::unique_ptr<FunctionIndexObjectFile>>
FunctionIndexObjectFile::create(MemoryBufferRef Object, LLVMContext &Context,
                                bool IsLazy) {
  std::unique_ptr<FunctionInfoIndex> Index;

  ErrorOr<MemoryBufferRef> BCOrErr = findBitcodeInMemBuffer(Object);
  if (!BCOrErr) return BCOrErr.getError();

  ErrorOr<std::unique_ptr<FunctionInfoIndex>> IOrErr =
      getFunctionInfoIndex(BCOrErr.get(), Context, nullptr, IsLazy);

  if (std::error_code EC = IOrErr.getError()) return EC;

  Index = std::move(IOrErr.get());

  return llvm::make_unique<FunctionIndexObjectFile>(Object, std::move(Index));
}

// Parse the function summary information for function with the
// given name out of the given buffer. Parsed information is
// stored on the index object saved in this object.
std::error_code FunctionIndexObjectFile::findFunctionSummaryInMemBuffer(
    MemoryBufferRef Object, LLVMContext &Context, StringRef FunctionName) {
  sys::fs::file_magic Type = sys::fs::identify_magic(Object.getBuffer());
  switch (Type) {
    case sys::fs::file_magic::bitcode: {
      return readFunctionSummary(Object, Context, nullptr, FunctionName,
                                 std::move(Index));
    }
    default:
      return object_error::invalid_file_type;
  }
}
