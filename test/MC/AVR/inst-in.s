; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  in r2, 4
  in r9, 6
  in r5, 32
  in r0, 0

; CHECK: in r2, 4                   ; encoding: [0x24,0xb0]
; CHECK: in r9, 6                   ; encoding: [0x96,0xb0]
; CHECK: in r5, 32                  ; encoding: [0x50,0xb4]
; CHECK: in r0, 0                   ; encoding: [0x00,0xb0]
