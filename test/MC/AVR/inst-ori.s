; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  ori r17, 0b11010000
  ori r24, 0xbe
  ori r20, 0xad
  ori r31, 0x00
  
; CHECK: ori r17, 0b11010000           ; encoding: [0x10,0x6d]
; CHECK: ori r24, 0xbe                 ; encoding: [0x8e,0x6b]
; CHECK: ori r20, 0xad                 ; encoding: [0x4d,0x6a]
; CHECK: ori r31, 0x00                 ; encoding: [0xf0,0x60]
