//===----- CompileOnDemandLayerTest.cpp - Unit tests for the COD layer ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "OrcTestCommon.h"
#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::orc;

namespace {

class DummyCallbackManager : public orc::JITCompileCallbackManagerBase {
public:
  DummyCallbackManager()
      : JITCompileCallbackManagerBase(0, 0), NextStubAddress(0),
        UniversalCompile([]() { return 0; }) {
  }

  CompileCallbackInfo getCompileCallback(LLVMContext &Context) override {
    return CompileCallbackInfo(++NextStubAddress, UniversalCompile);
  }
public:
  TargetAddress NextStubAddress;
  CompileFtor UniversalCompile;
};

class DummyStubsManager : public orc::IndirectStubsManagerBase {
public:
  std::error_code init(const StubInitsMap &StubInits) override {
    llvm_unreachable("Not implemented");
  }

  JITSymbol findStub(StringRef Name, bool ExportedStubsOnly) override {
    llvm_unreachable("Not implemented");
  }

  JITSymbol findPointer(StringRef Name) override {
    llvm_unreachable("Not implemented");
  }

  std::error_code updatePointer(StringRef Name,
                                TargetAddress NewAddr) override {
    llvm_unreachable("Not implemented");
  }
};

TEST(CompileOnDemandLayerTest, FindSymbol) {
  auto MockBaseLayer =
    createMockBaseLayer<int>(DoNothingAndReturn<int>(0),
                             DoNothingAndReturn<void>(),
                             [](const std::string &Name, bool) {
                               if (Name == "foo")
                                 return JITSymbol(1, JITSymbolFlags::Exported);
                               return JITSymbol(nullptr);
                             },
                             DoNothingAndReturn<JITSymbol>(nullptr));

  typedef decltype(MockBaseLayer) MockBaseLayerT;
  DummyCallbackManager CallbackMgr;
  auto StubsMgrBuilder =
    []() {
      return llvm::make_unique<DummyStubsManager>();
    };

  llvm::orc::CompileOnDemandLayer<MockBaseLayerT>
    COD(MockBaseLayer,
        [](Function &F) { std::set<Function*> S; S.insert(&F); return S; },
        CallbackMgr, StubsMgrBuilder, true);
  auto Sym = COD.findSymbol("foo", true);

  EXPECT_TRUE(!!Sym)
    << "CompileOnDemand::findSymbol should call findSymbol in the base layer.";
}

}
