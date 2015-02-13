; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  sbi 3, 5
  sbi 1, 1
  sbi 0, 0
  sbi 7, 2

; CHECK: sbi 3, 5                  ; encoding: [0x1d,0x9a]
; CHECK: sbi 1, 1                  ; encoding: [0x09,0x9a]
; CHECK: sbi 0, 0                  ; encoding: [0x00,0x9a]
; CHECK: sbi 7, 2                  ; encoding: [0x3a,0x9a]
