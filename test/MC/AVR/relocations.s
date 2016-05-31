; RUN: llvm-mc -filetype=obj -triple=avr %s | llvm-objdump -r - | FileCheck %s

; CHECK: RELOCATION RECORDS FOR

; CHECK-NEXT: R_AVR_LDI SYMBOL+3
ldi r21, SYMBOL+3

; adiw r24, FOO
