//===-- LowerTypeTests.cpp - type metadata lowering pass ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass lowers type metadata and calls to the llvm.type.test intrinsic.
// See http://llvm.org/docs/TypeMetadata.html for more information.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/IPO/LowerTypeTests.h"
#include "llvm/ADT/EquivalenceClasses.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalObject.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

using namespace llvm;
using namespace lowertypetests;

#define DEBUG_TYPE "lowertypetests"

STATISTIC(ByteArraySizeBits, "Byte array size in bits");
STATISTIC(ByteArraySizeBytes, "Byte array size in bytes");
STATISTIC(NumByteArraysCreated, "Number of byte arrays created");
STATISTIC(NumTypeTestCallsLowered, "Number of type test calls lowered");
STATISTIC(NumTypeIdDisjointSets, "Number of disjoint sets of type identifiers");

static cl::opt<bool> AvoidReuse(
    "lowertypetests-avoid-reuse",
    cl::desc("Try to avoid reuse of byte array addresses using aliases"),
    cl::Hidden, cl::init(true));

bool BitSetInfo::containsGlobalOffset(uint64_t Offset) const {
  if (Offset < ByteOffset)
    return false;

  if ((Offset - ByteOffset) % (uint64_t(1) << AlignLog2) != 0)
    return false;

  uint64_t BitOffset = (Offset - ByteOffset) >> AlignLog2;
  if (BitOffset >= BitSize)
    return false;

  return Bits.count(BitOffset);
}

bool BitSetInfo::containsValue(
    const DataLayout &DL,
    const DenseMap<GlobalObject *, uint64_t> &GlobalLayout, Value *V,
    uint64_t COffset) const {
  if (auto GV = dyn_cast<GlobalObject>(V)) {
    auto I = GlobalLayout.find(GV);
    if (I == GlobalLayout.end())
      return false;
    return containsGlobalOffset(I->second + COffset);
  }

  if (auto GEP = dyn_cast<GEPOperator>(V)) {
    APInt APOffset(DL.getPointerSizeInBits(0), 0);
    bool Result = GEP->accumulateConstantOffset(DL, APOffset);
    if (!Result)
      return false;
    COffset += APOffset.getZExtValue();
    return containsValue(DL, GlobalLayout, GEP->getPointerOperand(), COffset);
  }

  if (auto Op = dyn_cast<Operator>(V)) {
    if (Op->getOpcode() == Instruction::BitCast)
      return containsValue(DL, GlobalLayout, Op->getOperand(0), COffset);

    if (Op->getOpcode() == Instruction::Select)
      return containsValue(DL, GlobalLayout, Op->getOperand(1), COffset) &&
             containsValue(DL, GlobalLayout, Op->getOperand(2), COffset);
  }

  return false;
}

void BitSetInfo::print(raw_ostream &OS) const {
  OS << "offset " << ByteOffset << " size " << BitSize << " align "
     << (1 << AlignLog2);

  if (isAllOnes()) {
    OS << " all-ones\n";
    return;
  }

  OS << " { ";
  for (uint64_t B : Bits)
    OS << B << ' ';
  OS << "}\n";
}

BitSetInfo BitSetBuilder::build() {
  if (Min > Max)
    Min = 0;

  // Normalize each offset against the minimum observed offset, and compute
  // the bitwise OR of each of the offsets. The number of trailing zeros
  // in the mask gives us the log2 of the alignment of all offsets, which
  // allows us to compress the bitset by only storing one bit per aligned
  // address.
  uint64_t Mask = 0;
  for (uint64_t &Offset : Offsets) {
    Offset -= Min;
    Mask |= Offset;
  }

  BitSetInfo BSI;
  BSI.ByteOffset = Min;

  BSI.AlignLog2 = 0;
  if (Mask != 0)
    BSI.AlignLog2 = countTrailingZeros(Mask, ZB_Undefined);

  // Build the compressed bitset while normalizing the offsets against the
  // computed alignment.
  BSI.BitSize = ((Max - Min) >> BSI.AlignLog2) + 1;
  for (uint64_t Offset : Offsets) {
    Offset >>= BSI.AlignLog2;
    BSI.Bits.insert(Offset);
  }

  return BSI;
}

void GlobalLayoutBuilder::addFragment(const std::set<uint64_t> &F) {
  // Create a new fragment to hold the layout for F.
  Fragments.emplace_back();
  std::vector<uint64_t> &Fragment = Fragments.back();
  uint64_t FragmentIndex = Fragments.size() - 1;

  for (auto ObjIndex : F) {
    uint64_t OldFragmentIndex = FragmentMap[ObjIndex];
    if (OldFragmentIndex == 0) {
      // We haven't seen this object index before, so just add it to the current
      // fragment.
      Fragment.push_back(ObjIndex);
    } else {
      // This index belongs to an existing fragment. Copy the elements of the
      // old fragment into this one and clear the old fragment. We don't update
      // the fragment map just yet, this ensures that any further references to
      // indices from the old fragment in this fragment do not insert any more
      // indices.
      std::vector<uint64_t> &OldFragment = Fragments[OldFragmentIndex];
      Fragment.insert(Fragment.end(), OldFragment.begin(), OldFragment.end());
      OldFragment.clear();
    }
  }

  // Update the fragment map to point our object indices to this fragment.
  for (uint64_t ObjIndex : Fragment)
    FragmentMap[ObjIndex] = FragmentIndex;
}

void ByteArrayBuilder::allocate(const std::set<uint64_t> &Bits,
                                uint64_t BitSize, uint64_t &AllocByteOffset,
                                uint8_t &AllocMask) {
  // Find the smallest current allocation.
  unsigned Bit = 0;
  for (unsigned I = 1; I != BitsPerByte; ++I)
    if (BitAllocs[I] < BitAllocs[Bit])
      Bit = I;

  AllocByteOffset = BitAllocs[Bit];

  // Add our size to it.
  unsigned ReqSize = AllocByteOffset + BitSize;
  BitAllocs[Bit] = ReqSize;
  if (Bytes.size() < ReqSize)
    Bytes.resize(ReqSize);

  // Set our bits.
  AllocMask = 1 << Bit;
  for (uint64_t B : Bits)
    Bytes[AllocByteOffset + B] |= AllocMask;
}

namespace {

struct ByteArrayInfo {
  std::set<uint64_t> Bits;
  uint64_t BitSize;
  GlobalVariable *ByteArray;
  Constant *Mask;
};

class LowerTypeTestsModule {
  Module &M;

  bool LinkerSubsectionsViaSymbols;
  Triple::ArchType Arch;
  Triple::ObjectFormatType ObjectFormat;

  IntegerType *Int1Ty = Type::getInt1Ty(M.getContext());
  IntegerType *Int8Ty = Type::getInt8Ty(M.getContext());
  IntegerType *Int32Ty = Type::getInt32Ty(M.getContext());
  PointerType *Int32PtrTy = PointerType::getUnqual(Int32Ty);
  IntegerType *Int64Ty = Type::getInt64Ty(M.getContext());
  IntegerType *IntPtrTy = M.getDataLayout().getIntPtrType(M.getContext(), 0);

  // Indirect function call index assignment counter for WebAssembly
  uint64_t IndirectIndex = 1;

  // Mapping from type identifiers to the call sites that test them.
  DenseMap<Metadata *, std::vector<CallInst *>> TypeTestCallSites;

  std::vector<ByteArrayInfo> ByteArrayInfos;

  Mangler Mang;
  Function *WeakInitializerFn = nullptr;

  BitSetInfo
  buildBitSet(Metadata *TypeId,
              const DenseMap<GlobalObject *, uint64_t> &GlobalLayout);
  ByteArrayInfo *createByteArray(BitSetInfo &BSI);
  void allocateByteArrays();
  Value *createBitSetTest(IRBuilder<> &B, BitSetInfo &BSI, ByteArrayInfo *&BAI,
                          Value *BitOffset);
  void
  lowerTypeTestCalls(ArrayRef<Metadata *> TypeIds, Constant *CombinedGlobalAddr,
                     const DenseMap<GlobalObject *, uint64_t> &GlobalLayout);
  Value *
  lowerBitSetCall(CallInst *CI, BitSetInfo &BSI, ByteArrayInfo *&BAI,
                  Constant *CombinedGlobal,
                  const DenseMap<GlobalObject *, uint64_t> &GlobalLayout);
  void buildBitSetsFromGlobalVariables(ArrayRef<Metadata *> TypeIds,
                                       ArrayRef<GlobalVariable *> Globals);
  unsigned getJumpTableEntrySize();
  Type *getJumpTableEntryType();
  void createJumpTableEntry(raw_ostream &OS, Function *Dest, unsigned Distance);
  void createJumpTableAlias(raw_ostream &OS, Function *Dest,
                            GlobalVariable *JumpTable, unsigned Distance);
  void verifyTypeMDNode(GlobalObject *GO, MDNode *Type);
  void buildBitSetsFromFunctions(ArrayRef<Metadata *> TypeIds,
                                 ArrayRef<Function *> Functions);
  void buildBitSetsFromFunctionsNative(ArrayRef<Metadata *> TypeIds,
                                    ArrayRef<Function *> Functions);
  void buildBitSetsFromFunctionsWASM(ArrayRef<Metadata *> TypeIds,
                                     ArrayRef<Function *> Functions);
  void buildBitSetsFromDisjointSet(ArrayRef<Metadata *> TypeIds,
                                   ArrayRef<GlobalObject *> Globals);

  void replaceWeakDeclarationWithJumpTablePtr(Function *F, Constant *JT);
  void moveInitializerToModuleConstructor(GlobalVariable *GV);
  void findGlobalVariableUsersOf(Constant *C,
                                 SmallSetVector<GlobalVariable *, 8> &Out);

public:
  LowerTypeTestsModule(Module &M);
  bool lower();
};

struct LowerTypeTests : public ModulePass {
  static char ID;
  LowerTypeTests() : ModulePass(ID) {
    initializeLowerTypeTestsPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) override {
    if (skipModule(M))
      return false;
    return LowerTypeTestsModule(M).lower();
  }
};

} // anonymous namespace

INITIALIZE_PASS(LowerTypeTests, "lowertypetests", "Lower type metadata", false,
                false)
char LowerTypeTests::ID = 0;

ModulePass *llvm::createLowerTypeTestsPass() { return new LowerTypeTests; }

/// Build a bit set for TypeId using the object layouts in
/// GlobalLayout.
BitSetInfo LowerTypeTestsModule::buildBitSet(
    Metadata *TypeId, const DenseMap<GlobalObject *, uint64_t> &GlobalLayout) {
  BitSetBuilder BSB;

  // Compute the byte offset of each address associated with this type
  // identifier.
  SmallVector<MDNode *, 2> Types;
  for (auto &GlobalAndOffset : GlobalLayout) {
    Types.clear();
    GlobalAndOffset.first->getMetadata(LLVMContext::MD_type, Types);
    for (MDNode *Type : Types) {
      if (Type->getOperand(1) != TypeId)
        continue;
      uint64_t Offset =
          cast<ConstantInt>(
              cast<ConstantAsMetadata>(Type->getOperand(0))->getValue())
              ->getZExtValue();
      BSB.addOffset(GlobalAndOffset.second + Offset);
    }
  }

  return BSB.build();
}

/// Build a test that bit BitOffset mod sizeof(Bits)*8 is set in
/// Bits. This pattern matches to the bt instruction on x86.
static Value *createMaskedBitTest(IRBuilder<> &B, Value *Bits,
                                  Value *BitOffset) {
  auto BitsType = cast<IntegerType>(Bits->getType());
  unsigned BitWidth = BitsType->getBitWidth();

  BitOffset = B.CreateZExtOrTrunc(BitOffset, BitsType);
  Value *BitIndex =
      B.CreateAnd(BitOffset, ConstantInt::get(BitsType, BitWidth - 1));
  Value *BitMask = B.CreateShl(ConstantInt::get(BitsType, 1), BitIndex);
  Value *MaskedBits = B.CreateAnd(Bits, BitMask);
  return B.CreateICmpNE(MaskedBits, ConstantInt::get(BitsType, 0));
}

ByteArrayInfo *LowerTypeTestsModule::createByteArray(BitSetInfo &BSI) {
  // Create globals to stand in for byte arrays and masks. These never actually
  // get initialized, we RAUW and erase them later in allocateByteArrays() once
  // we know the offset and mask to use.
  auto ByteArrayGlobal = new GlobalVariable(
      M, Int8Ty, /*isConstant=*/true, GlobalValue::PrivateLinkage, nullptr);
  auto MaskGlobal = new GlobalVariable(M, Int8Ty, /*isConstant=*/true,
                                       GlobalValue::PrivateLinkage, nullptr);

  ByteArrayInfos.emplace_back();
  ByteArrayInfo *BAI = &ByteArrayInfos.back();

  BAI->Bits = BSI.Bits;
  BAI->BitSize = BSI.BitSize;
  BAI->ByteArray = ByteArrayGlobal;
  BAI->Mask = ConstantExpr::getPtrToInt(MaskGlobal, Int8Ty);
  return BAI;
}

void LowerTypeTestsModule::allocateByteArrays() {
  std::stable_sort(ByteArrayInfos.begin(), ByteArrayInfos.end(),
                   [](const ByteArrayInfo &BAI1, const ByteArrayInfo &BAI2) {
                     return BAI1.BitSize > BAI2.BitSize;
                   });

  std::vector<uint64_t> ByteArrayOffsets(ByteArrayInfos.size());

  ByteArrayBuilder BAB;
  for (unsigned I = 0; I != ByteArrayInfos.size(); ++I) {
    ByteArrayInfo *BAI = &ByteArrayInfos[I];

    uint8_t Mask;
    BAB.allocate(BAI->Bits, BAI->BitSize, ByteArrayOffsets[I], Mask);

    BAI->Mask->replaceAllUsesWith(ConstantInt::get(Int8Ty, Mask));
    cast<GlobalVariable>(BAI->Mask->getOperand(0))->eraseFromParent();
  }

  Constant *ByteArrayConst = ConstantDataArray::get(M.getContext(), BAB.Bytes);
  auto ByteArray =
      new GlobalVariable(M, ByteArrayConst->getType(), /*isConstant=*/true,
                         GlobalValue::PrivateLinkage, ByteArrayConst);

  for (unsigned I = 0; I != ByteArrayInfos.size(); ++I) {
    ByteArrayInfo *BAI = &ByteArrayInfos[I];

    Constant *Idxs[] = {ConstantInt::get(IntPtrTy, 0),
                        ConstantInt::get(IntPtrTy, ByteArrayOffsets[I])};
    Constant *GEP = ConstantExpr::getInBoundsGetElementPtr(
        ByteArrayConst->getType(), ByteArray, Idxs);

    // Create an alias instead of RAUW'ing the gep directly. On x86 this ensures
    // that the pc-relative displacement is folded into the lea instead of the
    // test instruction getting another displacement.
    if (LinkerSubsectionsViaSymbols) {
      BAI->ByteArray->replaceAllUsesWith(GEP);
    } else {
      GlobalAlias *Alias = GlobalAlias::create(
          Int8Ty, 0, GlobalValue::PrivateLinkage, "bits", GEP, &M);
      BAI->ByteArray->replaceAllUsesWith(Alias);
    }
    BAI->ByteArray->eraseFromParent();
  }

  ByteArraySizeBits = BAB.BitAllocs[0] + BAB.BitAllocs[1] + BAB.BitAllocs[2] +
                      BAB.BitAllocs[3] + BAB.BitAllocs[4] + BAB.BitAllocs[5] +
                      BAB.BitAllocs[6] + BAB.BitAllocs[7];
  ByteArraySizeBytes = BAB.Bytes.size();
}

/// Build a test that bit BitOffset is set in BSI, where
/// BitSetGlobal is a global containing the bits in BSI.
Value *LowerTypeTestsModule::createBitSetTest(IRBuilder<> &B, BitSetInfo &BSI,
                                              ByteArrayInfo *&BAI,
                                              Value *BitOffset) {
  if (BSI.BitSize <= 64) {
    // If the bit set is sufficiently small, we can avoid a load by bit testing
    // a constant.
    IntegerType *BitsTy;
    if (BSI.BitSize <= 32)
      BitsTy = Int32Ty;
    else
      BitsTy = Int64Ty;

    uint64_t Bits = 0;
    for (auto Bit : BSI.Bits)
      Bits |= uint64_t(1) << Bit;
    Constant *BitsConst = ConstantInt::get(BitsTy, Bits);
    return createMaskedBitTest(B, BitsConst, BitOffset);
  } else {
    if (!BAI) {
      ++NumByteArraysCreated;
      BAI = createByteArray(BSI);
    }

    Constant *ByteArray = BAI->ByteArray;
    Type *Ty = BAI->ByteArray->getValueType();
    if (!LinkerSubsectionsViaSymbols && AvoidReuse) {
      // Each use of the byte array uses a different alias. This makes the
      // backend less likely to reuse previously computed byte array addresses,
      // improving the security of the CFI mechanism based on this pass.
      ByteArray = GlobalAlias::create(BAI->ByteArray->getValueType(), 0,
                                      GlobalValue::PrivateLinkage, "bits_use",
                                      ByteArray, &M);
    }

    Value *ByteAddr = B.CreateGEP(Ty, ByteArray, BitOffset);
    Value *Byte = B.CreateLoad(ByteAddr);

    Value *ByteAndMask = B.CreateAnd(Byte, BAI->Mask);
    return B.CreateICmpNE(ByteAndMask, ConstantInt::get(Int8Ty, 0));
  }
}

/// Lower a llvm.type.test call to its implementation. Returns the value to
/// replace the call with.
Value *LowerTypeTestsModule::lowerBitSetCall(
    CallInst *CI, BitSetInfo &BSI, ByteArrayInfo *&BAI,
    Constant *CombinedGlobalIntAddr,
    const DenseMap<GlobalObject *, uint64_t> &GlobalLayout) {
  Value *Ptr = CI->getArgOperand(0);
  const DataLayout &DL = M.getDataLayout();

  if (BSI.containsValue(DL, GlobalLayout, Ptr))
    return ConstantInt::getTrue(M.getContext());

  Constant *OffsetedGlobalAsInt = ConstantExpr::getAdd(
      CombinedGlobalIntAddr, ConstantInt::get(IntPtrTy, BSI.ByteOffset));

  BasicBlock *InitialBB = CI->getParent();

  IRBuilder<> B(CI);

  Value *PtrAsInt = B.CreatePtrToInt(Ptr, IntPtrTy);

  if (BSI.isSingleOffset())
    return B.CreateICmpEQ(PtrAsInt, OffsetedGlobalAsInt);

  Value *PtrOffset = B.CreateSub(PtrAsInt, OffsetedGlobalAsInt);

  Value *BitOffset;
  if (BSI.AlignLog2 == 0) {
    BitOffset = PtrOffset;
  } else {
    // We need to check that the offset both falls within our range and is
    // suitably aligned. We can check both properties at the same time by
    // performing a right rotate by log2(alignment) followed by an integer
    // comparison against the bitset size. The rotate will move the lower
    // order bits that need to be zero into the higher order bits of the
    // result, causing the comparison to fail if they are nonzero. The rotate
    // also conveniently gives us a bit offset to use during the load from
    // the bitset.
    Value *OffsetSHR =
        B.CreateLShr(PtrOffset, ConstantInt::get(IntPtrTy, BSI.AlignLog2));
    Value *OffsetSHL = B.CreateShl(
        PtrOffset,
        ConstantInt::get(IntPtrTy, DL.getPointerSizeInBits(0) - BSI.AlignLog2));
    BitOffset = B.CreateOr(OffsetSHR, OffsetSHL);
  }

  Constant *BitSizeConst = ConstantInt::get(IntPtrTy, BSI.BitSize);
  Value *OffsetInRange = B.CreateICmpULT(BitOffset, BitSizeConst);

  // If the bit set is all ones, testing against it is unnecessary.
  if (BSI.isAllOnes())
    return OffsetInRange;

  TerminatorInst *Term = SplitBlockAndInsertIfThen(OffsetInRange, CI, false);
  IRBuilder<> ThenB(Term);

  // Now that we know that the offset is in range and aligned, load the
  // appropriate bit from the bitset.
  Value *Bit = createBitSetTest(ThenB, BSI, BAI, BitOffset);

  // The value we want is 0 if we came directly from the initial block
  // (having failed the range or alignment checks), or the loaded bit if
  // we came from the block in which we loaded it.
  B.SetInsertPoint(CI);
  PHINode *P = B.CreatePHI(Int1Ty, 2);
  P->addIncoming(ConstantInt::get(Int1Ty, 0), InitialBB);
  P->addIncoming(Bit, ThenB.GetInsertBlock());
  return P;
}

/// Given a disjoint set of type identifiers and globals, lay out the globals,
/// build the bit sets and lower the llvm.type.test calls.
void LowerTypeTestsModule::buildBitSetsFromGlobalVariables(
    ArrayRef<Metadata *> TypeIds, ArrayRef<GlobalVariable *> Globals) {
  // Build a new global with the combined contents of the referenced globals.
  // This global is a struct whose even-indexed elements contain the original
  // contents of the referenced globals and whose odd-indexed elements contain
  // any padding required to align the next element to the next power of 2.
  std::vector<Constant *> GlobalInits;
  const DataLayout &DL = M.getDataLayout();
  for (GlobalVariable *G : Globals) {
    GlobalInits.push_back(G->getInitializer());
    uint64_t InitSize = DL.getTypeAllocSize(G->getValueType());

    // Compute the amount of padding required.
    uint64_t Padding = NextPowerOf2(InitSize - 1) - InitSize;

    // Cap at 128 was found experimentally to have a good data/instruction
    // overhead tradeoff.
    if (Padding > 128)
      Padding = alignTo(InitSize, 128) - InitSize;

    GlobalInits.push_back(
        ConstantAggregateZero::get(ArrayType::get(Int8Ty, Padding)));
  }
  if (!GlobalInits.empty())
    GlobalInits.pop_back();
  Constant *NewInit = ConstantStruct::getAnon(M.getContext(), GlobalInits);
  auto *CombinedGlobal =
      new GlobalVariable(M, NewInit->getType(), /*isConstant=*/true,
                         GlobalValue::PrivateLinkage, NewInit);

  StructType *NewTy = cast<StructType>(NewInit->getType());
  const StructLayout *CombinedGlobalLayout = DL.getStructLayout(NewTy);

  // Compute the offsets of the original globals within the new global.
  DenseMap<GlobalObject *, uint64_t> GlobalLayout;
  for (unsigned I = 0; I != Globals.size(); ++I)
    // Multiply by 2 to account for padding elements.
    GlobalLayout[Globals[I]] = CombinedGlobalLayout->getElementOffset(I * 2);

  lowerTypeTestCalls(TypeIds, CombinedGlobal, GlobalLayout);

  // Build aliases pointing to offsets into the combined global for each
  // global from which we built the combined global, and replace references
  // to the original globals with references to the aliases.
  for (unsigned I = 0; I != Globals.size(); ++I) {
    // Multiply by 2 to account for padding elements.
    Constant *CombinedGlobalIdxs[] = {ConstantInt::get(Int32Ty, 0),
                                      ConstantInt::get(Int32Ty, I * 2)};
    Constant *CombinedGlobalElemPtr = ConstantExpr::getGetElementPtr(
        NewInit->getType(), CombinedGlobal, CombinedGlobalIdxs);
    if (LinkerSubsectionsViaSymbols) {
      Globals[I]->replaceAllUsesWith(CombinedGlobalElemPtr);
    } else {
      assert(Globals[I]->getType()->getAddressSpace() == 0);
      GlobalAlias *GAlias = GlobalAlias::create(NewTy->getElementType(I * 2), 0,
                                                Globals[I]->getLinkage(), "",
                                                CombinedGlobalElemPtr, &M);
      GAlias->setVisibility(Globals[I]->getVisibility());
      GAlias->takeName(Globals[I]);
      Globals[I]->replaceAllUsesWith(GAlias);
    }
    Globals[I]->eraseFromParent();
  }
}

void LowerTypeTestsModule::lowerTypeTestCalls(
    ArrayRef<Metadata *> TypeIds, Constant *CombinedGlobalAddr,
    const DenseMap<GlobalObject *, uint64_t> &GlobalLayout) {
  Constant *CombinedGlobalIntAddr =
      ConstantExpr::getPtrToInt(CombinedGlobalAddr, IntPtrTy);

  // For each type identifier in this disjoint set...
  for (Metadata *TypeId : TypeIds) {
    // Build the bitset.
    BitSetInfo BSI = buildBitSet(TypeId, GlobalLayout);
    DEBUG({
      if (auto MDS = dyn_cast<MDString>(TypeId))
        dbgs() << MDS->getString() << ": ";
      else
        dbgs() << "<unnamed>: ";
      BSI.print(dbgs());
    });

    ByteArrayInfo *BAI = nullptr;

    // Lower each call to llvm.type.test for this type identifier.
    for (CallInst *CI : TypeTestCallSites[TypeId]) {
      ++NumTypeTestCallsLowered;
      Value *Lowered =
          lowerBitSetCall(CI, BSI, BAI, CombinedGlobalIntAddr, GlobalLayout);
      CI->replaceAllUsesWith(Lowered);
      CI->eraseFromParent();
    }
  }
}

void LowerTypeTestsModule::verifyTypeMDNode(GlobalObject *GO, MDNode *Type) {
  if (Type->getNumOperands() != 2)
    report_fatal_error("All operands of type metadata must have 2 elements");

  if (GO->isThreadLocal())
    report_fatal_error("Bit set element may not be thread-local");
  if (isa<GlobalVariable>(GO) && GO->hasSection())
    report_fatal_error(
        "A member of a type identifier may not have an explicit section");

  if (isa<GlobalVariable>(GO) && GO->isDeclarationForLinker())
    report_fatal_error(
        "A global var member of a type identifier must be a definition");

  auto OffsetConstMD = dyn_cast<ConstantAsMetadata>(Type->getOperand(0));
  if (!OffsetConstMD)
    report_fatal_error("Type offset must be a constant");
  auto OffsetInt = dyn_cast<ConstantInt>(OffsetConstMD->getValue());
  if (!OffsetInt)
    report_fatal_error("Type offset must be an integer constant");
}

static const unsigned kX86JumpTableEntrySize = 8;
static const unsigned kARMJumpTableEntrySize = 4;

unsigned LowerTypeTestsModule::getJumpTableEntrySize() {
  switch (Arch) {
    case Triple::x86:
    case Triple::x86_64:
      return kX86JumpTableEntrySize;
    case Triple::arm:
    case Triple::aarch64:
      return kARMJumpTableEntrySize;
    default:
      report_fatal_error("Unsupported architecture for jump tables");
  }
}

static bool isValidAsmUnquotedName(StringRef Name) {
  if (Name.empty())
    return false;

  for (char C : Name) {
    if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') ||
          (C >= '0' && C <= '9') || C == '_' || C == '$' || C == '.' ||
          C == '@'))
      return false;
  }

  return true;
}

// Create a constant representing a jump table entry for the target. This
// consists of an instruction sequence containing a relative branch to Dest. The
// constant will be laid out at address Src+(Len*Distance) where Len is the
// target-specific jump table entry size.
void LowerTypeTestsModule::createJumpTableEntry(raw_ostream &OS, Function *Dest,
                                                unsigned Distance) {
  // FIXME: replace IR Mangler with TargetLoweringObjectFile interface.
  // A private instance of Mangler we use here can not deal with unnamed
  // symbols, as it may create colliding labels. Thankfully(?), the use of
  // inline asm requires us to give names to all affected functions anyway.
  assert(Dest->hasName() && "jumptable targets can not be anonymous");
  SmallString<16> Name;
  Mang.getNameWithPrefix(Name, Dest, /* CannotUsePrivateLabel */ false);

  if (!isValidAsmUnquotedName(Name)) {
    // We are going to emit a function call as textual asm. Escaped strings
    // in such expressions are not well supported.
    report_fatal_error(
        "CFI-ICall does not allow special characters in a function name.");
  }

  if (Arch == Triple::x86 || Arch == Triple::x86_64) {
    OS << "jmp " << Name << "@plt\n";
    OS << "int3\nint3\nint3\n";
  } else if (Arch == Triple::arm || Arch == Triple::aarch64) {
    OS << "b " << Name << "\n";
  } else {
    report_fatal_error("Unsupported architecture for jump tables");
  }
}

void LowerTypeTestsModule::createJumpTableAlias(raw_ostream &OS, Function *Dest,
                                                GlobalVariable *JumpTable,
                                                unsigned Distance) {
  assert(Dest->hasName() && "jumptable targets can not be anonymous");
  SmallString<16> Name;
  Mang.getNameWithPrefix(Name, Dest, /* CannotUsePrivateLabel */ false);

  if (!isValidAsmUnquotedName(Name)) {
    // We are going to emit a function alias as textual asm. Escaped strings
    // in such expressions are not well supported.
    report_fatal_error(
        "CFI-ICall does not allow special characters in a function name.");
  }

  if (Dest->isWeakForLinker())
    OS << ".weak " << Name << "\n";
  else if (!Dest->hasLocalLinkage())
    OS << ".globl " << Name << "\n";
  OS << ".type " << Name << ", function\n";
  OS << Name << " = " << JumpTable->getName() << " + "
     << (getJumpTableEntrySize() * Distance) << "\n";
  OS << ".size " << Name << ", " << getJumpTableEntrySize() << "\n";
}

Type *LowerTypeTestsModule::getJumpTableEntryType() {
  return ArrayType::get(Int8Ty, getJumpTableEntrySize());
}

/// Given a disjoint set of type identifiers and functions, build the bit sets
/// and lower the llvm.type.test calls, architecture dependently.
void LowerTypeTestsModule::buildBitSetsFromFunctions(
    ArrayRef<Metadata *> TypeIds, ArrayRef<Function *> Functions) {
  if (Arch == Triple::x86 || Arch == Triple::x86_64 || Arch == Triple::arm ||
      Arch == Triple::aarch64)
    buildBitSetsFromFunctionsNative(TypeIds, Functions);
  else if (Arch == Triple::wasm32 || Arch == Triple::wasm64)
    buildBitSetsFromFunctionsWASM(TypeIds, Functions);
  else
    report_fatal_error("Unsupported architecture for jump tables");
}

void LowerTypeTestsModule::moveInitializerToModuleConstructor(
    GlobalVariable *GV) {
  if (WeakInitializerFn == nullptr) {
    WeakInitializerFn = Function::Create(
        FunctionType::get(Type::getVoidTy(M.getContext()),
                          /* IsVarArg */ false),
        GlobalValue::InternalLinkage, "__cfi_global_var_init", &M);
    BasicBlock *BB =
        BasicBlock::Create(M.getContext(), "entry", WeakInitializerFn);
    ReturnInst::Create(M.getContext(), BB);
    WeakInitializerFn->setSection(
        ObjectFormat == Triple::MachO
            ? "__TEXT,__StaticInit,regular,pure_instructions"
            : ".text.startup");
    // This code is equivalent to relocation application, and should run at the
    // earliest possible time (i.e. with the highest priority).
    appendToGlobalCtors(M, WeakInitializerFn, /* Priority */ 0);
  }

  IRBuilder<> IRB(WeakInitializerFn->getEntryBlock().getTerminator());
  GV->setConstant(false);
  IRB.CreateAlignedStore(GV->getInitializer(), GV, GV->getAlignment());
  GV->setInitializer(Constant::getNullValue(GV->getValueType()));
}

void LowerTypeTestsModule::findGlobalVariableUsersOf(
    Constant *C, SmallSetVector<GlobalVariable *, 8> &Out) {
  for (auto *U : C->users()){
    if (auto *GV = dyn_cast<GlobalVariable>(U))
      Out.insert(GV);
    else if (auto *C2 = dyn_cast<Constant>(U))
      findGlobalVariableUsersOf(C2, Out);
  }
}

// Replace all uses of F with (F ? JT : 0).
void LowerTypeTestsModule::replaceWeakDeclarationWithJumpTablePtr(
    Function *F, Constant *JT) {
  // The target expression can not appear in a constant initializer on most
  // (all?) targets. Switch to a runtime initializer.
  SmallSetVector<GlobalVariable *, 8> GlobalVarUsers;
  findGlobalVariableUsersOf(F, GlobalVarUsers);
  for (auto GV : GlobalVarUsers)
    moveInitializerToModuleConstructor(GV);

  // Can not RAUW F with an expression that uses F. Replace with a temporary
  // placeholder first.
  Function *PlaceholderFn =
      Function::Create(cast<FunctionType>(F->getValueType()),
                       GlobalValue::ExternalWeakLinkage, "", &M);
  F->replaceAllUsesWith(PlaceholderFn);

  Constant *Target = ConstantExpr::getSelect(
      ConstantExpr::getICmp(CmpInst::ICMP_NE, F,
                            Constant::getNullValue(F->getType())),
      JT, Constant::getNullValue(F->getType()));
  PlaceholderFn->replaceAllUsesWith(Target);
  PlaceholderFn->eraseFromParent();
}

/// Given a disjoint set of type identifiers and functions, build a jump table
/// for the functions, build the bit sets and lower the llvm.type.test calls.
void LowerTypeTestsModule::buildBitSetsFromFunctionsNative(
    ArrayRef<Metadata *> TypeIds, ArrayRef<Function *> Functions) {
  // Unlike the global bitset builder, the function bitset builder cannot
  // re-arrange functions in a particular order and base its calculations on the
  // layout of the functions' entry points, as we have no idea how large a
  // particular function will end up being (the size could even depend on what
  // this pass does!) Instead, we build a jump table, which is a block of code
  // consisting of one branch instruction for each of the functions in the bit
  // set that branches to the target function, and redirect any taken function
  // addresses to the corresponding jump table entry. In the object file's
  // symbol table, the symbols for the target functions also refer to the jump
  // table entries, so that addresses taken outside the module will pass any
  // verification done inside the module.
  //
  // In more concrete terms, suppose we have three functions f, g, h which are
  // of the same type, and a function foo that returns their addresses:
  //
  // f:
  // mov 0, %eax
  // ret
  //
  // g:
  // mov 1, %eax
  // ret
  //
  // h:
  // mov 2, %eax
  // ret
  //
  // foo:
  // mov f, %eax
  // mov g, %edx
  // mov h, %ecx
  // ret
  //
  // We output the jump table as module-level inline asm string. The end result
  // will (conceptually) look like this:
  //
  // f = .cfi.jumptable
  // g = .cfi.jumptable + 4
  // h = .cfi.jumptable + 8
  // .cfi.jumptable:
  // jmp f.cfi  ; 5 bytes
  // int3       ; 1 byte
  // int3       ; 1 byte
  // int3       ; 1 byte
  // jmp g.cfi  ; 5 bytes
  // int3       ; 1 byte
  // int3       ; 1 byte
  // int3       ; 1 byte
  // jmp h.cfi  ; 5 bytes
  // int3       ; 1 byte
  // int3       ; 1 byte
  // int3       ; 1 byte
  //
  // f.cfi:
  // mov 0, %eax
  // ret
  //
  // g.cfi:
  // mov 1, %eax
  // ret
  //
  // h.cfi:
  // mov 2, %eax
  // ret
  //
  // foo:
  // mov f, %eax
  // mov g, %edx
  // mov h, %ecx
  // ret
  //
  // Because the addresses of f, g, h are evenly spaced at a power of 2, in the
  // normal case the check can be carried out using the same kind of simple
  // arithmetic that we normally use for globals.

  // FIXME: find a better way to represent the jumptable in the IR.

  assert(!Functions.empty());

  // Build a simple layout based on the regular layout of jump tables.
  DenseMap<GlobalObject *, uint64_t> GlobalLayout;
  unsigned EntrySize = getJumpTableEntrySize();
  for (unsigned I = 0; I != Functions.size(); ++I)
    GlobalLayout[Functions[I]] = I * EntrySize;

  // Create a constant to hold the jump table.
  ArrayType *JumpTableType =
      ArrayType::get(getJumpTableEntryType(), Functions.size());
  auto JumpTable =
      new GlobalVariable(M, JumpTableType,
                         /*isConstant=*/true, GlobalValue::ExternalLinkage,
                         nullptr, ".cfi.jumptable");
  JumpTable->setVisibility(GlobalValue::HiddenVisibility);
  lowerTypeTestCalls(TypeIds, JumpTable, GlobalLayout);

  std::string AsmStr;
  raw_string_ostream AsmOS(AsmStr);

  // Build aliases pointing to offsets into the jump table, and replace
  // references to the original functions with references to the aliases.
  for (unsigned I = 0; I != Functions.size(); ++I) {
    // Need a name for the asm label. Normally, unnamed functions get temporary
    // asm labels in TargetLoweringObjectFile but we don't have access to that
    // here.
    if (!Functions[I]->hasName())
      Functions[I]->setName("unnamed");
    if (LinkerSubsectionsViaSymbols || Functions[I]->isDeclarationForLinker()) {
      Constant *CombinedGlobalElemPtr = ConstantExpr::getBitCast(
          ConstantExpr::getGetElementPtr(
              JumpTableType, JumpTable,
              ArrayRef<Constant *>{ConstantInt::get(IntPtrTy, 0),
                                   ConstantInt::get(IntPtrTy, I)}),
          Functions[I]->getType());


      if (Functions[I]->isWeakForLinker()) {
        AsmOS << ".weak " << Functions[I]->getName() << "\n";
        replaceWeakDeclarationWithJumpTablePtr(Functions[I],
                                               CombinedGlobalElemPtr);
      } else {
        Functions[I]->replaceAllUsesWith(CombinedGlobalElemPtr);
      }
    } else {
      assert(Functions[I]->getType()->getAddressSpace() == 0);

      createJumpTableAlias(AsmOS, Functions[I], JumpTable, I);

      Function *DeclAlias =
          Function::Create(cast<FunctionType>(Functions[I]->getValueType()),
                           GlobalValue::ExternalLinkage, "", &M);
      // Since the alias (DeclAlias) is actually a declaration, it can not have
      // internal linkage. Compensate for that by giving it hidden visibility.
      // With this we end up with a GOT relocation against a local symbol.
      DeclAlias->setVisibility(Functions[I]->hasLocalLinkage()
                                   ? GlobalValue::HiddenVisibility
                                   : Functions[I]->getVisibility());
      DeclAlias->takeName(Functions[I]);
      // Unnamed functions can not be added to llvm.used.
      Functions[I]->setName(DeclAlias->getName() + ".cfi");
      Functions[I]->replaceAllUsesWith(DeclAlias);
    }
    if (!Functions[I]->isDeclarationForLinker())
      Functions[I]->setLinkage(GlobalValue::InternalLinkage);
  }

  // Try to emit the jump table at the end of the text segment.
  // Jump table must come after __cfi_check in the cross-dso mode.
  // FIXME: this magic section name seems to do the trick.
  AsmOS << ".section " << (ObjectFormat == Triple::MachO
                               ? "__TEXT,__text,regular,pure_instructions"
                               : ".text.cfi, \"ax\", @progbits")
        << "\n";
  // Align the whole table by entry size.
  AsmOS << ".balign " << EntrySize << "\n";
  AsmOS << JumpTable->getName() << ":\n";
  for (unsigned I = 0; I != Functions.size(); ++I)
    createJumpTableEntry(AsmOS, Functions[I], I);

  M.appendModuleInlineAsm(AsmOS.str());

  SmallVector<GlobalValue *, 16> Used;
  Used.reserve(Functions.size());
  for (auto *F : Functions)
    Used.push_back(F);
  appendToUsed(M, Used);
}

/// Assign a dummy layout using an incrementing counter, tag each function
/// with its index represented as metadata, and lower each type test to an
/// integer range comparison. During generation of the indirect function call
/// table in the backend, it will assign the given indexes.
/// Note: Dynamic linking is not supported, as the WebAssembly ABI has not yet
/// been finalized.
void LowerTypeTestsModule::buildBitSetsFromFunctionsWASM(
    ArrayRef<Metadata *> TypeIds, ArrayRef<Function *> Functions) {
  assert(!Functions.empty());

  // Build consecutive monotonic integer ranges for each call target set
  DenseMap<GlobalObject *, uint64_t> GlobalLayout;

  for (Function *F : Functions) {
    // Skip functions that are not address taken, to avoid bloating the table
    if (!F->hasAddressTaken())
      continue;

    // Store metadata with the index for each function
    MDNode *MD = MDNode::get(F->getContext(),
                             ArrayRef<Metadata *>(ConstantAsMetadata::get(
                                 ConstantInt::get(Int64Ty, IndirectIndex))));
    F->setMetadata("wasm.index", MD);

    // Assign the counter value
    GlobalLayout[F] = IndirectIndex++;
  }

  // The indirect function table index space starts at zero, so pass a NULL
  // pointer as the subtracted "jump table" offset.
  lowerTypeTestCalls(TypeIds, ConstantPointerNull::get(Int32PtrTy),
                     GlobalLayout);
}

void LowerTypeTestsModule::buildBitSetsFromDisjointSet(
    ArrayRef<Metadata *> TypeIds, ArrayRef<GlobalObject *> Globals) {
  llvm::DenseMap<Metadata *, uint64_t> TypeIdIndices;
  for (unsigned I = 0; I != TypeIds.size(); ++I)
    TypeIdIndices[TypeIds[I]] = I;

  // For each type identifier, build a set of indices that refer to members of
  // the type identifier.
  std::vector<std::set<uint64_t>> TypeMembers(TypeIds.size());
  SmallVector<MDNode *, 2> Types;
  unsigned GlobalIndex = 0;
  for (GlobalObject *GO : Globals) {
    Types.clear();
    GO->getMetadata(LLVMContext::MD_type, Types);
    for (MDNode *Type : Types) {
      // Type = { offset, type identifier }
      unsigned TypeIdIndex = TypeIdIndices[Type->getOperand(1)];
      TypeMembers[TypeIdIndex].insert(GlobalIndex);
    }
    GlobalIndex++;
  }

  // Order the sets of indices by size. The GlobalLayoutBuilder works best
  // when given small index sets first.
  std::stable_sort(
      TypeMembers.begin(), TypeMembers.end(),
      [](const std::set<uint64_t> &O1, const std::set<uint64_t> &O2) {
        return O1.size() < O2.size();
      });

  // Create a GlobalLayoutBuilder and provide it with index sets as layout
  // fragments. The GlobalLayoutBuilder tries to lay out members of fragments as
  // close together as possible.
  GlobalLayoutBuilder GLB(Globals.size());
  for (auto &&MemSet : TypeMembers)
    GLB.addFragment(MemSet);

  // Build the bitsets from this disjoint set.
  if (Globals.empty() || isa<GlobalVariable>(Globals[0])) {
    // Build a vector of global variables with the computed layout.
    std::vector<GlobalVariable *> OrderedGVs(Globals.size());
    auto OGI = OrderedGVs.begin();
    for (auto &&F : GLB.Fragments) {
      for (auto &&Offset : F) {
        auto GV = dyn_cast<GlobalVariable>(Globals[Offset]);
        if (!GV)
          report_fatal_error("Type identifier may not contain both global "
                             "variables and functions");
        *OGI++ = GV;
      }
    }

    buildBitSetsFromGlobalVariables(TypeIds, OrderedGVs);
  } else {
    // Build a vector of functions with the computed layout.
    std::vector<Function *> OrderedFns(Globals.size());
    auto OFI = OrderedFns.begin();
    for (auto &&F : GLB.Fragments) {
      for (auto &&Offset : F) {
        auto Fn = dyn_cast<Function>(Globals[Offset]);
        if (!Fn)
          report_fatal_error("Type identifier may not contain both global "
                             "variables and functions");
        *OFI++ = Fn;
      }
    }

    buildBitSetsFromFunctions(TypeIds, OrderedFns);
  }
}

/// Lower all type tests in this module.
LowerTypeTestsModule::LowerTypeTestsModule(Module &M) : M(M) {
  Triple TargetTriple(M.getTargetTriple());
  LinkerSubsectionsViaSymbols = TargetTriple.isMacOSX();
  Arch = TargetTriple.getArch();
  ObjectFormat = TargetTriple.getObjectFormat();
}

bool LowerTypeTestsModule::lower() {
  Function *TypeTestFunc =
      M.getFunction(Intrinsic::getName(Intrinsic::type_test));
  if (!TypeTestFunc || TypeTestFunc->use_empty())
    return false;

  // Equivalence class set containing type identifiers and the globals that
  // reference them. This is used to partition the set of type identifiers in
  // the module into disjoint sets.
  typedef EquivalenceClasses<PointerUnion<GlobalObject *, Metadata *>>
      GlobalClassesTy;
  GlobalClassesTy GlobalClasses;

  // Verify the type metadata and build a mapping from type identifiers to their
  // last observed index in the list of globals. This will be used later to
  // deterministically order the list of type identifiers.
  llvm::DenseMap<Metadata *, unsigned> TypeIdIndices;
  unsigned I = 0;
  SmallVector<MDNode *, 2> Types;
  for (GlobalObject &GO : M.global_objects()) {
    Types.clear();
    GO.getMetadata(LLVMContext::MD_type, Types);
    for (MDNode *Type : Types) {
      verifyTypeMDNode(&GO, Type);
      TypeIdIndices[cast<MDNode>(Type)->getOperand(1)] = ++I;
    }
  }

  for (const Use &U : TypeTestFunc->uses()) {
    auto CI = cast<CallInst>(U.getUser());

    auto BitSetMDVal = dyn_cast<MetadataAsValue>(CI->getArgOperand(1));
    if (!BitSetMDVal)
      report_fatal_error("Second argument of llvm.type.test must be metadata");
    auto BitSet = BitSetMDVal->getMetadata();

    // Add the call site to the list of call sites for this type identifier. We
    // also use TypeTestCallSites to keep track of whether we have seen this
    // type identifier before. If we have, we don't need to re-add the
    // referenced globals to the equivalence class.
    std::pair<DenseMap<Metadata *, std::vector<CallInst *>>::iterator, bool>
        Ins = TypeTestCallSites.insert(
            std::make_pair(BitSet, std::vector<CallInst *>()));
    Ins.first->second.push_back(CI);
    if (!Ins.second)
      continue;

    // Add the type identifier to the equivalence class.
    GlobalClassesTy::iterator GCI = GlobalClasses.insert(BitSet);
    GlobalClassesTy::member_iterator CurSet = GlobalClasses.findLeader(GCI);

    // Add the referenced globals to the type identifier's equivalence class.
    for (GlobalObject &GO : M.global_objects()) {
      Types.clear();
      GO.getMetadata(LLVMContext::MD_type, Types);
      for (MDNode *Type : Types)
        if (Type->getOperand(1) == BitSet)
          CurSet = GlobalClasses.unionSets(
              CurSet, GlobalClasses.findLeader(GlobalClasses.insert(&GO)));
    }
  }

  if (GlobalClasses.empty())
    return false;

  // Build a list of disjoint sets ordered by their maximum global index for
  // determinism.
  std::vector<std::pair<GlobalClassesTy::iterator, unsigned>> Sets;
  for (GlobalClassesTy::iterator I = GlobalClasses.begin(),
                                 E = GlobalClasses.end();
       I != E; ++I) {
    if (!I->isLeader())
      continue;
    ++NumTypeIdDisjointSets;

    unsigned MaxIndex = 0;
    for (GlobalClassesTy::member_iterator MI = GlobalClasses.member_begin(I);
         MI != GlobalClasses.member_end(); ++MI) {
      if ((*MI).is<Metadata *>())
        MaxIndex = std::max(MaxIndex, TypeIdIndices[MI->get<Metadata *>()]);
    }
    Sets.emplace_back(I, MaxIndex);
  }
  std::sort(Sets.begin(), Sets.end(),
            [](const std::pair<GlobalClassesTy::iterator, unsigned> &S1,
               const std::pair<GlobalClassesTy::iterator, unsigned> &S2) {
              return S1.second < S2.second;
            });

  // For each disjoint set we found...
  for (const auto &S : Sets) {
    // Build the list of type identifiers in this disjoint set.
    std::vector<Metadata *> TypeIds;
    std::vector<GlobalObject *> Globals;
    for (GlobalClassesTy::member_iterator MI =
             GlobalClasses.member_begin(S.first);
         MI != GlobalClasses.member_end(); ++MI) {
      if ((*MI).is<Metadata *>())
        TypeIds.push_back(MI->get<Metadata *>());
      else
        Globals.push_back(MI->get<GlobalObject *>());
    }

    // Order type identifiers by global index for determinism. This ordering is
    // stable as there is a one-to-one mapping between metadata and indices.
    std::sort(TypeIds.begin(), TypeIds.end(), [&](Metadata *M1, Metadata *M2) {
      return TypeIdIndices[M1] < TypeIdIndices[M2];
    });

    // Build bitsets for this disjoint set.
    buildBitSetsFromDisjointSet(TypeIds, Globals);
  }

  allocateByteArrays();

  return true;
}

PreservedAnalyses LowerTypeTestsPass::run(Module &M,
                                          ModuleAnalysisManager &AM) {
  bool Changed = LowerTypeTestsModule(M).lower();
  if (!Changed)
    return PreservedAnalyses::all();
  return PreservedAnalyses::none();
}
