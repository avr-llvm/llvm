; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  wdr

; CHECK: wdr                  ; encoding: [0xa8,0x95]
