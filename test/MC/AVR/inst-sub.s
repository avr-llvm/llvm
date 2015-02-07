; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  sub r13, r2
  sub r9,  r3
  sub r18, r9
  sub r29, r29

; CHECK: sub r13, r2                  ; encoding: [0xd2,0x18]
; CHECK: sub r9,  r3                  ; encoding: [0x93,0x18]
; CHECK: sub r18, 9                   ; encoding: [0x29,0x19]
; CHECK: sub r29, r29                 ; encoding: [0xdd,0x1b]
