; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  brbs 3, 8
  brbs 0, 0

; CHECK: brbs 3, 8                   ; encoding: [0x1b,0xf0]
; CHECK: brbs 0, 0                   ; encoding: [0xf0,0xf3]

