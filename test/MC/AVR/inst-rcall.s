; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  rcall  .+0
  rcall  .-8
  rcall  .+12
  rcall  .+46

; CHECK: rcall  .+0                   ; encoding: [0x00,0xd0]
; CHECK: rcall  .-8                   ; encoding: [0xfc,0xdf]
; CHECK: rcall  .+12                  ; encoding: [0x06,0xd0]
; CHECK: rcall  .+46                  ; encoding: [0x17,0xd0]
