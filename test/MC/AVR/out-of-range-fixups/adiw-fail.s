; RUN: not llvm-mc -triple avr -mattr=avr6 -filetype=obj < %s 2>&1 | FileCheck %s

; CHECK: error: out of range immediate (expected a integer in the range 0 to 63)
adiw r24, foo+64

