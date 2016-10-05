add_llvm_library(LLVMAVRDesc
  AVRAsmBackend.cpp
  AVRMCAsmInfo.cpp
  AVRMCExpr.cpp
  AVRELFObjectWriter.cpp
  AVRELFStreamer.cpp
  AVRMCCodeEmitter.cpp
  AVRMCTargetDesc.cpp
  AVRTargetStreamer.cpp
)

add_dependencies(LLVMAVRDesc AVRCommonTableGen)

