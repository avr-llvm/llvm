; RUN: llc -mattr=mul,movw < %s -march=avr | FileCheck %s

; Unsigned 8-bit remision
define i8 @urem8(i8 %a, i8 %b) {
; CHECK-LABEL: rem8:
; CHECK: mov r25, r24
; CHECK: mov r24, r22
; CHECK: mov r22, r25
; CHECK: call __udivmodqi4
; CHECK: mov r24, r25
; CHECK: ret

  %rem = urem i8 %b, %a
  ret i8 %rem
}

; Signed 8-bit remision
define i8 @srem8(i8 %a, i8 %b) {
; CHECK-LABEL: srem8:
; CHECK: mov r25, r24
; CHECK: mov r24, r22
; CHECK: mov r22, r25
; CHECK: call __divmodqi4
; CHECK: mov r24, r25
; CHECK: ret

  %rem = srem i8 %b, %a
  ret i8 %rem
}

; Unsigned 16-bit remision
define i16 @urem16(i16 %a, i16 %b) {
; CHECK-LABEL: urem16:
; CHECK: movw r18, r24
; CHECK: movw r24, r22
; CHECK: movw r22, r18
; CHECK: call __udivmodhi4
; CHECK: ret
  %rem = urem i16 %b, %a
  ret i16 %rem
}

; Signed 16-bit remision
define i16 @srem16(i16 %a, i16 %b) {
; CHECK-LABEL: srem16:
; CHECK: movw r18, r24
; CHECK: movw r24, r22
; CHECK: movw r22, r18
; CHECK: call __divmodhi4
; CHECK: ret
  %rem = srem i16 %b, %a
  ret i16 %rem
}

