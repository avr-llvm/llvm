; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  lac Z, r13
  lac Z, r0
  lac Z, r31
  lac Z, r3

; CHECK: lac r2, r13                  ; encoding: [0xd6,0x92]
; CHECK: lac r9, r0                   ; encoding: [0x06,0x92]
; CHECK: lac r5, r31                  ; encoding: [0xf6,0x93]
; CHECK: lac r3, r3                   ; encoding: [0x36,0x92]
