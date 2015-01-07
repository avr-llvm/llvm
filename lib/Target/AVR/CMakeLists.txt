set(LLVM_TARGET_DEFINITIONS AVR.td)

tablegen(LLVM AVRGenRegisterInfo.inc -gen-register-info)
tablegen(LLVM AVRGenInstrInfo.inc -gen-instr-info)
tablegen(LLVM AVRGenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM AVRGenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM AVRGenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM AVRGenDAGISel.inc -gen-dag-isel)
tablegen(LLVM AVRGenCallingConv.inc -gen-callingconv)
tablegen(LLVM AVRGenSubtargetInfo.inc -gen-subtarget)
add_public_tablegen_target(AVRCommonTableGen)

add_llvm_target(AVRCodeGen
  AVRAsmPrinter.cpp
  AVRBranchSelector.cpp
  AVRExpandPseudoInsts.cpp
  AVRFrameLowering.cpp
  AVRInstrInfo.cpp
  AVRISelDAGToDAG.cpp
  AVRISelLowering.cpp
  AVRMachineFunctionInfo.cpp
  AVRMCInstLower.cpp
  AVRRegisterInfo.cpp
  AVRSelectionDAGInfo.cpp
  AVRSubtarget.cpp
  AVRTargetMachine.cpp
  AVRTargetObjectFile.cpp
  )

add_dependencies(LLVMAVRCodeGen intrinsics_gen)

add_subdirectory(AsmParser)
add_subdirectory(InstPrinter)
add_subdirectory(TargetInfo)
add_subdirectory(MCTargetDesc)

