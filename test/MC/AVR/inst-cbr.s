; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  cbr r17, 0b11010000
  cbr r24, 0xbe
  cbr r20, 0xad
  cbr r31, 0x00
  
; CHECK: cbr r17, 0b11010000           ; encoding: [0x1f,0x72]
; CHECK: cbr r24, 0xbe                 ; encoding: [0x81,0x74]
; CHECK: cbr r20, 0xad                 ; encoding: [0x42,0x75]
; CHECK: cbr r31, 0x00                 ; encoding: [0xff,0x7f]
