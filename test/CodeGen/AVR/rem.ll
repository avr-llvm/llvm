; RUN: llc -mattr=mul,movw < %s -march=avr | FileCheck %s

; Unsigned 8-bit remision
define i8 @urem8(i8 %a, i8 %b) {
; CHECK-LABEL: rem8:
; CHECK: call __udivmodqi4
; CHECK: mov r24, r25
; CHECK: ret

  %rem = urem i8 %a, %b
  ret i8 %rem
}

; Signed 8-bit remision
define i8 @srem8(i8 %a, i8 %b) {
; CHECK-LABEL: srem8:
; CHECK: call __divmodqi4
; CHECK: mov r24, r25
; CHECK: ret

  %rem = srem i8 %a, %b
  ret i8 %rem
}

; Unsigned 16-bit remision
define i16 @urem16(i16 %a, i16 %b) {
; CHECK-LABEL: urem16:
; CHECK: call __udivmodhi4
; CHECK: ret
  %rem = urem i16 %a, %b
  ret i16 %rem
}

; Signed 16-bit remision
define i16 @srem16(i16 %a, i16 %b) {
; CHECK-LABEL: srem16:
; CHECK: call __divmodhi4
; CHECK: ret
  %rem = srem i16 %a, %b
  ret i16 %rem
}

