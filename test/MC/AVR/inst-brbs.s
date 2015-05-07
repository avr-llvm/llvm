; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  brbs 3, .+8
  brbs 0, .-12

; CHECK: brvs .+8                   ; encoding: [0x23,0xf0]
; CHECK: brcs .-12                  ; encoding: [0xd0,0xf3]

