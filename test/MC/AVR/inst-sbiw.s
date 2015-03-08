; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  sbiw X,  54
  sbiw X,  63
  
  sbiw Y,  52
  sbiw Y,  0
  
  sbiw Z,  63
  sbiw Z,  47

; CHECK: sbiw r26,  54                 ; encoding: [0xd6,0x97]
; CHECK: sbiw r26,  63                 ; encoding: [0xdf,0x97]

; CHECK: sbiw r28,  52                 ; encoding: [0xe4,0x97]
; CHECK: sbiw r28,  0                  ; encoding: [0x20,0x97]

; CHECK: sbiw r30,  63                 ; encoding: [0xff,0x97]
; CHECK: sbiw r30,  47                 ; encoding: [0xbf,0x97]
