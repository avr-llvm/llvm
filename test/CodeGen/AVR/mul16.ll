; RUN: llc -mcpu=atmega328p -O0  < %s -march=avr | FileCheck %s
; XFAIL: *

; TODO: fix this test.

; This test should be moved into a seperate file once it passes.
; Note that this file causes a different error when compiled with
; optimizations enabled.

define i32 @_Z3maplllll(i32 %x, i32 %in_min, i32 %in_max, i32 %out_min, i32 %out_max) {
entry:
  %sub = sub nsw i32 %x, %in_min
  %sub1 = sub nsw i32 %out_max, %out_min
  %mul =  mul nsw i32 %sub1, %sub
  %sub2 = sub nsw i32 %in_max, %in_min
  %div = sdiv i32 %mul, %sub2
  %add = add nsw i32 %div, %out_min
  ret i32 %add
}

