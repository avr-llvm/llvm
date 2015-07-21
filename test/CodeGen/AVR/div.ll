; RUN: llc -mattr=mul,movw < %s -march=avr | FileCheck %s

; Unsigned 8-bit division
define i8 @udiv8(i8 %a, i8 %b) {
; CHECK-LABEL: div8:
; CHECK: call __udivmodqi4
; CHECK: ret

  %quotient = udiv i8 %a, %b
  ret i8 %quotient
}

; Signed 8-bit division
define i8 @sdiv8(i8 %a, i8 %b) {
; CHECK-LABEL: sdiv8:
; CHECK: call __divmodqi4
; CHECK: ret

  %quotient = sdiv i8 %a, %b
  ret i8 %quotient
}

; Unsigned 16-bit division
define i16 @udiv16(i16 %a, i16 %b) {
; CHECK-LABEL: udiv16:
; CHECK: call __udivmodhi4
; CHECK: movw r24, r22
; CHECK: ret
  %quot = udiv i16 %a, %b
  ret i16 %quot
}

; Signed 16-bit division
define i16 @sdiv16(i16 %a, i16 %b) {
; CHECK-LABEL: sdiv16:
; CHECK: call __divmodhi4
; CHECK: movw r24, r22
; CHECK: ret
  %quot = sdiv i16 %a, %b
  ret i16 %quot
}

