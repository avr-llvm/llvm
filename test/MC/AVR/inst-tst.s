; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  tst r3
  tst r14
  tst r24
  tst r12

; CHECK: and r3,  r3               ; encoding: [0x33,0x20]
; CHECK: and r14, r14              ; encoding: [0xee,0x20]
; CHECK: and r24, r24              ; encoding: [0x88,0x23]
; CHECK: and r12, r12              ; encoding: [0xcc,0x20]
