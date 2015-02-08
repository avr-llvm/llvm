; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  eor r5,  r7
  eor r23, r30
  eor r0,  r14
  eor r10,  r28

; CHECK: eor r5,  r7                ; encoding: [0x57,0x24]
; CHECK: eor r23, r30               ; encoding: [0x7e,0x27]
; CHECK: eor r0,  r14               ; encoding: [0x0e,0x24]
; CHECK: eor r10, r28               ; encoding: [0xac,0x26]
