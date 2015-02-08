; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  ser r16 
  ser r31
  ser r27
  ser r31

; CHECK: ser r16                   ; encoding: [0x0f,0xef]
; CHECK: ser r31                   ; encoding: [0xff,0xef]
; CHECK: ser r27                   ; encoding: [0xbf,0xef]
; CHECK: ser r31                   ; encoding: [0xff,0xef]
