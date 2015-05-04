; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  brbc 3, .+8
  brbc 0, .-16

; CHECK: brbc 3, .+8                   ; encoding: [0x23,0xf4]
; CHECK: brbc 0, .-16                  ; encoding: [0xc0,0xf7]

