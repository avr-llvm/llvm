; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  add r2, r13
  add r9, r0
  add r5, r31
  add r3, r3

; CHECK: add r2, r13                  ; encoding: [0x2d,0x0c]
; CHECK: add r9, r0                   ; encoding: [0x90,0x0c]
; CHECK: add r5, r31                  ; encoding: [0x5f,0x0e]
; CHECK: add r3, r3                   ; encoding: [0x33,0x0c]
