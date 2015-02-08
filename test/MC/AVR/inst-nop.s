; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  nop

; CHECK: nop                  ; encoding: [0x00,0x00]
