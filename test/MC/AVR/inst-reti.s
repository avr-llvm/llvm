; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  reti

; CHECK: reti                  ; encoding: [0x18,0x95]
