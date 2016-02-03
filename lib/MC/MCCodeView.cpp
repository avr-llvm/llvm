//===- MCCodeView.h - Machine Code CodeView support -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Holds state from .cv_file and .cv_loc directives for later emission.
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCCodeView.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/DebugInfo/CodeView/CodeView.h"
#include "llvm/DebugInfo/CodeView/Line.h"
#include "llvm/DebugInfo/CodeView/SymbolRecord.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCObjectStreamer.h"
#include "llvm/Support/COFF.h"

using namespace llvm;
using namespace llvm::codeview;

CodeViewContext::CodeViewContext() {}

CodeViewContext::~CodeViewContext() {
  // If someone inserted strings into the string table but never actually
  // emitted them somewhere, clean up the fragment.
  if (!InsertedStrTabFragment)
    delete StrTabFragment;
}

/// This is a valid number for use with .cv_loc if we've already seen a .cv_file
/// for it.
bool CodeViewContext::isValidFileNumber(unsigned FileNumber) const {
  unsigned Idx = FileNumber - 1;
  if (Idx < Filenames.size())
    return !Filenames[Idx].empty();
  return false;
}

bool CodeViewContext::addFile(unsigned FileNumber, StringRef Filename) {
  assert(FileNumber > 0);
  Filename = addToStringTable(Filename);
  unsigned Idx = FileNumber - 1;
  if (Idx >= Filenames.size())
    Filenames.resize(Idx + 1);

  if (Filename.empty())
    Filename = "<stdin>";

  if (!Filenames[Idx].empty())
    return false;

  // FIXME: We should store the string table offset of the filename, rather than
  // the filename itself for efficiency.
  Filename = addToStringTable(Filename);

  Filenames[Idx] = Filename;
  return true;
}

MCDataFragment *CodeViewContext::getStringTableFragment() {
  if (!StrTabFragment) {
    StrTabFragment = new MCDataFragment();
    // Start a new string table out with a null byte.
    StrTabFragment->getContents().push_back('\0');
  }
  return StrTabFragment;
}

StringRef CodeViewContext::addToStringTable(StringRef S) {
  SmallVectorImpl<char> &Contents = getStringTableFragment()->getContents();
  auto Insertion =
      StringTable.insert(std::make_pair(S, unsigned(Contents.size())));
  // Return the string from the table, since it is stable.
  S = Insertion.first->first();
  if (Insertion.second) {
    // The string map key is always null terminated.
    Contents.append(S.begin(), S.end() + 1);
  }
  return S;
}

unsigned CodeViewContext::getStringTableOffset(StringRef S) {
  // A string table offset of zero is always the empty string.
  if (S.empty())
    return 0;
  auto I = StringTable.find(S);
  assert(I != StringTable.end());
  return I->second;
}

void CodeViewContext::emitStringTable(MCObjectStreamer &OS) {
  MCContext &Ctx = OS.getContext();
  MCSymbol *StringBegin = Ctx.createTempSymbol("strtab_begin", false),
           *StringEnd = Ctx.createTempSymbol("strtab_end", false);

  OS.EmitIntValue(unsigned(ModuleSubstreamKind::StringTable), 4);
  OS.emitAbsoluteSymbolDiff(StringEnd, StringBegin, 4);
  OS.EmitLabel(StringBegin);

  // Put the string table data fragment here, if we haven't already put it
  // somewhere else. If somebody wants two string tables in their .s file, one
  // will just be empty.
  if (!InsertedStrTabFragment) {
    OS.insert(getStringTableFragment());
    InsertedStrTabFragment = true;
  }

  OS.EmitValueToAlignment(4, 0);

  OS.EmitLabel(StringEnd);
}

void CodeViewContext::emitFileChecksums(MCObjectStreamer &OS) {
  MCContext &Ctx = OS.getContext();
  MCSymbol *FileBegin = Ctx.createTempSymbol("filechecksums_begin", false),
           *FileEnd = Ctx.createTempSymbol("filechecksums_end", false);

  OS.EmitIntValue(unsigned(ModuleSubstreamKind::FileChecksums), 4);
  OS.emitAbsoluteSymbolDiff(FileEnd, FileBegin, 4);
  OS.EmitLabel(FileBegin);

  // Emit an array of FileChecksum entries. We index into this table using the
  // user-provided file number. Each entry is currently 8 bytes, as we don't
  // emit checksums.
  for (StringRef Filename : Filenames) {
    OS.EmitIntValue(getStringTableOffset(Filename), 4);
    // Zero the next two fields and align back to 4 bytes. This indicates that
    // no checksum is present.
    OS.EmitIntValue(0, 4);
  }

  OS.EmitLabel(FileEnd);
}

void CodeViewContext::emitLineTableForFunction(MCObjectStreamer &OS,
                                               unsigned FuncId,
                                               const MCSymbol *FuncBegin,
                                               const MCSymbol *FuncEnd) {
  MCContext &Ctx = OS.getContext();
  MCSymbol *LineBegin = Ctx.createTempSymbol("linetable_begin", false),
           *LineEnd = Ctx.createTempSymbol("linetable_end", false);

  OS.EmitIntValue(unsigned(ModuleSubstreamKind::Lines), 4);
  OS.emitAbsoluteSymbolDiff(LineEnd, LineBegin, 4);
  OS.EmitLabel(LineBegin);
  OS.EmitCOFFSecRel32(FuncBegin);
  OS.EmitCOFFSectionIndex(FuncBegin);

  // Actual line info.
  std::vector<MCCVLineEntry> Locs = getFunctionLineEntries(FuncId);
  bool HaveColumns = any_of(Locs, [](const MCCVLineEntry &LineEntry) {
    return LineEntry.getColumn() != 0;
  });
  OS.EmitIntValue(HaveColumns ? int(LineFlags::HaveColumns) : 0, 2);
  OS.emitAbsoluteSymbolDiff(FuncEnd, FuncBegin, 4);

  for (auto I = Locs.begin(), E = Locs.end(); I != E;) {
    // Emit a file segment for the run of locations that share a file id.
    unsigned CurFileNum = I->getFileNum();
    auto FileSegEnd =
        std::find_if(I, E, [CurFileNum](const MCCVLineEntry &Loc) {
          return Loc.getFileNum() != CurFileNum;
        });
    unsigned EntryCount = FileSegEnd - I;
    OS.AddComment("Segment for file '" + Twine(Filenames[CurFileNum - 1]) +
                  "' begins");
    OS.EmitIntValue(8 * (CurFileNum - 1), 4);
    OS.EmitIntValue(EntryCount, 4);
    uint32_t SegmentSize = 12;
    SegmentSize += 8 * EntryCount;
    if (HaveColumns)
      SegmentSize += 4 * EntryCount;
    OS.EmitIntValue(SegmentSize, 4);

    for (auto J = I; J != FileSegEnd; ++J) {
      OS.emitAbsoluteSymbolDiff(J->getLabel(), FuncBegin, 4);
      unsigned LineData = J->getLine();
      if (J->isStmt())
        LineData |= LineInfo::StatementFlag;
      OS.EmitIntValue(LineData, 4);
    }
    if (HaveColumns) {
      for (auto J = I; J != FileSegEnd; ++J) {
        OS.EmitIntValue(J->getColumn(), 2);
        OS.EmitIntValue(0, 2);
      }
    }
    I = FileSegEnd;
  }
  OS.EmitLabel(LineEnd);
}

static bool compressAnnotation(uint32_t Data, SmallVectorImpl<char> &Buffer) {
  if (isUInt<7>(Data)) {
    Buffer.push_back(Data);
    return true;
  }

  if (isUInt<14>(Data)) {
    Buffer.push_back((Data >> 8) | 0x80);
    Buffer.push_back(Data & 0xff);
    return true;
  }

  if (isUInt<29>(Data)) {
    Buffer.push_back((Data >> 24) | 0xC0);
    Buffer.push_back((Data >> 16) & 0xff);
    Buffer.push_back((Data >> 8) & 0xff);
    Buffer.push_back(Data & 0xff);
    return true;
  }

  return false;
}

static uint32_t encodeSignedNumber(uint32_t Data) {
  if (Data >> 31)
    return ((-Data) << 1) | 1;
  return Data << 1;
}

void CodeViewContext::emitInlineLineTableForFunction(
    MCObjectStreamer &OS, unsigned PrimaryFunctionId, unsigned SourceFileId,
    unsigned SourceLineNum, const MCSymbol *FnStartSym,
    const MCSymbol *FnEndSym, ArrayRef<unsigned> SecondaryFunctionIds) {
  // Create and insert a fragment into the current section that will be encoded
  // later.
  new MCCVInlineLineTableFragment(
      PrimaryFunctionId, SourceFileId, SourceLineNum, FnStartSym, FnEndSym,
      SecondaryFunctionIds, OS.getCurrentSectionOnly());
}

unsigned computeLabelDiff(MCAsmLayout &Layout, const MCSymbol *Begin,
                          const MCSymbol *End) {
  MCContext &Ctx = Layout.getAssembler().getContext();
  MCSymbolRefExpr::VariantKind Variant = MCSymbolRefExpr::VK_None;
  const MCExpr *BeginRef = MCSymbolRefExpr::create(Begin, Variant, Ctx),
               *EndRef = MCSymbolRefExpr::create(End, Variant, Ctx);
  const MCExpr *AddrDelta =
      MCBinaryExpr::create(MCBinaryExpr::Sub, EndRef, BeginRef, Ctx);
  int64_t Result;
  bool Success = AddrDelta->evaluateKnownAbsolute(Result, Layout);
  assert(Success && "failed to evaluate label difference as absolute");
  (void)Success;
  assert(Result >= 0 && "negative label difference requested");
  assert(Result < UINT_MAX && "label difference greater than 2GB");
  return unsigned(Result);
}

void CodeViewContext::encodeInlineLineTable(MCAsmLayout &Layout,
                                            MCCVInlineLineTableFragment &Frag) {
  size_t LocBegin;
  size_t LocEnd;
  std::tie(LocBegin, LocEnd) = getLineExtent(Frag.SiteFuncId);
  for (unsigned SecondaryId : Frag.SecondaryFuncs) {
    auto Extent = getLineExtent(SecondaryId);
    LocBegin = std::min(LocBegin, Extent.first);
    LocEnd = std::max(LocEnd, Extent.second);
  }
  if (LocBegin >= LocEnd)
    return;
  ArrayRef<MCCVLineEntry> Locs = getLinesForExtent(LocBegin, LocEnd);
  if (Locs.empty())
    return;

  SmallSet<unsigned, 8> InlinedFuncIds;
  InlinedFuncIds.insert(Frag.SiteFuncId);
  InlinedFuncIds.insert(Frag.SecondaryFuncs.begin(), Frag.SecondaryFuncs.end());

  // Make an artificial start location using the function start and the inlinee
  // lines start location information. All deltas start relative to this
  // location.
  MCCVLineEntry StartLoc(Frag.getFnStartSym(), MCCVLoc(Locs.front()));
  StartLoc.setFileNum(Frag.StartFileId);
  StartLoc.setLine(Frag.StartLineNum);
  const MCCVLineEntry *LastLoc = &StartLoc;
  bool WithinFunction = true;

  SmallVectorImpl<char> &Buffer = Frag.getContents();
  Buffer.clear(); // Clear old contents if we went through relaxation.
  for (const MCCVLineEntry &Loc : Locs) {
    if (!InlinedFuncIds.count(Loc.getFunctionId())) {
      // We've hit a cv_loc not attributed to this inline call site. Use this
      // label to end the PC range.
      if (WithinFunction) {
        unsigned Length =
            computeLabelDiff(Layout, LastLoc->getLabel(), Loc.getLabel());
        compressAnnotation(ChangeCodeLength, Buffer);
        compressAnnotation(Length, Buffer);
      }
      WithinFunction = false;
      continue;
    }
    WithinFunction = true;

    if (Loc.getFileNum() != LastLoc->getFileNum()) {
      compressAnnotation(ChangeFile, Buffer);
      compressAnnotation(Loc.getFileNum(), Buffer);
    }

    int LineDelta = Loc.getLine() - LastLoc->getLine();
    if (LineDelta == 0)
      continue;

    unsigned EncodedLineDelta = encodeSignedNumber(LineDelta);
    unsigned CodeDelta =
        computeLabelDiff(Layout, LastLoc->getLabel(), Loc.getLabel());
    if (CodeDelta == 0) {
      compressAnnotation(ChangeLineOffset, Buffer);
      compressAnnotation(EncodedLineDelta, Buffer);
    } else if (EncodedLineDelta < 0x8 && CodeDelta <= 0xf) {
      // The ChangeCodeOffsetAndLineOffset combination opcode is used when the
      // encoded line delta uses 3 or fewer set bits and the code offset fits
      // in one nibble.
      unsigned Operand = (EncodedLineDelta << 4) | CodeDelta;
      compressAnnotation(ChangeCodeOffsetAndLineOffset, Buffer);
      compressAnnotation(Operand, Buffer);
    } else {
      // Otherwise use the separate line and code deltas.
      compressAnnotation(ChangeLineOffset, Buffer);
      compressAnnotation(EncodedLineDelta, Buffer);
      compressAnnotation(ChangeCodeOffset, Buffer);
      compressAnnotation(CodeDelta, Buffer);
    }

    LastLoc = &Loc;
  }

  assert(WithinFunction);

  unsigned EndSymLength =
      computeLabelDiff(Layout, LastLoc->getLabel(), Frag.getFnEndSym());
  unsigned LocAfterLength = ~0U;
  ArrayRef<MCCVLineEntry> LocAfter = getLinesForExtent(LocEnd, LocEnd + 1);
  if (!LocAfter.empty())
    LocAfterLength =
        computeLabelDiff(Layout, LastLoc->getLabel(), LocAfter[0].getLabel());

  compressAnnotation(ChangeCodeLength, Buffer);
  compressAnnotation(std::min(EndSymLength, LocAfterLength), Buffer);
}

//
// This is called when an instruction is assembled into the specified section
// and if there is information from the last .cv_loc directive that has yet to have
// a line entry made for it is made.
//
void MCCVLineEntry::Make(MCObjectStreamer *MCOS) {
  if (!MCOS->getContext().getCVLocSeen())
    return;

  // Create a symbol at in the current section for use in the line entry.
  MCSymbol *LineSym = MCOS->getContext().createTempSymbol();
  // Set the value of the symbol to use for the MCCVLineEntry.
  MCOS->EmitLabel(LineSym);

  // Get the current .loc info saved in the context.
  const MCCVLoc &CVLoc = MCOS->getContext().getCurrentCVLoc();

  // Create a (local) line entry with the symbol and the current .loc info.
  MCCVLineEntry LineEntry(LineSym, CVLoc);

  // clear CVLocSeen saying the current .loc info is now used.
  MCOS->getContext().clearCVLocSeen();

  // Add the line entry to this section's entries.
  MCOS->getContext().getCVContext().addLineEntry(LineEntry);
}
