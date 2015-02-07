; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  and r30, r2
  and r1,  r2
  and r15, r7
  and r27, r27

; CHECK: and r30, r2                  ; encoding: [0xe2,0x21]
; CHECK: and r1,  r2                  ; encoding: [0x12,0x20]
; CHECK: and r15, r7                  ; encoding: [0xf7,0x20]
; CHECK: and r27, r27                 ; encoding: [0xbb,0x23]
