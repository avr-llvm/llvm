; RUN: llvm-mc -triple avr-none -mattr=addsubiw -show-encoding < %s | FileCheck %s


foo:

  sbiw r26, 54
  sbiw X,   63
  
  sbiw 28,  52
  sbiw r28, 0
  
  sbiw r30, 63
  sbiw Z,   47

  sbiw r24,     1
  sbiw r25:r24, 2
  sbiw r27:r26, 3


; CHECK: sbiw r26,  54                 ; encoding: [0xd6,0x97]
; CHECK: sbiw r26,  63                 ; encoding: [0xdf,0x97]

; CHECK: sbiw r28,  52                 ; encoding: [0xe4,0x97]
; CHECK: sbiw r28,  0                  ; encoding: [0x20,0x97]

; CHECK: sbiw r30,  63                 ; encoding: [0xff,0x97]
; CHECK: sbiw r30,  47                 ; encoding: [0xbf,0x97]

; CHECK: sbiw r24,  1                  ; encoding: [0x01,0x97]
; CHECK: sbiw r24,  2                  ; encoding: [0x02,0x97]
; CHECK: sbiw r26,  3                  ; encoding: [0x13,0x97]
