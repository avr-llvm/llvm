; RUN: llvm-mc -triple avr -mattr=sram -show-encoding < %s | FileCheck %s


foo:

  lds r16, 241
  lds r29, 190
  lds r22, 172
  lds r27, 92
  
; CHECK: lds r16, 241                 ; encoding: [0x00,0x91,0xf1,0x00]
; CHECK: lds r29, 190                 ; encoding: [0xd0,0x91,0xbe,0x00]
; CHECK: lds r22, 172                 ; encoding: [0x60,0x91,0xac,0x00]
; CHECK: lds r27, 92                  ; encoding: [0xb0,0x91,0x5c,0x00]
