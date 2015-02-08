; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  ret

; CHECK: ret                  ; encoding: [0x08,0x95]
