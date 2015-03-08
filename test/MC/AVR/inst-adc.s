; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  adc r12, r3
  adc r10, r0
  adc r5,  r4
  adc r13, r13

; CHECK: adc r12, r3                 ; encoding: [0xc3,0x1c]
; CHECK: adc r10, r0                 ; encoding: [0xa0,0x1c]
; CHECK: adc r5, r4                  ; encoding: [0x54,0x1c]
; CHECK: adc r13, r13                ; encoding: [0xdd,0x1c]
