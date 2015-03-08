; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  adiw X,  12
  adiw X,  63
  
  adiw Y,  17
  adiw Y,  0
  
  adiw Z,  63
  adiw Z,  3

; CHECK: adiw r26,  12                 ; encoding: [0x1c,0x96]
; CHECK: adiw r26,  63                 ; encoding: [0xdf,0x96]

; CHECK: adiw r28,  17                 ; encoding: [0x61,0x96]
; CHECK: adiw r28,  0                  ; encoding: [0x20,0x96]

; CHECK: adiw r30,  63                 ; encoding: [0xff,0x96]
; CHECK: adiw r30,  3                  ; encoding: [0x33,0x96]
