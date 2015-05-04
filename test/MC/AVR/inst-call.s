; RUN: llvm-mc -triple avr-none -mattr=jmpcall -show-encoding < %s | FileCheck %s


foo:

  call  4096 
  call  -124
  call   -12
  call   0

; CHECK: call  4096                 ; encoding: [0x00,0x08,0x0e,0x94]
; CHECK: call -124                  ; encoding: [0xc2,0xff,0xff,0x95]
; CHECK: call -12                   ; encoding: [0xfa,0xff,0xff,0x95]
; CHECK: call  0                    ; encoding: [0x00,0x00,0x0e,0x94]
