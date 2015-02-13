; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  ldi r16, 0xf1
  ldi r29, 0xbe
  ldi r22, 0xac
  ldi r27, 92
  
; CHECK: ldi r16, 0xf1                 ; encoding: [0x01,0xef]
; CHECK: ldi r29, 0xbe                 ; encoding: [0xde,0xeb]
; CHECK: ldi r22, 0xac                 ; encoding: [0x6c,0xea]
; CHECK: ldi r27, 92                   ; encoding: [0xbc,0xe5]
