; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  movw X, Y
  movw Y, X
  movw Y, Z
  movw Z, X


; CHECK: movw X, Y                   ; encoding: [0xde,0x01]
; CHECK: movw Y, X                   ; encoding: [0xed,0x01]
; CHECK: movw Y, Z                   ; encoding: [0xef,0x01]
; CHECK: movw Z, X                   ; encoding: [0xfd,0x01]
