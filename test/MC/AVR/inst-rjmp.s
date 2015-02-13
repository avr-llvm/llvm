; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  rjmp   2 
  rjmp  -2
  rjmp   8
  rjmp   0

; CHECK: rjmp  2                   ; encoding: [0x00,0xc0]
; CHECK: rjmp -2                   ; encoding: [0xfd,0xcf]
; CHECK: rjmp  8                   ; encoding: [0x01,0xc0]
; CHECK: rjmp  0                   ; encoding: [0xfc,0xcf]
