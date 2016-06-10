; RUN: llc -mattr=avr6 < %s -march=avr | FileCheck %s

; Tests atomic operations on AVR

; CHECK-LABEL: atomic_load8
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RR:r[0-9]+]], [[RD:(X|Y|Z)]]
; CHECK-NEXT: out 63, r0
define i8 @atomic_load8(i8* %foo) {
  %val = load atomic i8, i8* %foo unordered, align 1
  ret i8 %val
}

; CHECK-LABEL: atomic_load_add8
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RD:r[0-9]+]], [[RR:(X|Y|Z)]]
; CHECK-NEXT: add [[RD]], [[RR:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define i8 @atomic_load_add8(i8* %foo) {
  %val = atomicrmw add i8* %foo, i8 13 seq_cst
  ret i8 %val
}

; CHECK-LABEL: atomic_load_sub8
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RD:r[0-9]+]], [[RR:(X|Y|Z)]]
; CHECK-NEXT: sub [[RD]], [[RR:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define i8 @atomic_load_sub8(i8* %foo) {
  %val = atomicrmw sub i8* %foo, i8 13 seq_cst
  ret i8 %val
}

; CHECK-LABEL: atomic_load_and8
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RD:r[0-9]+]], [[RR:(X|Y|Z)]]
; CHECK-NEXT: and [[RD]], [[RR:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define i8 @atomic_load_and8(i8* %foo) {
  %val = atomicrmw and i8* %foo, i8 13 seq_cst
  ret i8 %val
}

; CHECK-LABEL: atomic_load_or8
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RD:r[0-9]+]], [[RR:(X|Y|Z)]]
; CHECK-NEXT: or [[RD]], [[RR:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define i8 @atomic_load_or8(i8* %foo) {
  %val = atomicrmw or i8* %foo, i8 13 seq_cst
  ret i8 %val
}

; CHECK-LABEL: atomic_load_xor8
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RD:r[0-9]+]], [[RR:(X|Y|Z)]]
; CHECK-NEXT: eor [[RD]], [[RR:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define i8 @atomic_load_xor8(i8* %foo) {
  %val = atomicrmw xor i8* %foo, i8 13 seq_cst
  ret i8 %val
}

