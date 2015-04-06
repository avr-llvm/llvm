; RUN: llvm-mc -triple avr-none -mattr=eijmpcall -show-encoding < %s | FileCheck %s


foo:

  eijmp 

; CHECK: eijmp                  ; encoding: [0x19,0x94]
