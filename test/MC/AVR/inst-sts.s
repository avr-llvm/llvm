; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  sts 3,   r5
  sts 255, r7

; CHECK:  sts 3,   r5                 ; encoding: [0x50,0x92,0x03,0x00]
; CHECK:  sts 255, r7                 ; encoding: [0x70,0x92,0xff,0x00]
