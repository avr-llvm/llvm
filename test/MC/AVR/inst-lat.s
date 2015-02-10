; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  lat Z, r13
  lat Z, r0
  lat Z, r31
  lat Z, r3

; CHECK: lat r2, r13                  ; encoding: [0xd7,0x92]
; CHECK: lat r9, r0                   ; encoding: [0x07,0x92]
; CHECK: lat r5, r31                  ; encoding: [0xf7,0x93]
; CHECK: lat r3, r3                   ; encoding: [0x37,0x92]
