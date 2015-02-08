; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  sbc r30, 13
  sbc r9,  r0
  sbc r15, r4
  sbc r31, r31

; CHECK: sbc r30, 13                 ; encoding: [0xed,0x09]
; CHECK: sbc r9,  r0                 ; encoding: [0x90,0x08]
; CHECK: sbc r15, r4                 ; encoding: [0xf4,0x08]
; CHECK: sbc r31, r31                ; encoding: [0xff,0x0b]
