; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  las Z, r13
  las Z, r0
  las Z, r31
  las Z, r3

; CHECK: las r2, r13                  ; encoding: [0xd5,0x92]
; CHECK: las r9, r0                   ; encoding: [0x05,0x92]
; CHECK: las r5, r31                  ; encoding: [0xf5,0x93]
; CHECK: las r3, r3                   ; encoding: [0x35,0x92]
