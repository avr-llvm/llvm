; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  sbr r17, 0b11010000
  sbr r24, 0xbe
  sbr r20, 0xad
  sbr r31, 0x00
  
; CHECK: sbr r17, 0b11010000           ; encoding: [0x10,0x6d]
; CHECK: sbr r24, 0xbe                 ; encoding: [0x8e,0x6b]
; CHECK: sbr r20, 0xad                 ; encoding: [0x4d,0x6a]
; CHECK: sbr r31, 0x00                 ; encoding: [0xf0,0x60]
