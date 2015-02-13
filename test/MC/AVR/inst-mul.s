; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  mul r2, r13
  mul r9, r0
  mul r5, r31
  mul r3, r3

; CHECK: mul r2, r13                  ; encoding: [0x2d,0x9c]
; CHECK: mul r9, r0                   ; encoding: [0x90,0x9c]
; CHECK: mul r5, r31                  ; encoding: [0x5f,0x9e]
; CHECK: mul r3, r3                   ; encoding: [0x33,0x9c]
