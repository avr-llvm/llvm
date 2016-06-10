; RUN: llc -mattr=avr6 < %s -march=avr | FileCheck %s

; CHECK-LABEL: atomic_store8
; CHECK:      in r0, 63
; CHECK-NEXT: cli
; CHECK-NEXT: st [[RD:(X|Y|Z)]], [[RR:r[0-9]+]]
; CHECK-NEXT: out 63, r0
define void @atomic_store8(i8* %foo) {
  store atomic i8 1, i8* %foo unordered, align 1
  ret void
}
