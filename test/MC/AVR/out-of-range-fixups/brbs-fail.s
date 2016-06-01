; RUN: not llvm-mc -triple avr -mattr=avr6 -filetype=obj < %s 2>&1 | FileCheck %s

; CHECK: error: out of range branch target (expected a integer in the range -128 to 127)
brbs 1, foo+128

