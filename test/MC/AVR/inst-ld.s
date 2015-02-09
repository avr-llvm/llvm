; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  ld r10, X
  ld r17, X
  
  ld r30, Y
  ld r19, Y
  
  ld r10, Z
  ld r2,  Z

; CHECK: ld r10, X                  ; encoding: [0xac,0x90]
; CHECK: ld r17, X                  ; encoding: [0x1c,0x91]

; CHECK: ld r30, Y                  ; encoding: [0xe8,0x81]
; CHECK: ld r19, Y                  ; encoding: [0x38,0x81]

; CHECK: ld r10, Z                  ; encoding: [0xa0,0x80]
; CHECK: ld r2,  Z                  ; encoding: [0x20,0x80]
