; RUN: not llvm-mc -triple avr -mattr=avr6 -filetype=obj < %s 2>&1 | FileCheck %s

; CHECK: error: out of range port number (expected a integer in the range 0 to 31)
sbi foo+32, 1

