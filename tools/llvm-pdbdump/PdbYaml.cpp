//===- PdbYAML.cpp -------------------------------------------- *- C++ --*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "PdbYaml.h"

#include "llvm/DebugInfo/PDB/PDBExtras.h"
#include "llvm/DebugInfo/PDB/Raw/PDBFile.h"

using namespace llvm;
using namespace llvm::yaml;
using namespace llvm::pdb;
using namespace llvm::pdb::yaml;

namespace llvm {
namespace yaml {
template <> struct ScalarTraits<llvm::pdb::PDB_UniqueId> {
  static void output(const llvm::pdb::PDB_UniqueId &S, void *,
                     llvm::raw_ostream &OS) {
    OS << S;
  }

  static StringRef input(StringRef Scalar, void *Ctx,
                         llvm::pdb::PDB_UniqueId &S) {
    if (Scalar.size() != 38)
      return "GUID strings are 38 characters long";
    if (Scalar[0] != '{' || Scalar[37] != '}')
      return "GUID is not enclosed in {}";
    if (Scalar[9] != '-' || Scalar[14] != '-' || Scalar[19] != '-' ||
        Scalar[24] != '-')
      return "GUID sections are not properly delineated with dashes";

    char *OutBuffer = S.Guid;
    for (auto Iter = Scalar.begin(); Iter != Scalar.end();) {
      if (*Iter == '-' || *Iter == '{' || *Iter == '}') {
        ++Iter;
        continue;
      }
      uint8_t Value = (llvm::hexDigitValue(*Iter) << 4);
      ++Iter;
      Value |= llvm::hexDigitValue(*Iter);
      ++Iter;
      *OutBuffer++ = Value;
    }

    return "";
  }

  static bool mustQuote(StringRef Scalar) { return needsQuotes(Scalar); }
};

template <> struct ScalarEnumerationTraits<llvm::pdb::PDB_Machine> {
  static void enumeration(IO &io, llvm::pdb::PDB_Machine &Value) {
    io.enumCase(Value, "Invalid", PDB_Machine::Invalid);
    io.enumCase(Value, "Am33", PDB_Machine::Am33);
    io.enumCase(Value, "Amd64", PDB_Machine::Amd64);
    io.enumCase(Value, "Arm", PDB_Machine::Arm);
    io.enumCase(Value, "ArmNT", PDB_Machine::ArmNT);
    io.enumCase(Value, "Ebc", PDB_Machine::Ebc);
    io.enumCase(Value, "x86", PDB_Machine::x86);
    io.enumCase(Value, "Ia64", PDB_Machine::Ia64);
    io.enumCase(Value, "M32R", PDB_Machine::M32R);
    io.enumCase(Value, "Mips16", PDB_Machine::Mips16);
    io.enumCase(Value, "MipsFpu", PDB_Machine::MipsFpu);
    io.enumCase(Value, "MipsFpu16", PDB_Machine::MipsFpu16);
    io.enumCase(Value, "PowerPCFP", PDB_Machine::PowerPCFP);
    io.enumCase(Value, "R4000", PDB_Machine::R4000);
    io.enumCase(Value, "SH3", PDB_Machine::SH3);
    io.enumCase(Value, "SH3DSP", PDB_Machine::SH3DSP);
    io.enumCase(Value, "Thumb", PDB_Machine::Thumb);
    io.enumCase(Value, "WceMipsV2", PDB_Machine::WceMipsV2);
  }
};

template <> struct ScalarEnumerationTraits<llvm::pdb::PdbRaw_DbiVer> {
  static void enumeration(IO &io, llvm::pdb::PdbRaw_DbiVer &Value) {
    io.enumCase(Value, "V41", llvm::pdb::PdbRaw_DbiVer::PdbDbiVC41);
    io.enumCase(Value, "V50", llvm::pdb::PdbRaw_DbiVer::PdbDbiV50);
    io.enumCase(Value, "V60", llvm::pdb::PdbRaw_DbiVer::PdbDbiV60);
    io.enumCase(Value, "V70", llvm::pdb::PdbRaw_DbiVer::PdbDbiV70);
    io.enumCase(Value, "V110", llvm::pdb::PdbRaw_DbiVer::PdbDbiV110);
  }
};

template <> struct ScalarEnumerationTraits<llvm::pdb::PdbRaw_ImplVer> {
  static void enumeration(IO &io, llvm::pdb::PdbRaw_ImplVer &Value) {
    io.enumCase(Value, "VC2", llvm::pdb::PdbRaw_ImplVer::PdbImplVC2);
    io.enumCase(Value, "VC4", llvm::pdb::PdbRaw_ImplVer::PdbImplVC4);
    io.enumCase(Value, "VC41", llvm::pdb::PdbRaw_ImplVer::PdbImplVC41);
    io.enumCase(Value, "VC50", llvm::pdb::PdbRaw_ImplVer::PdbImplVC50);
    io.enumCase(Value, "VC98", llvm::pdb::PdbRaw_ImplVer::PdbImplVC98);
    io.enumCase(Value, "VC70Dep", llvm::pdb::PdbRaw_ImplVer::PdbImplVC70Dep);
    io.enumCase(Value, "VC70", llvm::pdb::PdbRaw_ImplVer::PdbImplVC70);
    io.enumCase(Value, "VC80", llvm::pdb::PdbRaw_ImplVer::PdbImplVC80);
    io.enumCase(Value, "VC110", llvm::pdb::PdbRaw_ImplVer::PdbImplVC110);
    io.enumCase(Value, "VC140", llvm::pdb::PdbRaw_ImplVer::PdbImplVC140);
  }
};
}
}

void MappingTraits<PdbObject>::mapping(IO &IO, PdbObject &Obj) {
  IO.mapOptional("MSF", Obj.Headers);
  IO.mapOptional("StreamSizes", Obj.StreamSizes);
  IO.mapOptional("StreamMap", Obj.StreamMap);
  IO.mapOptional("PdbStream", Obj.PdbStream);
  IO.mapOptional("DbiStream", Obj.DbiStream);
}

void MappingTraits<MsfHeaders>::mapping(IO &IO, MsfHeaders &Obj) {
  IO.mapRequired("SuperBlock", Obj.SuperBlock);
  IO.mapRequired("NumDirectoryBlocks", Obj.NumDirectoryBlocks);
  IO.mapRequired("DirectoryBlocks", Obj.DirectoryBlocks);
  IO.mapRequired("NumStreams", Obj.NumStreams);
  IO.mapRequired("FileSize", Obj.FileSize);
}

void MappingTraits<msf::SuperBlock>::mapping(IO &IO, msf::SuperBlock &SB) {
  if (!IO.outputting()) {
    ::memcpy(SB.MagicBytes, msf::Magic, sizeof(msf::Magic));
  }

  IO.mapRequired("BlockSize", SB.BlockSize);
  IO.mapRequired("FreeBlockMap", SB.FreeBlockMapBlock);
  IO.mapRequired("NumBlocks", SB.NumBlocks);
  IO.mapRequired("NumDirectoryBytes", SB.NumDirectoryBytes);
  IO.mapRequired("Unknown1", SB.Unknown1);
  IO.mapRequired("BlockMapAddr", SB.BlockMapAddr);
}

void MappingTraits<StreamBlockList>::mapping(IO &IO, StreamBlockList &SB) {
  IO.mapRequired("Stream", SB.Blocks);
}

void MappingTraits<PdbInfoStream>::mapping(IO &IO, PdbInfoStream &Obj) {
  IO.mapRequired("Age", Obj.Age);
  IO.mapRequired("Guid", Obj.Guid);
  IO.mapRequired("Signature", Obj.Signature);
  IO.mapRequired("Version", Obj.Version);
  IO.mapRequired("NamedStreams", Obj.NamedStreams);
}

void MappingTraits<PdbDbiStream>::mapping(IO &IO, PdbDbiStream &Obj) {
  IO.mapRequired("VerHeader", Obj.VerHeader);
  IO.mapRequired("Age", Obj.Age);
  IO.mapRequired("BuildNumber", Obj.BuildNumber);
  IO.mapRequired("PdbDllVersion", Obj.PdbDllVersion);
  IO.mapRequired("PdbDllRbld", Obj.PdbDllRbld);
  IO.mapRequired("Flags", Obj.Flags);
  IO.mapRequired("MachineType", Obj.MachineType);
}

void MappingTraits<NamedStreamMapping>::mapping(IO &IO,
                                                NamedStreamMapping &Obj) {
  IO.mapRequired("Name", Obj.StreamName);
  IO.mapRequired("StreamNum", Obj.StreamNumber);
}
