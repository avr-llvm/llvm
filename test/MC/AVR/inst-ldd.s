; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  ldd r2, Y+2 
  ldd r0, Y+0

  ldd r9, Z+12 
  ldd r7, Z+30

; CHECK: ldd r2, Y+2                  ; encoding: [0x2a.0x80]
; CHECK: ldd r0, Y+0                  ; encoding: [0x08,0x80]
                     
; CHECK: ldd r9, Z+12                 ; encoding: [0x94,0x84]
; CHECK: ldd r7, Z+30                 ; encoding: [0x76,0x8c]
