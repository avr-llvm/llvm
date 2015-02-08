; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  or r31, r12
  or r12, r13
  or r0,  r14
  or r9,  r29

; CHECK: or r31, r12                 ; encoding: [0xfc,0x29]
; CHECK: or r12, r13                 ; encoding: [0xcd,0x28]
; CHECK: or r0,  r14                 ; encoding: [0x0e,0x28]
; CHECK: or r9,  r29                 ; encoding: [0x9d,0x2a]
