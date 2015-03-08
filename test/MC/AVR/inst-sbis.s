; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  sbis 4,  3 
  sbis 6,  2
  sbis 16, 5
  sbis 0,  0

; CHECK: sbis 4,  3                  ; encoding: [0x23,0x9b]
; CHECK: sbis 6,  2                  ; encoding: [0x32,0x9b]
; CHECK: sbis 16, 5                  ; encoding: [0x85,0x9b]
; CHECK: sbis 0,  0                  ; encoding: [0x00,0x9b]
