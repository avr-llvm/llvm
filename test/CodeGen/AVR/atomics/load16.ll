; RUN: llc -mattr=avr6 < %s -march=avr | FileCheck %s

; CHECK-LABEL: atomic_load16
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RR:r[0-9]+]], [[RD:(X|Y|Z)]]
; CHECK-NEXT: ldd [[RR:r[0-9]+]], [[RD:(X|Y|Z)]]+
; CHECK-NEXT: out 63, r0
define i16 @atomic_load16(i16* %foo) {
  %val = load atomic i16, i16* %foo unordered, align 2
  ret i16 %val
}

; CHECK-LABEL: atomic_load_add16
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RR1:r[0-9]+]], [[RD1:(X|Y|Z)]]
; CHECK-NEXT: ldd [[RR2:r[0-9]+]], [[RD2:(X|Y|Z)]]+
; CHECK-NEXT: add [[RR1]], [[TMP:r[0-9]+]]
; CHECK-NEXT: adc [[RR2]], [[TMP:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define i16 @atomic_load_add16(i16* %foo) {
  %val = atomicrmw add i16* %foo, i16 13 seq_cst
  ret i16 %val
}

; CHECK-LABEL: atomic_load_sub16
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RR1:r[0-9]+]], [[RD1:(X|Y|Z)]]
; CHECK-NEXT: ldd [[RR2:r[0-9]+]], [[RD2:(X|Y|Z)]]+
; CHECK-NEXT: sub [[RR1]], [[TMP:r[0-9]+]]
; CHECK-NEXT: sbc [[RR2]], [[TMP:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define i16 @atomic_load_sub16(i16* %foo) {
  %val = atomicrmw sub i16* %foo, i16 13 seq_cst
  ret i16 %val
}

; CHECK-LABEL: atomic_load_and16
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RR1:r[0-9]+]], [[RD1:(X|Y|Z)]]
; CHECK-NEXT: ldd [[RR2:r[0-9]+]], [[RD2:(X|Y|Z)]]+
; CHECK-NEXT: and [[RR1]], [[TMP:r[0-9]+]]
; CHECK-NEXT: and [[RR2]], [[TMP:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define i16 @atomic_load_and16(i16* %foo) {
  %val = atomicrmw and i16* %foo, i16 13 seq_cst
  ret i16 %val
}

; CHECK-LABEL: atomic_load_or16
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RR1:r[0-9]+]], [[RD1:(X|Y|Z)]]
; CHECK-NEXT: ldd [[RR2:r[0-9]+]], [[RD2:(X|Y|Z)]]+
; CHECK-NEXT: or [[RR1]], [[TMP:r[0-9]+]]
; CHECK-NEXT: or [[RR2]], [[TMP:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define i16 @atomic_load_or16(i16* %foo) {
  %val = atomicrmw or i16* %foo, i16 13 seq_cst
  ret i16 %val
}

; CHECK-LABEL: atomic_load_xor16
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: ld [[RR1:r[0-9]+]], [[RD1:(X|Y|Z)]]
; CHECK-NEXT: ldd [[RR2:r[0-9]+]], [[RD2:(X|Y|Z)]]+
; CHECK-NEXT: eor [[RR1]], [[TMP:r[0-9]+]]
; CHECK-NEXT: eor [[RR2]], [[TMP:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define i16 @atomic_load_xor16(i16* %foo) {
  %val = atomicrmw xor i16* %foo, i16 13 seq_cst
  ret i16 %val
}

