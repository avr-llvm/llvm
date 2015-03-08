; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  sbci r17, 21
  sbci r23, 196
  sbci r30, 244
  sbci r19, 16
  
; CHECK: sbci r17, 21                 ; encoding: [0x15,0x41]
; CHECK: sbci r23, 196                ; encoding: [0x74,0x4c]
; CHECK: sbci r30, 244                ; encoding: [0xe4,0x4f]
; CHECK: sbci r19, 16                 ; encoding: [0x30,0x41]
