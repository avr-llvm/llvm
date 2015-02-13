; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  des 0x00
  des 0x06
  des 0x01
  des 0x08

; CHECK: des 0x00                  ; encoding: [0x0b,0x94]
; CHECK: des 0x06                  ; encoding: [0x6b,0x94]
; CHECK: des 0x01                  ; encoding: [0x1b,0x94]
; CHECK: des 0x08                  ; encoding: [0x8b,0x94]
