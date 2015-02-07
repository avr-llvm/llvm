; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  andi r16, 0xff
  andi r29, 0xbe
  andi r22, 0xac
  andi r27, 92
  
; CHECK: andi r16, 0xff                 ; encoding: [0x0f,0x7f]
; CHECK: andi r29, 0xbe                 ; encoding: [0xde,0x7b]
; CHECK: andi r22, 0xac                 ; encoding: [0x6c,0x7a]
; CHECK: andi r27, 92                   ; encoding: [0xbc,0x75]
