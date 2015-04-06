; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  cpi r16, 241
  cpi r29, 190
  cpi r22, 172
  cpi r27, 92
  
; CHECK: cpi r16, 241                  ; encoding: [0x01,0x3f]
; CHECK: cpi r29, 190                  ; encoding: [0xde,0x3b]
; CHECK: cpi r22, 172                  ; encoding: [0x6c,0x3a]
; CHECK: cpi r27, 92                   ; encoding: [0xbc,0x35]
