; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  brbc 3, .+8
  brbc 0, .-16

; CHECK: brvc .+8                   ; encoding: [0x23,0xf4]
; CHECK: brcc .-16                  ; encoding: [0xc0,0xf7]

