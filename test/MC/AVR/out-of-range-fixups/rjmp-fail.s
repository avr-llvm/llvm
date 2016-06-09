; RUN: not llvm-mc -triple avr -mattr=avr6 -filetype=obj < %s 2>&1 | FileCheck %s

; CHECK: error: out of range branch target (expected an integer in the range -4096 to 4095)
rjmp foo+4096
