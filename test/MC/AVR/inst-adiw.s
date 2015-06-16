; RUN: llvm-mc -triple avr-none -mattr=addsubiw -show-encoding < %s | FileCheck %s


foo:

  adiw r26,  12
  adiw r26,  63
  
  adiw r28,  17
  adiw r28,  0
  
  adiw r30,  63
  adiw r30,  3

; CHECK: adiw r26,  12               ; encoding: [0x1c,0x96]
; CHECK: adiw r26,   63               ; encoding: [0xdf,0x96]

; CHECK: adiw r28,  17               ; encoding: [0x61,0x96]
; CHECK: adiw r28,  0                ; encoding: [0x20,0x96]

; CHECK: adiw r30,  63               ; encoding: [0xff,0x96]
; CHECK: adiw r30,  3                ; encoding: [0x33,0x96]
