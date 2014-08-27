add_llvm_library(LLVMAVRDesc
  AVRMCAsmInfo.cpp
  AVRMCExpr.cpp
  AVRMCTargetDesc.cpp
  )

add_dependencies(LLVMAVRDesc AVRCommonTableGen)