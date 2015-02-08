; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  rcall   0 
  rcall  -8
  rcall   12
  rcall   46

; CHECK: rcall  0                   ; encoding: [0x00,0xdf]
; CHECK: rcall -8                   ; encoding: [0xfa,0xdf]
; CHECK: rcall  12                  ; encoding: [0x03,0xd0]
; CHECK: rcall  46                  ; encoding: [0x13,0xd0]
