; RUN: llvm-mc -triple avr-none -mattr=movw -show-encoding < %s | FileCheck %s


foo:

  movw r10, r8
  movw r12, r16
  movw r20, r22
  movw r8,  r12

; CHECK: movw r11:r10, r9:r8             ; encoding: [0x54,0x01]
; CHECK: movw r13:r12, r17:r16           ; encoding: [0x68,0x01]
; CHECK: movw r21:r20, r23:r22           ; encoding: [0xab,0x01]
; CHECK: movw r9:r8,   r13:r12           ; encoding: [0x46,0x01]
