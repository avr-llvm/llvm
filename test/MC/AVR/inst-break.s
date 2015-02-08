; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  break

; CHECK: break                  ; encoding: [0x98,0x95]
