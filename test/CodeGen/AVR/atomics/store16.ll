; RUN: llc -mattr=avr6 < %s -march=avr | FileCheck %s

; CHECK-LABEL: atomic_store16
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: st [[RD:(X|Y|Z)]], [[RR:r[0-9]+]]
; CHECK-NEXT: std [[RD:(X|Y|Z)]]+1, [[RR:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define void @atomic_store16(i16* %foo) {
  store atomic i16 1, i16* %foo unordered, align 2
  ret void
}

