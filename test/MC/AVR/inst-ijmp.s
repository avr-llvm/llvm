; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  ijmp 

; CHECK: ijmp                  ; encoding: [0x09,0x94]
