//===- CGSCCPassManagerTest.cpp -------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/SourceMgr.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {

class TestModuleAnalysis {
public:
  struct Result {
    Result(int Count) : FunctionCount(Count) {}
    int FunctionCount;
  };

  static void *ID() { return (void *)&PassID; }
  static StringRef name() { return "TestModuleAnalysis"; }

  TestModuleAnalysis(int &Runs) : Runs(Runs) {}

  Result run(Module &M, ModuleAnalysisManager &AM) {
    ++Runs;
    return Result(M.size());
  }

private:
  static char PassID;

  int &Runs;
};

char TestModuleAnalysis::PassID;

class TestSCCAnalysis {
public:
  struct Result {
    Result(int Count) : FunctionCount(Count) {}
    int FunctionCount;
  };

  static void *ID() { return (void *)&PassID; }
  static StringRef name() { return "TestSCCAnalysis"; }

  TestSCCAnalysis(int &Runs) : Runs(Runs) {}

  Result run(LazyCallGraph::SCC &C, CGSCCAnalysisManager &AM, LazyCallGraph &) {
    ++Runs;
    return Result(C.size());
  }

private:
  static char PassID;

  int &Runs;
};

char TestSCCAnalysis::PassID;

class TestFunctionAnalysis {
public:
  struct Result {
    Result(int Count) : InstructionCount(Count) {}
    int InstructionCount;
  };

  static void *ID() { return (void *)&PassID; }
  static StringRef name() { return "TestFunctionAnalysis"; }

  TestFunctionAnalysis(int &Runs) : Runs(Runs) {}

  Result run(Function &F, FunctionAnalysisManager &AM) {
    ++Runs;
    int Count = 0;
    for (Instruction &I : instructions(F)) {
      (void)I;
      ++Count;
    }
    return Result(Count);
  }

private:
  static char PassID;

  int &Runs;
};

char TestFunctionAnalysis::PassID;

class TestImmutableFunctionAnalysis {
public:
  struct Result {
    bool invalidate(Function &, const PreservedAnalyses &) { return false; }
  };

  static void *ID() { return (void *)&PassID; }
  static StringRef name() { return "TestImmutableFunctionAnalysis"; }

  TestImmutableFunctionAnalysis(int &Runs) : Runs(Runs) {}

  Result run(Function &F, FunctionAnalysisManager &AM) {
    ++Runs;
    return Result();
  }

private:
  static char PassID;

  int &Runs;
};

char TestImmutableFunctionAnalysis::PassID;

struct LambdaSCCPass : public PassInfoMixin<LambdaSCCPass> {
  template <typename T> LambdaSCCPass(T &&Arg) : Func(std::forward<T>(Arg)) {}
  // We have to explicitly define all the special member functions because MSVC
  // refuses to generate them.
  LambdaSCCPass(LambdaSCCPass &&Arg) : Func(std::move(Arg.Func)) {}
  LambdaSCCPass &operator=(LambdaSCCPass &&RHS) {
    Func = std::move(RHS.Func);
    return *this;
  }

  PreservedAnalyses run(LazyCallGraph::SCC &C, CGSCCAnalysisManager &AM,
                        LazyCallGraph &CG, CGSCCUpdateResult &UR) {
    return Func(C, AM, CG, UR);
  }

  std::function<PreservedAnalyses(LazyCallGraph::SCC &, CGSCCAnalysisManager &,
                                  LazyCallGraph &, CGSCCUpdateResult &)>
      Func;
};

struct LambdaFunctionPass : public PassInfoMixin<LambdaFunctionPass> {
  template <typename T> LambdaFunctionPass(T &&Arg) : Func(std::forward<T>(Arg)) {}
  // We have to explicitly define all the special member functions because MSVC
  // refuses to generate them.
  LambdaFunctionPass(LambdaFunctionPass &&Arg) : Func(std::move(Arg.Func)) {}
  LambdaFunctionPass &operator=(LambdaFunctionPass &&RHS) {
    Func = std::move(RHS.Func);
    return *this;
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    return Func(F, AM);
  }

  std::function<PreservedAnalyses(Function &, FunctionAnalysisManager &)> Func;
};

std::unique_ptr<Module> parseIR(const char *IR) {
  // We just use a static context here. This is never called from multiple
  // threads so it is harmless no matter how it is implemented. We just need
  // the context to outlive the module which it does.
  static LLVMContext C;
  SMDiagnostic Err;
  return parseAssemblyString(IR, Err, C);
}

class CGSCCPassManagerTest : public ::testing::Test {
protected:
  LLVMContext Context;
  std::unique_ptr<Module> M;

public:
  CGSCCPassManagerTest()
      : M(parseIR("define void @f() {\n"
                  "entry:\n"
                  "  call void @g()\n"
                  "  call void @h1()\n"
                  "  ret void\n"
                  "}\n"
                  "define void @g() {\n"
                  "entry:\n"
                  "  call void @g()\n"
                  "  call void @x()\n"
                  "  ret void\n"
                  "}\n"
                  "define void @h1() {\n"
                  "entry:\n"
                  "  call void @h2()\n"
                  "  ret void\n"
                  "}\n"
                  "define void @h2() {\n"
                  "entry:\n"
                  "  call void @h3()\n"
                  "  call void @x()\n"
                  "  ret void\n"
                  "}\n"
                  "define void @h3() {\n"
                  "entry:\n"
                  "  call void @h1()\n"
                  "  ret void\n"
                  "}\n"
                  "define void @x() {\n"
                  "entry:\n"
                  "  ret void\n"
                  "}\n")) {}
};

TEST_F(CGSCCPassManagerTest, Basic) {
  FunctionAnalysisManager FAM(/*DebugLogging*/ true);
  int FunctionAnalysisRuns = 0;
  FAM.registerPass([&] { return TestFunctionAnalysis(FunctionAnalysisRuns); });
  int ImmutableFunctionAnalysisRuns = 0;
  FAM.registerPass([&] {
    return TestImmutableFunctionAnalysis(ImmutableFunctionAnalysisRuns);
  });

  CGSCCAnalysisManager CGAM(/*DebugLogging*/ true);
  int SCCAnalysisRuns = 0;
  CGAM.registerPass([&] { return TestSCCAnalysis(SCCAnalysisRuns); });

  ModuleAnalysisManager MAM(/*DebugLogging*/ true);
  int ModuleAnalysisRuns = 0;
  MAM.registerPass([&] { return LazyCallGraphAnalysis(); });
  MAM.registerPass([&] { return TestModuleAnalysis(ModuleAnalysisRuns); });

  MAM.registerPass([&] { return FunctionAnalysisManagerModuleProxy(FAM); });
  MAM.registerPass([&] { return CGSCCAnalysisManagerModuleProxy(CGAM); });
  CGAM.registerPass([&] { return FunctionAnalysisManagerCGSCCProxy(FAM); });
  CGAM.registerPass([&] { return ModuleAnalysisManagerCGSCCProxy(MAM); });
  FAM.registerPass([&] { return CGSCCAnalysisManagerFunctionProxy(CGAM); });
  FAM.registerPass([&] { return ModuleAnalysisManagerFunctionProxy(MAM); });

  ModulePassManager MPM(/*DebugLogging*/ true);
  MPM.addPass(RequireAnalysisPass<TestModuleAnalysis, Module>());

  CGSCCPassManager CGPM1(/*DebugLogging*/ true);
  int SCCPassRunCount1 = 0;
  int AnalyzedInstrCount1 = 0;
  int AnalyzedSCCFunctionCount1 = 0;
  int AnalyzedModuleFunctionCount1 = 0;
  CGPM1.addPass(
      LambdaSCCPass([&](LazyCallGraph::SCC &C, CGSCCAnalysisManager &AM,
                        LazyCallGraph &CG, CGSCCUpdateResult &UR) {
        ++SCCPassRunCount1;

        const ModuleAnalysisManager &MAM =
            AM.getResult<ModuleAnalysisManagerCGSCCProxy>(C, CG).getManager();
        FunctionAnalysisManager &FAM =
            AM.getResult<FunctionAnalysisManagerCGSCCProxy>(C, CG).getManager();
        if (TestModuleAnalysis::Result *TMA =
                MAM.getCachedResult<TestModuleAnalysis>(
                    *C.begin()->getFunction().getParent()))
          AnalyzedModuleFunctionCount1 += TMA->FunctionCount;

        TestSCCAnalysis::Result &AR = AM.getResult<TestSCCAnalysis>(C, CG);
        AnalyzedSCCFunctionCount1 += AR.FunctionCount;
        for (LazyCallGraph::Node &N : C) {
          TestFunctionAnalysis::Result &FAR =
              FAM.getResult<TestFunctionAnalysis>(N.getFunction());
          AnalyzedInstrCount1 += FAR.InstructionCount;

          // Just ensure we get the immutable results.
          (void)FAM.getResult<TestImmutableFunctionAnalysis>(N.getFunction());
        }

        return PreservedAnalyses::all();
      }));

  FunctionPassManager FPM1(/*DebugLogging*/ true);
  int FunctionPassRunCount1 = 0;
  FPM1.addPass(LambdaFunctionPass([&](Function &, FunctionAnalysisManager &) {
    ++FunctionPassRunCount1;
    return PreservedAnalyses::all();
  }));
  CGPM1.addPass(createCGSCCToFunctionPassAdaptor(std::move(FPM1)));
  MPM.addPass(createModuleToPostOrderCGSCCPassAdaptor(std::move(CGPM1)));

  MPM.run(*M, MAM);

  EXPECT_EQ(1, ModuleAnalysisRuns);
  EXPECT_EQ(4, SCCAnalysisRuns);
  EXPECT_EQ(6, FunctionAnalysisRuns);
  EXPECT_EQ(6, ImmutableFunctionAnalysisRuns);

  EXPECT_EQ(4, SCCPassRunCount1);
  EXPECT_EQ(14, AnalyzedInstrCount1);
  EXPECT_EQ(6, AnalyzedSCCFunctionCount1);
  EXPECT_EQ(4 * 6, AnalyzedModuleFunctionCount1);
}

}
