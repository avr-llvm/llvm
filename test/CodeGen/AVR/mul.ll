; RUN: llc < %s -march=avr | FileCheck %s

define i8 @mult8(i8 %a, i8 %b) {
; CHECK-LABEL: mult8:
; CHECK: mul r22, r24
; CHECK: mov r24, r0
; :TODO: clr r1
  %mul = mul i8 %b, %a
  ret i8 %mul
}

define i16 @mult16(i16 %a, i16 %b) {
; CHECK-LABEL: mult16:
; CHECK: mul r22, r24
; CHECK: movw r18, r0
; CHECK: mul r22, r25
; CHECK: add r19, r0
; CHECK: mul r23, r24
; CHECK: add r19, r0
; CHECK: movw r24, r18
; :TODO: clr r1
  %mul = mul nsw i16 %b, %a
  ret i16 %mul
}
