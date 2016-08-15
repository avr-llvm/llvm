//===- Archive.cpp - ar File Format implementation --------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the ArchiveObjectFile class.
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/Archive.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"

using namespace llvm;
using namespace object;
using namespace llvm::support::endian;

static const char *const Magic = "!<arch>\n";
static const char *const ThinMagic = "!<thin>\n";

void Archive::anchor() { }

StringRef ArchiveMemberHeader::getName() const {
  char EndCond;
  if (Name[0] == '/' || Name[0] == '#')
    EndCond = ' ';
  else
    EndCond = '/';
  llvm::StringRef::size_type end =
      llvm::StringRef(Name, sizeof(Name)).find(EndCond);
  if (end == llvm::StringRef::npos)
    end = sizeof(Name);
  assert(end <= sizeof(Name) && end > 0);
  // Don't include the EndCond if there is one.
  return llvm::StringRef(Name, end);
}

ErrorOr<uint32_t> ArchiveMemberHeader::getSize() const {
  uint32_t Ret;
  if (llvm::StringRef(Size, sizeof(Size)).rtrim(" ").getAsInteger(10, Ret))
    return object_error::parse_failed; // Size is not a decimal number.
  return Ret;
}

sys::fs::perms ArchiveMemberHeader::getAccessMode() const {
  unsigned Ret;
  if (StringRef(AccessMode, sizeof(AccessMode)).rtrim(' ').getAsInteger(8, Ret))
    llvm_unreachable("Access mode is not an octal number.");
  return static_cast<sys::fs::perms>(Ret);
}

sys::TimeValue ArchiveMemberHeader::getLastModified() const {
  unsigned Seconds;
  if (StringRef(LastModified, sizeof(LastModified)).rtrim(' ')
          .getAsInteger(10, Seconds))
    llvm_unreachable("Last modified time not a decimal number.");

  sys::TimeValue Ret;
  Ret.fromEpochTime(Seconds);
  return Ret;
}

unsigned ArchiveMemberHeader::getUID() const {
  unsigned Ret;
  StringRef User = StringRef(UID, sizeof(UID)).rtrim(' ');
  if (User.empty())
    return 0;
  if (User.getAsInteger(10, Ret))
    llvm_unreachable("UID time not a decimal number.");
  return Ret;
}

unsigned ArchiveMemberHeader::getGID() const {
  unsigned Ret;
  StringRef Group = StringRef(GID, sizeof(GID)).rtrim(' ');
  if (Group.empty())
    return 0;
  if (Group.getAsInteger(10, Ret))
    llvm_unreachable("GID time not a decimal number.");
  return Ret;
}

Archive::Child::Child(const Archive *Parent, StringRef Data,
                      uint16_t StartOfFile)
    : Parent(Parent), Data(Data), StartOfFile(StartOfFile) {}

Archive::Child::Child(const Archive *Parent, const char *Start,
                      std::error_code *EC)
    : Parent(Parent) {
  if (!Start)
    return;

  uint64_t Size = sizeof(ArchiveMemberHeader);
  Data = StringRef(Start, Size);
  if (!isThinMember()) {
    ErrorOr<uint64_t> MemberSize = getRawSize();
    if ((*EC = MemberSize.getError()))
      return;
    Size += MemberSize.get();
    Data = StringRef(Start, Size);
  }

  // Setup StartOfFile and PaddingBytes.
  StartOfFile = sizeof(ArchiveMemberHeader);
  // Don't include attached name.
  StringRef Name = getRawName();
  if (Name.startswith("#1/")) {
    uint64_t NameSize;
    if (Name.substr(3).rtrim(' ').getAsInteger(10, NameSize))
      llvm_unreachable("Long name length is not an integer");
    StartOfFile += NameSize;
  }
}

ErrorOr<uint64_t> Archive::Child::getSize() const {
  if (Parent->IsThin) {
    ErrorOr<uint32_t> Size = getHeader()->getSize();
    if (std::error_code EC = Size.getError())
      return EC;
    return Size.get();
  }
  return Data.size() - StartOfFile;
}

ErrorOr<uint64_t> Archive::Child::getRawSize() const {
  ErrorOr<uint32_t> Size = getHeader()->getSize();
  if (std::error_code EC = Size.getError())
    return EC;
  return Size.get();
}

bool Archive::Child::isThinMember() const {
  StringRef Name = getHeader()->getName();
  return Parent->IsThin && Name != "/" && Name != "//";
}

ErrorOr<std::string> Archive::Child::getFullName() const {
  assert(isThinMember());
  ErrorOr<StringRef> NameOrErr = getName();
  if (std::error_code EC = NameOrErr.getError())
    return EC;
  StringRef Name = *NameOrErr;
  if (sys::path::is_absolute(Name))
    return Name;

  SmallString<128> FullName = sys::path::parent_path(
      Parent->getMemoryBufferRef().getBufferIdentifier());
  sys::path::append(FullName, Name);
  return StringRef(FullName);
}

ErrorOr<StringRef> Archive::Child::getBuffer() const {
  if (!isThinMember()) {
    ErrorOr<uint32_t> Size = getSize();
    if (std::error_code EC = Size.getError())
      return EC;
    return StringRef(Data.data() + StartOfFile, Size.get());
  }
  ErrorOr<std::string> FullNameOrEr = getFullName();
  if (std::error_code EC = FullNameOrEr.getError())
    return EC;
  const std::string &FullName = *FullNameOrEr;
  ErrorOr<std::unique_ptr<MemoryBuffer>> Buf = MemoryBuffer::getFile(FullName);
  if (std::error_code EC = Buf.getError())
    return EC;
  Parent->ThinBuffers.push_back(std::move(*Buf));
  return Parent->ThinBuffers.back()->getBuffer();
}

ErrorOr<Archive::Child> Archive::Child::getNext() const {
  size_t SpaceToSkip = Data.size();
  // If it's odd, add 1 to make it even.
  if (SpaceToSkip & 1)
    ++SpaceToSkip;

  const char *NextLoc = Data.data() + SpaceToSkip;

  // Check to see if this is at the end of the archive.
  if (NextLoc == Parent->Data.getBufferEnd())
    return Child(Parent, nullptr, nullptr);

  // Check to see if this is past the end of the archive.
  if (NextLoc > Parent->Data.getBufferEnd())
    return object_error::parse_failed;

  std::error_code EC;
  Child Ret(Parent, NextLoc, &EC);
  if (EC)
    return EC;
  return Ret;
}

uint64_t Archive::Child::getChildOffset() const {
  const char *a = Parent->Data.getBuffer().data();
  const char *c = Data.data();
  uint64_t offset = c - a;
  return offset;
}

ErrorOr<StringRef> Archive::Child::getName() const {
  StringRef name = getRawName();
  // Check if it's a special name.
  if (name[0] == '/') {
    if (name.size() == 1) // Linker member.
      return name;
    if (name.size() == 2 && name[1] == '/') // String table.
      return name;
    // It's a long name.
    // Get the offset.
    std::size_t offset;
    if (name.substr(1).rtrim(' ').getAsInteger(10, offset))
      llvm_unreachable("Long name offset is not an integer");

    // Verify it.
    if (offset >= Parent->StringTable.size())
      return object_error::parse_failed;
    const char *addr = Parent->StringTable.begin() + offset;

    // GNU long file names end with a "/\n".
    if (Parent->kind() == K_GNU || Parent->kind() == K_MIPS64) {
      StringRef::size_type End = StringRef(addr).find('\n');
      return StringRef(addr, End - 1);
    }
    return StringRef(addr);
  } else if (name.startswith("#1/")) {
    uint64_t name_size;
    if (name.substr(3).rtrim(' ').getAsInteger(10, name_size))
      llvm_unreachable("Long name length is not an ingeter");
    return Data.substr(sizeof(ArchiveMemberHeader), name_size).rtrim('\0');
  } else {
    // It is not a long name so trim the blanks at the end of the name.
    if (name[name.size() - 1] != '/') {
      return name.rtrim(' ');
    }
  }
  // It's a simple name.
  if (name[name.size() - 1] == '/')
    return name.substr(0, name.size() - 1);
  return name;
}

ErrorOr<MemoryBufferRef> Archive::Child::getMemoryBufferRef() const {
  ErrorOr<StringRef> NameOrErr = getName();
  if (std::error_code EC = NameOrErr.getError())
    return EC;
  StringRef Name = NameOrErr.get();
  ErrorOr<StringRef> Buf = getBuffer();
  if (std::error_code EC = Buf.getError())
    return EC;
  return MemoryBufferRef(*Buf, Name);
}

Expected<std::unique_ptr<Binary>>
Archive::Child::getAsBinary(LLVMContext *Context) const {
  ErrorOr<MemoryBufferRef> BuffOrErr = getMemoryBufferRef();
  if (std::error_code EC = BuffOrErr.getError())
    return errorCodeToError(EC);

  auto BinaryOrErr = createBinary(BuffOrErr.get(), Context);
  if (BinaryOrErr)
    return std::move(*BinaryOrErr);
  return BinaryOrErr.takeError();
}

Expected<std::unique_ptr<Archive>> Archive::create(MemoryBufferRef Source) {
  Error Err;
  std::unique_ptr<Archive> Ret(new Archive(Source, Err));
  if (Err)
    return std::move(Err);
  return std::move(Ret);
}

void Archive::setFirstRegular(const Child &C) {
  FirstRegularData = C.Data;
  FirstRegularStartOfFile = C.StartOfFile;
}

Archive::Archive(MemoryBufferRef Source, Error &Err)
    : Binary(Binary::ID_Archive, Source) {
  ErrorAsOutParameter ErrAsOutParam(Err);
  StringRef Buffer = Data.getBuffer();
  // Check for sufficient magic.
  if (Buffer.startswith(ThinMagic)) {
    IsThin = true;
  } else if (Buffer.startswith(Magic)) {
    IsThin = false;
  } else {
    Err = make_error<GenericBinaryError>("File too small to be an archive",
                                         object_error::invalid_file_type);
    return;
  }

  // Get the special members.
  child_iterator I = child_begin(Err, false);
  if (Err)
    return;
  child_iterator E = child_end();

  // This is at least a valid empty archive. Since an empty archive is the
  // same in all formats, just claim it to be gnu to make sure Format is
  // initialized.
  Format = K_GNU;

  if (I == E) {
    Err = Error::success();
    return;
  }
  const Child *C = &*I;

  auto Increment = [&]() {
    ++I;
    if (Err)
      return true;
    C = &*I;
    return false;
  };

  StringRef Name = C->getRawName();

  // Below is the pattern that is used to figure out the archive format
  // GNU archive format
  //  First member : / (may exist, if it exists, points to the symbol table )
  //  Second member : // (may exist, if it exists, points to the string table)
  //  Note : The string table is used if the filename exceeds 15 characters
  // BSD archive format
  //  First member : __.SYMDEF or "__.SYMDEF SORTED" (the symbol table)
  //  There is no string table, if the filename exceeds 15 characters or has a
  //  embedded space, the filename has #1/<size>, The size represents the size
  //  of the filename that needs to be read after the archive header
  // COFF archive format
  //  First member : /
  //  Second member : / (provides a directory of symbols)
  //  Third member : // (may exist, if it exists, contains the string table)
  //  Note: Microsoft PE/COFF Spec 8.3 says that the third member is present
  //  even if the string table is empty. However, lib.exe does not in fact
  //  seem to create the third member if there's no member whose filename
  //  exceeds 15 characters. So the third member is optional.

  if (Name == "__.SYMDEF" || Name == "__.SYMDEF_64") {
    if (Name == "__.SYMDEF")
      Format = K_BSD;
    else // Name == "__.SYMDEF_64"
      Format = K_DARWIN64;
    // We know that the symbol table is not an external file, so we just assert
    // there is no error.
    SymbolTable = *C->getBuffer();
    if (Increment())
      return;
    setFirstRegular(*C);

    Err = Error::success();
    return;
  }

  if (Name.startswith("#1/")) {
    Format = K_BSD;
    // We know this is BSD, so getName will work since there is no string table.
    ErrorOr<StringRef> NameOrErr = C->getName();
    if (auto ec = NameOrErr.getError()) {
      Err = errorCodeToError(ec);
      return;
    }
    Name = NameOrErr.get();
    if (Name == "__.SYMDEF SORTED" || Name == "__.SYMDEF") {
      // We know that the symbol table is not an external file, so we just
      // assert there is no error.
      SymbolTable = *C->getBuffer();
      if (Increment())
        return;
    }
    else if (Name == "__.SYMDEF_64 SORTED" || Name == "__.SYMDEF_64") {
      Format = K_DARWIN64;
      // We know that the symbol table is not an external file, so we just
      // assert there is no error.
      SymbolTable = *C->getBuffer();
      if (Increment())
        return;
    }
    setFirstRegular(*C);
    return;
  }

  // MIPS 64-bit ELF archives use a special format of a symbol table.
  // This format is marked by `ar_name` field equals to "/SYM64/".
  // For detailed description see page 96 in the following document:
  // http://techpubs.sgi.com/library/manuals/4000/007-4658-001/pdf/007-4658-001.pdf

  bool has64SymTable = false;
  if (Name == "/" || Name == "/SYM64/") {
    // We know that the symbol table is not an external file, so we just assert
    // there is no error.
    SymbolTable = *C->getBuffer();
    if (Name == "/SYM64/")
      has64SymTable = true;

    if (Increment())
      return;
    if (I == E) {
      Err = Error::success();
      return;
    }
    Name = C->getRawName();
  }

  if (Name == "//") {
    Format = has64SymTable ? K_MIPS64 : K_GNU;
    // The string table is never an external member, so we just assert on the
    // ErrorOr.
    StringTable = *C->getBuffer();
    if (Increment())
      return;
    setFirstRegular(*C);
    Err = Error::success();
    return;
  }

  if (Name[0] != '/') {
    Format = has64SymTable ? K_MIPS64 : K_GNU;
    setFirstRegular(*C);
    Err = Error::success();
    return;
  }

  if (Name != "/") {
    Err = errorCodeToError(object_error::parse_failed);
    return;
  }

  Format = K_COFF;
  // We know that the symbol table is not an external file, so we just assert
  // there is no error.
  SymbolTable = *C->getBuffer();

  if (Increment())
    return;

  if (I == E) {
    setFirstRegular(*C);
    Err = Error::success();
    return;
  }

  Name = C->getRawName();

  if (Name == "//") {
    // The string table is never an external member, so we just assert on the
    // ErrorOr.
    StringTable = *C->getBuffer();
    if (Increment())
      return;
  }

  setFirstRegular(*C);
  Err = Error::success();
}

Archive::child_iterator Archive::child_begin(Error &Err,
                                             bool SkipInternal) const {
  if (Data.getBufferSize() == 8) // empty archive.
    return child_end();

  if (SkipInternal)
    return child_iterator(Child(this, FirstRegularData,
                                FirstRegularStartOfFile),
                          &Err);

  const char *Loc = Data.getBufferStart() + strlen(Magic);
  std::error_code EC;
  Child C(this, Loc, &EC);
  if (EC) {
    ErrorAsOutParameter ErrAsOutParam(Err);
    Err = errorCodeToError(EC);
    return child_end();
  }
  return child_iterator(C, &Err);
}

Archive::child_iterator Archive::child_end() const {
  return child_iterator(Child(this, nullptr, nullptr), nullptr);
}

StringRef Archive::Symbol::getName() const {
  return Parent->getSymbolTable().begin() + StringIndex;
}

ErrorOr<Archive::Child> Archive::Symbol::getMember() const {
  const char *Buf = Parent->getSymbolTable().begin();
  const char *Offsets = Buf;
  if (Parent->kind() == K_MIPS64 || Parent->kind() == K_DARWIN64)
    Offsets += sizeof(uint64_t);
  else
    Offsets += sizeof(uint32_t);
  uint32_t Offset = 0;
  if (Parent->kind() == K_GNU) {
    Offset = read32be(Offsets + SymbolIndex * 4);
  } else if (Parent->kind() == K_MIPS64) {
    Offset = read64be(Offsets + SymbolIndex * 8);
  } else if (Parent->kind() == K_BSD) {
    // The SymbolIndex is an index into the ranlib structs that start at
    // Offsets (the first uint32_t is the number of bytes of the ranlib
    // structs).  The ranlib structs are a pair of uint32_t's the first
    // being a string table offset and the second being the offset into
    // the archive of the member that defines the symbol.  Which is what
    // is needed here.
    Offset = read32le(Offsets + SymbolIndex * 8 + 4);
  } else if (Parent->kind() == K_DARWIN64) {
    // The SymbolIndex is an index into the ranlib_64 structs that start at
    // Offsets (the first uint64_t is the number of bytes of the ranlib_64
    // structs).  The ranlib_64 structs are a pair of uint64_t's the first
    // being a string table offset and the second being the offset into
    // the archive of the member that defines the symbol.  Which is what
    // is needed here.
    Offset = read64le(Offsets + SymbolIndex * 16 + 8);
  } else {
    // Skip offsets.
    uint32_t MemberCount = read32le(Buf);
    Buf += MemberCount * 4 + 4;

    uint32_t SymbolCount = read32le(Buf);
    if (SymbolIndex >= SymbolCount)
      return object_error::parse_failed;

    // Skip SymbolCount to get to the indices table.
    const char *Indices = Buf + 4;

    // Get the index of the offset in the file member offset table for this
    // symbol.
    uint16_t OffsetIndex = read16le(Indices + SymbolIndex * 2);
    // Subtract 1 since OffsetIndex is 1 based.
    --OffsetIndex;

    if (OffsetIndex >= MemberCount)
      return object_error::parse_failed;

    Offset = read32le(Offsets + OffsetIndex * 4);
  }

  const char *Loc = Parent->getData().begin() + Offset;
  std::error_code EC;
  Child C(Parent, Loc, &EC);
  if (EC)
    return EC;
  return C;
}

Archive::Symbol Archive::Symbol::getNext() const {
  Symbol t(*this);
  if (Parent->kind() == K_BSD) {
    // t.StringIndex is an offset from the start of the __.SYMDEF or
    // "__.SYMDEF SORTED" member into the string table for the ranlib
    // struct indexed by t.SymbolIndex .  To change t.StringIndex to the
    // offset in the string table for t.SymbolIndex+1 we subtract the
    // its offset from the start of the string table for t.SymbolIndex
    // and add the offset of the string table for t.SymbolIndex+1.

    // The __.SYMDEF or "__.SYMDEF SORTED" member starts with a uint32_t
    // which is the number of bytes of ranlib structs that follow.  The ranlib
    // structs are a pair of uint32_t's the first being a string table offset
    // and the second being the offset into the archive of the member that
    // define the symbol. After that the next uint32_t is the byte count of
    // the string table followed by the string table.
    const char *Buf = Parent->getSymbolTable().begin();
    uint32_t RanlibCount = 0;
    RanlibCount = read32le(Buf) / 8;
    // If t.SymbolIndex + 1 will be past the count of symbols (the RanlibCount)
    // don't change the t.StringIndex as we don't want to reference a ranlib
    // past RanlibCount.
    if (t.SymbolIndex + 1 < RanlibCount) {
      const char *Ranlibs = Buf + 4;
      uint32_t CurRanStrx = 0;
      uint32_t NextRanStrx = 0;
      CurRanStrx = read32le(Ranlibs + t.SymbolIndex * 8);
      NextRanStrx = read32le(Ranlibs + (t.SymbolIndex + 1) * 8);
      t.StringIndex -= CurRanStrx;
      t.StringIndex += NextRanStrx;
    }
  } else {
    // Go to one past next null.
    t.StringIndex = Parent->getSymbolTable().find('\0', t.StringIndex) + 1;
  }
  ++t.SymbolIndex;
  return t;
}

Archive::symbol_iterator Archive::symbol_begin() const {
  if (!hasSymbolTable())
    return symbol_iterator(Symbol(this, 0, 0));

  const char *buf = getSymbolTable().begin();
  if (kind() == K_GNU) {
    uint32_t symbol_count = 0;
    symbol_count = read32be(buf);
    buf += sizeof(uint32_t) + (symbol_count * (sizeof(uint32_t)));
  } else if (kind() == K_MIPS64) {
    uint64_t symbol_count = read64be(buf);
    buf += sizeof(uint64_t) + (symbol_count * (sizeof(uint64_t)));
  } else if (kind() == K_BSD) {
    // The __.SYMDEF or "__.SYMDEF SORTED" member starts with a uint32_t
    // which is the number of bytes of ranlib structs that follow.  The ranlib
    // structs are a pair of uint32_t's the first being a string table offset
    // and the second being the offset into the archive of the member that
    // define the symbol. After that the next uint32_t is the byte count of
    // the string table followed by the string table.
    uint32_t ranlib_count = 0;
    ranlib_count = read32le(buf) / 8;
    const char *ranlibs = buf + 4;
    uint32_t ran_strx = 0;
    ran_strx = read32le(ranlibs);
    buf += sizeof(uint32_t) + (ranlib_count * (2 * (sizeof(uint32_t))));
    // Skip the byte count of the string table.
    buf += sizeof(uint32_t);
    buf += ran_strx;
  } else if (kind() == K_DARWIN64) {
    // The __.SYMDEF_64 or "__.SYMDEF_64 SORTED" member starts with a uint64_t
    // which is the number of bytes of ranlib_64 structs that follow.  The
    // ranlib_64 structs are a pair of uint64_t's the first being a string
    // table offset and the second being the offset into the archive of the
    // member that define the symbol. After that the next uint64_t is the byte
    // count of the string table followed by the string table.
    uint64_t ranlib_count = 0;
    ranlib_count = read64le(buf) / 16;
    const char *ranlibs = buf + 8;
    uint64_t ran_strx = 0;
    ran_strx = read64le(ranlibs);
    buf += sizeof(uint64_t) + (ranlib_count * (2 * (sizeof(uint64_t))));
    // Skip the byte count of the string table.
    buf += sizeof(uint64_t);
    buf += ran_strx;
  } else {
    uint32_t member_count = 0;
    uint32_t symbol_count = 0;
    member_count = read32le(buf);
    buf += 4 + (member_count * 4); // Skip offsets.
    symbol_count = read32le(buf);
    buf += 4 + (symbol_count * 2); // Skip indices.
  }
  uint32_t string_start_offset = buf - getSymbolTable().begin();
  return symbol_iterator(Symbol(this, 0, string_start_offset));
}

Archive::symbol_iterator Archive::symbol_end() const {
  return symbol_iterator(Symbol(this, getNumberOfSymbols(), 0));
}

uint32_t Archive::getNumberOfSymbols() const {
  if (!hasSymbolTable())
    return 0;
  const char *buf = getSymbolTable().begin();
  if (kind() == K_GNU)
    return read32be(buf);
  if (kind() == K_MIPS64)
    return read64be(buf);
  if (kind() == K_BSD)
    return read32le(buf) / 8;
  if (kind() == K_DARWIN64)
    return read64le(buf) / 16;
  uint32_t member_count = 0;
  member_count = read32le(buf);
  buf += 4 + (member_count * 4); // Skip offsets.
  return read32le(buf);
}

Expected<Optional<Archive::Child>> Archive::findSym(StringRef name) const {
  Archive::symbol_iterator bs = symbol_begin();
  Archive::symbol_iterator es = symbol_end();

  for (; bs != es; ++bs) {
    StringRef SymName = bs->getName();
    if (SymName == name) {
      if (auto MemberOrErr = bs->getMember())
        return Child(*MemberOrErr);
      else
        return errorCodeToError(MemberOrErr.getError());
    }
  }
  return Optional<Child>();
}

bool Archive::hasSymbolTable() const { return !SymbolTable.empty(); }
