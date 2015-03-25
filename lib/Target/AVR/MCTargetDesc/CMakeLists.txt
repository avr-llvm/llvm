add_llvm_library(LLVMAVRDesc
  AVRMCAsmInfo.cpp
  AVRMCExpr.cpp
  AVRMCTargetDesc.cpp
  AVRAsmBackend.cpp
  AVRELFObjectWriter.cpp
  AVRMCCodeEmitter.cpp
  AVRTargetStreamer.cpp
  )

add_dependencies(LLVMAVRDesc AVRCommonTableGen)
