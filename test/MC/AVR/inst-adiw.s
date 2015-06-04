; RUN: llvm-mc -triple avr-none -mattr=addsubiw -show-encoding < %s | FileCheck %s


foo:

  adiw X,  12
  adiw X,  63
  
  adiw Y,  17
  adiw Y,  0
  
  adiw Z,  63
  adiw Z,  3

; CHECK: adiw x,  12                 ; encoding: [0x1c,0x96]
; CHECK: adiw x,  63                 ; encoding: [0xdf,0x96]

; CHECK: adiw y,  17                 ; encoding: [0x61,0x96]
; CHECK: adiw y,  0                  ; encoding: [0x20,0x96]

; CHECK: adiw z,  63                 ; encoding: [0xff,0x96]
; CHECK: adiw z,  3                  ; encoding: [0x33,0x96]
