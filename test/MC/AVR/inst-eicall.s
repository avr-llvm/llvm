; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  eicall 

; CHECK: eicall                  ; encoding: [0x19,0x94]
