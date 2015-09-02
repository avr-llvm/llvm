//===-- WebAssemblyMCAsmInfo.cpp - WebAssembly asm properties -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file contains the declarations of the WebAssemblyMCAsmInfo
/// properties.
///
//===----------------------------------------------------------------------===//

#include "WebAssemblyMCAsmInfo.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/CommandLine.h"
using namespace llvm;

#define DEBUG_TYPE "wasm-mc-asm-info"

WebAssemblyMCAsmInfo::~WebAssemblyMCAsmInfo() {}

WebAssemblyMCAsmInfo::WebAssemblyMCAsmInfo(const Triple &T) {
  PointerSize = CalleeSaveStackSlotSize = T.isArch64Bit() ? 8 : 4;

  // TODO: What should MaxInstLength be?

  // The s-expression format of WebAssembly uses LISP-style comments.
  CommentString = ";;";

  PrivateGlobalPrefix = "";
  PrivateLabelPrefix = "";

  UseDataRegionDirectives = true;

  Data8bitsDirective = "\t.int8\t";
  Data16bitsDirective = "\t.int16\t";
  Data32bitsDirective = "\t.int32\t";
  Data64bitsDirective = "\t.int64\t";

  AlignmentIsInBytes = false;
  COMMDirectiveAlignmentIsInBytes = false;
  LCOMMDirectiveAlignmentType = LCOMM::Log2Alignment;

  HasDotTypeDotSizeDirective = false;
  HasSingleParameterDotFile = false;

  SupportsDebugInformation = true;

  // For now, WebAssembly does not support exceptions.
  ExceptionsType = ExceptionHandling::None;

  // FIXME: modify AsmPrinter to be more flexible, and fix other virtual ISAs.
  WeakDirective = "\t;; .weak\t";
  GlobalDirective = "\t;; .globl\t";

  // TODO: UseIntegratedAssembler?
}
