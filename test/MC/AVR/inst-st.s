; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  st X,  r10
  st X,  r17
  
  st Y,  r30
  st Y,  r19
  
  st Z,  r10
  st Z,  r2

; CHECK: st X,  r10                  ; encoding: [0xac,0x92]
; CHECK: st X,  r17                  ; encoding: [0x1c,0x93]

; CHECK: st Y,  r30                  ; encoding: [0xe8,0x83]
; CHECK: st Y,  r19                  ; encoding: [0x38,0x83]

; CHECK: st Z,  r10                  ; encoding: [0xa0,0x82]
; CHECK: st Z,  r2                   ; encoding: [0x20,0x82]
