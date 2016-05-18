//===-- TypeDumper.cpp - CodeView type info dumper --------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/CodeView/TypeDumper.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/DebugInfo/CodeView/CVTypeVisitor.h"
#include "llvm/DebugInfo/CodeView/TypeIndex.h"
#include "llvm/DebugInfo/CodeView/TypeRecord.h"
#include "llvm/DebugInfo/CodeView/TypeStream.h"
#include "llvm/Support/ScopedPrinter.h"

using namespace llvm;
using namespace llvm::codeview;

/// The names here all end in "*". If the simple type is a pointer type, we
/// return the whole name. Otherwise we lop off the last character in our
/// StringRef.
static const EnumEntry<SimpleTypeKind> SimpleTypeNames[] = {
    {"void*", SimpleTypeKind::Void},
    {"<not translated>*", SimpleTypeKind::NotTranslated},
    {"HRESULT*", SimpleTypeKind::HResult},
    {"signed char*", SimpleTypeKind::SignedCharacter},
    {"unsigned char*", SimpleTypeKind::UnsignedCharacter},
    {"char*", SimpleTypeKind::NarrowCharacter},
    {"wchar_t*", SimpleTypeKind::WideCharacter},
    {"char16_t*", SimpleTypeKind::Character16},
    {"char32_t*", SimpleTypeKind::Character32},
    {"__int8*", SimpleTypeKind::SByte},
    {"unsigned __int8*", SimpleTypeKind::Byte},
    {"short*", SimpleTypeKind::Int16Short},
    {"unsigned short*", SimpleTypeKind::UInt16Short},
    {"__int16*", SimpleTypeKind::Int16},
    {"unsigned __int16*", SimpleTypeKind::UInt16},
    {"long*", SimpleTypeKind::Int32Long},
    {"unsigned long*", SimpleTypeKind::UInt32Long},
    {"int*", SimpleTypeKind::Int32},
    {"unsigned*", SimpleTypeKind::UInt32},
    {"__int64*", SimpleTypeKind::Int64Quad},
    {"unsigned __int64*", SimpleTypeKind::UInt64Quad},
    {"__int64*", SimpleTypeKind::Int64},
    {"unsigned __int64*", SimpleTypeKind::UInt64},
    {"__int128*", SimpleTypeKind::Int128},
    {"unsigned __int128*", SimpleTypeKind::UInt128},
    {"__half*", SimpleTypeKind::Float16},
    {"float*", SimpleTypeKind::Float32},
    {"float*", SimpleTypeKind::Float32PartialPrecision},
    {"__float48*", SimpleTypeKind::Float48},
    {"double*", SimpleTypeKind::Float64},
    {"long double*", SimpleTypeKind::Float80},
    {"__float128*", SimpleTypeKind::Float128},
    {"_Complex float*", SimpleTypeKind::Complex32},
    {"_Complex double*", SimpleTypeKind::Complex64},
    {"_Complex long double*", SimpleTypeKind::Complex80},
    {"_Complex __float128*", SimpleTypeKind::Complex128},
    {"bool*", SimpleTypeKind::Boolean8},
    {"__bool16*", SimpleTypeKind::Boolean16},
    {"__bool32*", SimpleTypeKind::Boolean32},
    {"__bool64*", SimpleTypeKind::Boolean64},
};

static const EnumEntry<TypeLeafKind> LeafTypeNames[] = {
#define CV_TYPE(enum, val) {#enum, enum},
#include "llvm/DebugInfo/CodeView/TypeRecords.def"
};

#define ENUM_ENTRY(enum_class, enum)                                           \
  { #enum, std::underlying_type < enum_class > ::type(enum_class::enum) }

static const EnumEntry<uint16_t> ClassOptionNames[] = {
    ENUM_ENTRY(ClassOptions, Packed),
    ENUM_ENTRY(ClassOptions, HasConstructorOrDestructor),
    ENUM_ENTRY(ClassOptions, HasOverloadedOperator),
    ENUM_ENTRY(ClassOptions, Nested),
    ENUM_ENTRY(ClassOptions, ContainsNestedClass),
    ENUM_ENTRY(ClassOptions, HasOverloadedAssignmentOperator),
    ENUM_ENTRY(ClassOptions, HasConversionOperator),
    ENUM_ENTRY(ClassOptions, ForwardReference),
    ENUM_ENTRY(ClassOptions, Scoped),
    ENUM_ENTRY(ClassOptions, HasUniqueName),
    ENUM_ENTRY(ClassOptions, Sealed),
    ENUM_ENTRY(ClassOptions, Intrinsic),
};

static const EnumEntry<uint8_t> MemberAccessNames[] = {
    ENUM_ENTRY(MemberAccess, None),
    ENUM_ENTRY(MemberAccess, Private),
    ENUM_ENTRY(MemberAccess, Protected),
    ENUM_ENTRY(MemberAccess, Public),
};

static const EnumEntry<uint16_t> MethodOptionNames[] = {
    ENUM_ENTRY(MethodOptions, Pseudo),
    ENUM_ENTRY(MethodOptions, NoInherit),
    ENUM_ENTRY(MethodOptions, NoConstruct),
    ENUM_ENTRY(MethodOptions, CompilerGenerated),
    ENUM_ENTRY(MethodOptions, Sealed),
};

static const EnumEntry<uint16_t> MemberKindNames[] = {
    ENUM_ENTRY(MethodKind, Vanilla),
    ENUM_ENTRY(MethodKind, Virtual),
    ENUM_ENTRY(MethodKind, Static),
    ENUM_ENTRY(MethodKind, Friend),
    ENUM_ENTRY(MethodKind, IntroducingVirtual),
    ENUM_ENTRY(MethodKind, PureVirtual),
    ENUM_ENTRY(MethodKind, PureIntroducingVirtual),
};

static const EnumEntry<uint8_t> PtrKindNames[] = {
    ENUM_ENTRY(PointerKind, Near16),
    ENUM_ENTRY(PointerKind, Far16),
    ENUM_ENTRY(PointerKind, Huge16),
    ENUM_ENTRY(PointerKind, BasedOnSegment),
    ENUM_ENTRY(PointerKind, BasedOnValue),
    ENUM_ENTRY(PointerKind, BasedOnSegmentValue),
    ENUM_ENTRY(PointerKind, BasedOnAddress),
    ENUM_ENTRY(PointerKind, BasedOnSegmentAddress),
    ENUM_ENTRY(PointerKind, BasedOnType),
    ENUM_ENTRY(PointerKind, BasedOnSelf),
    ENUM_ENTRY(PointerKind, Near32),
    ENUM_ENTRY(PointerKind, Far32),
    ENUM_ENTRY(PointerKind, Near64),
};

static const EnumEntry<uint8_t> PtrModeNames[] = {
    ENUM_ENTRY(PointerMode, Pointer),
    ENUM_ENTRY(PointerMode, LValueReference),
    ENUM_ENTRY(PointerMode, PointerToDataMember),
    ENUM_ENTRY(PointerMode, PointerToMemberFunction),
    ENUM_ENTRY(PointerMode, RValueReference),
};

static const EnumEntry<uint16_t> PtrMemberRepNames[] = {
    ENUM_ENTRY(PointerToMemberRepresentation, Unknown),
    ENUM_ENTRY(PointerToMemberRepresentation, SingleInheritanceData),
    ENUM_ENTRY(PointerToMemberRepresentation, MultipleInheritanceData),
    ENUM_ENTRY(PointerToMemberRepresentation, VirtualInheritanceData),
    ENUM_ENTRY(PointerToMemberRepresentation, GeneralData),
    ENUM_ENTRY(PointerToMemberRepresentation, SingleInheritanceFunction),
    ENUM_ENTRY(PointerToMemberRepresentation, MultipleInheritanceFunction),
    ENUM_ENTRY(PointerToMemberRepresentation, VirtualInheritanceFunction),
    ENUM_ENTRY(PointerToMemberRepresentation, GeneralFunction),
};

static const EnumEntry<uint16_t> TypeModifierNames[] = {
    ENUM_ENTRY(ModifierOptions, Const),
    ENUM_ENTRY(ModifierOptions, Volatile),
    ENUM_ENTRY(ModifierOptions, Unaligned),
};

static const EnumEntry<uint8_t> CallingConventions[] = {
    ENUM_ENTRY(CallingConvention, NearC),
    ENUM_ENTRY(CallingConvention, FarC),
    ENUM_ENTRY(CallingConvention, NearPascal),
    ENUM_ENTRY(CallingConvention, FarPascal),
    ENUM_ENTRY(CallingConvention, NearFast),
    ENUM_ENTRY(CallingConvention, FarFast),
    ENUM_ENTRY(CallingConvention, NearStdCall),
    ENUM_ENTRY(CallingConvention, FarStdCall),
    ENUM_ENTRY(CallingConvention, NearSysCall),
    ENUM_ENTRY(CallingConvention, FarSysCall),
    ENUM_ENTRY(CallingConvention, ThisCall),
    ENUM_ENTRY(CallingConvention, MipsCall),
    ENUM_ENTRY(CallingConvention, Generic),
    ENUM_ENTRY(CallingConvention, AlphaCall),
    ENUM_ENTRY(CallingConvention, PpcCall),
    ENUM_ENTRY(CallingConvention, SHCall),
    ENUM_ENTRY(CallingConvention, ArmCall),
    ENUM_ENTRY(CallingConvention, AM33Call),
    ENUM_ENTRY(CallingConvention, TriCall),
    ENUM_ENTRY(CallingConvention, SH5Call),
    ENUM_ENTRY(CallingConvention, M32RCall),
    ENUM_ENTRY(CallingConvention, ClrCall),
    ENUM_ENTRY(CallingConvention, Inline),
    ENUM_ENTRY(CallingConvention, NearVector),
};

static const EnumEntry<uint8_t> FunctionOptionEnum[] = {
    ENUM_ENTRY(FunctionOptions, CxxReturnUdt),
    ENUM_ENTRY(FunctionOptions, Constructor),
    ENUM_ENTRY(FunctionOptions, ConstructorWithVirtualBases),
};

#undef ENUM_ENTRY


namespace {

/// Use this private dumper implementation to keep implementation details about
/// the visitor out of TypeDumper.h.
class CVTypeDumperImpl : public CVTypeVisitor<CVTypeDumperImpl> {
public:
  CVTypeDumperImpl(CVTypeDumper &CVTD, ScopedPrinter &W, bool PrintRecordBytes)
      : CVTD(CVTD), W(W), PrintRecordBytes(PrintRecordBytes) {}

  /// CVTypeVisitor overrides.
#define TYPE_RECORD(EnumName, EnumVal, Name)                                   \
  void visit##Name(TypeLeafKind LeafType, Name##Record &Record);
#define TYPE_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#define MEMBER_RECORD(EnumName, EnumVal, Name)                                 \
  void visit##Name(TypeLeafKind LeafType, Name##Record &Record);
#define MEMBER_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#include "llvm/DebugInfo/CodeView/TypeRecords.def"

  void visitUnknownMember(TypeLeafKind Leaf);

  void visitTypeBegin(TypeLeafKind Leaf, ArrayRef<uint8_t> LeafData);
  void visitTypeEnd(TypeLeafKind Leaf, ArrayRef<uint8_t> LeafData);

  void printMemberAttributes(MemberAttributes Attrs);
  void printMemberAttributes(MemberAccess Access, MethodKind Kind,
                             MethodOptions Options);

private:
  /// Forwards to the dumper, which holds the persistent state from visitation.
  StringRef getTypeName(TypeIndex TI) {
    return CVTD.getTypeName(TI);
  }

  void printTypeIndex(StringRef FieldName, TypeIndex TI) {
    CVTD.printTypeIndex(FieldName, TI);
  }

  CVTypeDumper &CVTD;
  ScopedPrinter &W;
  bool PrintRecordBytes = false;

  /// Name of the current type. Only valid before visitTypeEnd.
  StringRef Name;
};

} // end anonymous namespace

static StringRef getLeafTypeName(TypeLeafKind LT) {
  switch (LT) {
#define TYPE_RECORD(ename, value, name)                                        \
  case ename:                                                                  \
    return #name;
#include "llvm/DebugInfo/CodeView/TypeRecords.def"
  default:
    break;
  }
  return "UnknownLeaf";
}

void CVTypeDumperImpl::visitTypeBegin(TypeLeafKind Leaf,
                                      ArrayRef<uint8_t> LeafData) {
  // Reset Name to the empty string. If the visitor sets it, we know it.
  Name = "";

  W.startLine() << getLeafTypeName(Leaf) << " ("
                << HexNumber(CVTD.getNextTypeIndex()) << ") {\n";
  W.indent();
  W.printEnum("TypeLeafKind", unsigned(Leaf), makeArrayRef(LeafTypeNames));
}

void CVTypeDumperImpl::visitTypeEnd(TypeLeafKind Leaf,
                                    ArrayRef<uint8_t> LeafData) {
  // Always record some name for every type, even if Name is empty. CVUDTNames
  // is indexed by type index, and must have one entry for every type.
  CVTD.recordType(Name);

  if (PrintRecordBytes)
    W.printBinaryBlock("LeafData", getBytesAsCharacters(LeafData));

  W.unindent();
  W.startLine() << "}\n";
}

void CVTypeDumperImpl::visitStringId(TypeLeafKind Leaf,
                                           StringIdRecord &String) {
  printTypeIndex("Id", String.getId());
  W.printString("StringData", String.getString());
  // Put this in CVUDTNames so it gets printed with LF_UDT_SRC_LINE.
  Name = String.getString();
}

void CVTypeDumperImpl::visitArgList(TypeLeafKind Leaf, ArgListRecord &Args) {
  auto Indices = Args.getIndices();
  uint32_t Size = Indices.size();
  W.printNumber("NumArgs", Size);
  ListScope Arguments(W, "Arguments");
  SmallString<256> TypeName("(");
  for (uint32_t I = 0; I < Size; ++I) {
    printTypeIndex("ArgType", Indices[I]);
    StringRef ArgTypeName = getTypeName(Indices[I]);
    TypeName.append(ArgTypeName);
    if (I + 1 != Size)
      TypeName.append(", ");
  }
  TypeName.push_back(')');
  Name = CVTD.saveName(TypeName);
}

void CVTypeDumperImpl::visitClass(TypeLeafKind Leaf, ClassRecord &Class) {
  uint16_t Props = static_cast<uint16_t>(Class.getOptions());
  W.printNumber("MemberCount", Class.getMemberCount());
  W.printFlags("Properties", Props, makeArrayRef(ClassOptionNames));
  printTypeIndex("FieldList", Class.getFieldList());
  printTypeIndex("DerivedFrom", Class.getDerivationList());
  printTypeIndex("VShape", Class.getVTableShape());
  W.printNumber("SizeOf", Class.getSize());
  W.printString("Name", Class.getName());
  if (Props & uint16_t(ClassOptions::HasUniqueName))
    W.printString("LinkageName", Class.getUniqueName());
  Name = Class.getName();
}

void CVTypeDumperImpl::visitUnion(TypeLeafKind Leaf, UnionRecord &Union) {
  uint16_t Props = static_cast<uint16_t>(Union.getOptions());
  W.printNumber("MemberCount", Union.getMemberCount());
  W.printFlags("Properties", Props, makeArrayRef(ClassOptionNames));
  printTypeIndex("FieldList", Union.getFieldList());
  W.printNumber("SizeOf", Union.getSize());
  W.printString("Name", Union.getName());
  if (Props & uint16_t(ClassOptions::HasUniqueName))
    W.printString("LinkageName", Union.getUniqueName());
  Name = Union.getName();
}

void CVTypeDumperImpl::visitEnum(TypeLeafKind Leaf, EnumRecord &Enum) {
  W.printNumber("NumEnumerators", Enum.getMemberCount());
  W.printFlags("Properties", uint16_t(Enum.getOptions()),
               makeArrayRef(ClassOptionNames));
  printTypeIndex("UnderlyingType", Enum.getUnderlyingType());
  printTypeIndex("FieldListType", Enum.getFieldList());
  W.printString("Name", Enum.getName());
  Name = Enum.getName();
}

void CVTypeDumperImpl::visitArray(TypeLeafKind Leaf, ArrayRecord &AT) {
  printTypeIndex("ElementType", AT.getElementType());
  printTypeIndex("IndexType", AT.getIndexType());
  W.printNumber("SizeOf", AT.getSize());
  W.printString("Name", AT.getName());
  Name = AT.getName();
}

void CVTypeDumperImpl::visitVFTable(TypeLeafKind Leaf, VFTableRecord &VFT) {
  printTypeIndex("CompleteClass", VFT.getCompleteClass());
  printTypeIndex("OverriddenVFTable", VFT.getOverriddenVTable());
  W.printHex("VFPtrOffset", VFT.getVFPtrOffset());
  W.printString("VFTableName", VFT.getName());
  for (auto N : VFT.getMethodNames())
    W.printString("MethodName", N);
  Name = VFT.getName();
}

void CVTypeDumperImpl::visitMemberFuncId(TypeLeafKind Leaf,
                                               MemberFuncIdRecord &Id) {
  printTypeIndex("ClassType", Id.getClassType());
  printTypeIndex("FunctionType", Id.getFunctionType());
  W.printString("Name", Id.getName());
  Name = Id.getName();
}

void CVTypeDumperImpl::visitProcedure(TypeLeafKind Leaf,
                                      ProcedureRecord &Proc) {
  printTypeIndex("ReturnType", Proc.getReturnType());
  W.printEnum("CallingConvention", uint8_t(Proc.getCallConv()),
              makeArrayRef(CallingConventions));
  W.printFlags("FunctionOptions", uint8_t(Proc.getOptions()),
               makeArrayRef(FunctionOptionEnum));
  W.printNumber("NumParameters", Proc.getParameterCount());
  printTypeIndex("ArgListType", Proc.getArgumentList());

  StringRef ReturnTypeName = getTypeName(Proc.getReturnType());
  StringRef ArgListTypeName = getTypeName(Proc.getArgumentList());
  SmallString<256> TypeName(ReturnTypeName);
  TypeName.push_back(' ');
  TypeName.append(ArgListTypeName);
  Name = CVTD.saveName(TypeName);
}

void CVTypeDumperImpl::visitMemberFunction(TypeLeafKind Leaf,
                                           MemberFunctionRecord &MF) {
  printTypeIndex("ReturnType", MF.getReturnType());
  printTypeIndex("ClassType", MF.getClassType());
  printTypeIndex("ThisType", MF.getThisType());
  W.printEnum("CallingConvention", uint8_t(MF.getCallConv()),
              makeArrayRef(CallingConventions));
  W.printFlags("FunctionOptions", uint8_t(MF.getOptions()),
               makeArrayRef(FunctionOptionEnum));
  W.printNumber("NumParameters", MF.getParameterCount());
  printTypeIndex("ArgListType", MF.getArgumentList());
  W.printNumber("ThisAdjustment", MF.getThisPointerAdjustment());

  StringRef ReturnTypeName = getTypeName(MF.getReturnType());
  StringRef ClassTypeName = getTypeName(MF.getClassType());
  StringRef ArgListTypeName = getTypeName(MF.getArgumentList());
  SmallString<256> TypeName(ReturnTypeName);
  TypeName.push_back(' ');
  TypeName.append(ClassTypeName);
  TypeName.append("::");
  TypeName.append(ArgListTypeName);
  Name = CVTD.saveName(TypeName);
}

void CVTypeDumperImpl::visitMethodOverloadList(
    TypeLeafKind Leaf, MethodOverloadListRecord &MethodList) {
  for (auto &M : MethodList.getMethods()) {
    ListScope S(W, "Method");
    printMemberAttributes(M.getAccess(), M.getKind(), M.getOptions());
    printTypeIndex("Type", M.getType());
    if (M.isIntroducingVirtual())
      W.printHex("VFTableOffset", M.getVFTableOffset());
  }
}

void CVTypeDumperImpl::visitFuncId(TypeLeafKind Leaf, FuncIdRecord &Func) {
  printTypeIndex("ParentScope", Func.getParentScope());
  printTypeIndex("FunctionType", Func.getFunctionType());
  W.printString("Name", Func.getName());
  Name = Func.getName();
}

void CVTypeDumperImpl::visitTypeServer2(TypeLeafKind Leaf,
                                        TypeServer2Record &TS) {
  W.printBinary("Signature", TS.getGuid());
  W.printNumber("Age", TS.getAge());
  W.printString("Name", TS.getName());
  Name = TS.getName();
}

void CVTypeDumperImpl::visitPointer(TypeLeafKind Leaf, PointerRecord &Ptr) {
  printTypeIndex("PointeeType", Ptr.getReferentType());
  W.printHex("PointerAttributes", uint32_t(Ptr.getOptions()));
  W.printEnum("PtrType", unsigned(Ptr.getPointerKind()),
              makeArrayRef(PtrKindNames));
  W.printEnum("PtrMode", unsigned(Ptr.getMode()), makeArrayRef(PtrModeNames));

  W.printNumber("IsFlat", Ptr.isFlat());
  W.printNumber("IsConst", Ptr.isConst());
  W.printNumber("IsVolatile", Ptr.isVolatile());
  W.printNumber("IsUnaligned", Ptr.isUnaligned());

  if (Ptr.isPointerToMember()) {
    const MemberPointerInfo &MI = Ptr.getMemberInfo();

    printTypeIndex("ClassType", MI.getContainingType());
    W.printEnum("Representation", uint16_t(MI.getRepresentation()),
                makeArrayRef(PtrMemberRepNames));

    StringRef PointeeName = getTypeName(Ptr.getReferentType());
    StringRef ClassName = getTypeName(MI.getContainingType());
    SmallString<256> TypeName(PointeeName);
    TypeName.push_back(' ');
    TypeName.append(ClassName);
    TypeName.append("::*");
    Name = CVTD.saveName(TypeName);
  } else {
    SmallString<256> TypeName;
    if (Ptr.isConst())
      TypeName.append("const ");
    if (Ptr.isVolatile())
      TypeName.append("volatile ");
    if (Ptr.isUnaligned())
      TypeName.append("__unaligned ");

    TypeName.append(getTypeName(Ptr.getReferentType()));

    if (Ptr.getMode() == PointerMode::LValueReference)
      TypeName.append("&");
    else if (Ptr.getMode() == PointerMode::RValueReference)
      TypeName.append("&&");
    else if (Ptr.getMode() == PointerMode::Pointer)
      TypeName.append("*");

    Name = CVTD.saveName(TypeName);
  }
}

void CVTypeDumperImpl::visitModifier(TypeLeafKind Leaf, ModifierRecord &Mod) {
  uint16_t Mods = static_cast<uint16_t>(Mod.getModifiers());
  printTypeIndex("ModifiedType", Mod.getModifiedType());
  W.printFlags("Modifiers", Mods, makeArrayRef(TypeModifierNames));

  StringRef ModifiedName = getTypeName(Mod.getModifiedType());
  SmallString<256> TypeName;
  if (Mods & uint16_t(ModifierOptions::Const))
    TypeName.append("const ");
  if (Mods & uint16_t(ModifierOptions::Volatile))
    TypeName.append("volatile ");
  if (Mods & uint16_t(ModifierOptions::Unaligned))
    TypeName.append("__unaligned ");
  TypeName.append(ModifiedName);
  Name = CVTD.saveName(TypeName);
}

void CVTypeDumperImpl::visitVFTableShape(TypeLeafKind Leaf,
                                         VFTableShapeRecord &Shape) {
  W.printNumber("VFEntryCount", Shape.getEntryCount());
}

void CVTypeDumperImpl::visitUdtSourceLine(TypeLeafKind Leaf,
                                          UdtSourceLineRecord &Line) {
  printTypeIndex("UDT", Line.getUDT());
  printTypeIndex("SourceFile", Line.getSourceFile());
  W.printNumber("LineNumber", Line.getLineNumber());
}

void CVTypeDumperImpl::visitBuildInfo(TypeLeafKind Leaf,
                                      BuildInfoRecord &Args) {
  W.printNumber("NumArgs", static_cast<uint32_t>(Args.getArgs().size()));

  ListScope Arguments(W, "Arguments");
  for (auto Arg : Args.getArgs()) {
    printTypeIndex("ArgType", Arg);
  }
}

void CVTypeDumperImpl::printMemberAttributes(MemberAttributes Attrs) {
  return printMemberAttributes(Attrs.getAccess(), Attrs.getMethodKind(),
                               Attrs.getFlags());
}

void CVTypeDumperImpl::printMemberAttributes(MemberAccess Access,
                                             MethodKind Kind,
                                             MethodOptions Options) {
  W.printEnum("AccessSpecifier", uint8_t(Access),
              makeArrayRef(MemberAccessNames));
  // Data members will be vanilla. Don't try to print a method kind for them.
  if (Kind != MethodKind::Vanilla)
    W.printEnum("MethodKind", unsigned(Kind), makeArrayRef(MemberKindNames));
  if (Options != MethodOptions::None) {
    W.printFlags("MethodOptions", unsigned(Options),
                 makeArrayRef(MethodOptionNames));
  }
}

void CVTypeDumperImpl::visitUnknownMember(TypeLeafKind Leaf) {
  W.printHex("UnknownMember", unsigned(Leaf));
}

void CVTypeDumperImpl::visitNestedType(TypeLeafKind Leaf,
                                       NestedTypeRecord &Nested) {
  DictScope S(W, "NestedType");
  printTypeIndex("Type", Nested.getNestedType());
  W.printString("Name", Nested.getName());
  Name = Nested.getName();
}

void CVTypeDumperImpl::visitOneMethod(TypeLeafKind Leaf,
                                      OneMethodRecord &Method) {
  DictScope S(W, "OneMethod");
  MethodKind K = Method.getKind();
  printMemberAttributes(Method.getAccess(), K, Method.getOptions());
  printTypeIndex("Type", Method.getType());
  // If virtual, then read the vftable offset.
  if (Method.isIntroducingVirtual())
    W.printHex("VFTableOffset", Method.getVFTableOffset());
  W.printString("Name", Method.getName());
  Name = Method.getName();
}

void CVTypeDumperImpl::visitOverloadedMethod(TypeLeafKind Leaf,
                                             OverloadedMethodRecord &Method) {
  DictScope S(W, "OverloadedMethod");
  W.printHex("MethodCount", Method.getNumOverloads());
  printTypeIndex("MethodListIndex", Method.getMethodList());
  W.printString("Name", Method.getName());
  Name = Method.getName();
}

void CVTypeDumperImpl::visitDataMember(TypeLeafKind Leaf,
                                       DataMemberRecord &Field) {
  DictScope S(W, "DataMember");
  printMemberAttributes(Field.getAccess(), MethodKind::Vanilla,
                        MethodOptions::None);
  printTypeIndex("Type", Field.getType());
  W.printHex("FieldOffset", Field.getFieldOffset());
  W.printString("Name", Field.getName());
  Name = Field.getName();
}

void CVTypeDumperImpl::visitStaticDataMember(TypeLeafKind Leaf,
                                             StaticDataMemberRecord &Field) {
  DictScope S(W, "StaticDataMember");
  printMemberAttributes(Field.getAccess(), MethodKind::Vanilla,
                        MethodOptions::None);
  printTypeIndex("Type", Field.getType());
  W.printString("Name", Field.getName());
  Name = Field.getName();
}

void CVTypeDumperImpl::visitVFPtr(TypeLeafKind Leaf, VFPtrRecord &VFTable) {
  DictScope S(W, "VFPtr");
  printTypeIndex("Type", VFTable.getType());
}

void CVTypeDumperImpl::visitEnumerator(TypeLeafKind Leaf,
                                       EnumeratorRecord &Enum) {
  DictScope S(W, "Enumerator");
  printMemberAttributes(Enum.getAccess(), MethodKind::Vanilla,
                        MethodOptions::None);
  W.printNumber("EnumValue", Enum.getValue());
  W.printString("Name", Enum.getName());
  Name = Enum.getName();
}

void CVTypeDumperImpl::visitBaseClass(TypeLeafKind Leaf,
                                      BaseClassRecord &Base) {
  DictScope S(W, "BaseClass");
  printMemberAttributes(Base.getAccess(), MethodKind::Vanilla,
                        MethodOptions::None);
  printTypeIndex("BaseType", Base.getBaseType());
  W.printHex("BaseOffset", Base.getBaseOffset());
}

void CVTypeDumperImpl::visitVirtualBaseClass(TypeLeafKind Leaf,
                                             VirtualBaseClassRecord &Base) {
  DictScope S(W, "VirtualBaseClass");
  printMemberAttributes(Base.getAccess(), MethodKind::Vanilla,
                        MethodOptions::None);
  printTypeIndex("BaseType", Base.getBaseType());
  printTypeIndex("VBPtrType", Base.getVBPtrType());
  W.printHex("VBPtrOffset", Base.getVBPtrOffset());
  W.printHex("VBTableIndex", Base.getVTableIndex());
}

StringRef CVTypeDumper::getTypeName(TypeIndex TI) {
  if (TI.isNoType())
    return "<no type>";

  if (TI.isSimple()) {
    // This is a simple type.
    for (const auto &SimpleTypeName : SimpleTypeNames) {
      if (SimpleTypeName.Value == TI.getSimpleKind()) {
        if (TI.getSimpleMode() == SimpleTypeMode::Direct)
          return SimpleTypeName.Name.drop_back(1);
        // Otherwise, this is a pointer type. We gloss over the distinction
        // between near, far, 64, 32, etc, and just give a pointer type.
        return SimpleTypeName.Name;
      }
    }
    return "<unknown simple type>";
  }

  // User-defined type.
  StringRef UDTName;
  unsigned UDTIndex = TI.getIndex() - 0x1000;
  if (UDTIndex < CVUDTNames.size())
    return CVUDTNames[UDTIndex];

  return "<unknown UDT>";
}

void CVTypeDumper::printTypeIndex(StringRef FieldName, TypeIndex TI) {
  StringRef TypeName;
  if (!TI.isNoType())
    TypeName = getTypeName(TI);
  if (!TypeName.empty())
    W.printHex(FieldName, TypeName, TI.getIndex());
  else
    W.printHex(FieldName, TI.getIndex());
}

bool CVTypeDumper::dump(const TypeIterator::Record &Record) {
  CVTypeDumperImpl Dumper(*this, W, PrintRecordBytes);
  Dumper.visitTypeRecord(Record);
  return !Dumper.hadError();
}

bool CVTypeDumper::dump(ArrayRef<uint8_t> Data) {
  CVTypeDumperImpl Dumper(*this, W, PrintRecordBytes);
  Dumper.visitTypeStream(Data);
  return !Dumper.hadError();
}
