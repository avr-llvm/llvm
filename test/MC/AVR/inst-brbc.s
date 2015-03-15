; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  brbc 3, 8
  brbc 0, 0

; CHECK: brbc 3, .+8                   ; encoding: [0x1b,0xf4]
; CHECK: brbc 0, .+0                   ; encoding: [0xf0,0xf7]

