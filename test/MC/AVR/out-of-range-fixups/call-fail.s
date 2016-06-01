; RUN: not llvm-mc -triple avr -mattr=avr6 -filetype=obj < %s 2>&1 | FileCheck %s

; CHECK: error: out of range branch target (expected a integer in the range 0 to 8388607)
jmp foo+8388608
