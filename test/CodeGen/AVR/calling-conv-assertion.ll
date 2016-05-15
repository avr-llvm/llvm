; RUN: llc < %s -march=avr | FileCheck %s
; XFAIL: *

; Test case for an assertion error.
;
; Error:
; ```
; Call result #4 has unhandled type i16
; UNREACHABLE executed at lib/CodeGen/CallingConvLower.cpp
; ```

define void @foo(i64*, i64) {
  %remainder = srem i64 0, %1
  store i64 %remainder, i64* %0, align 1
  ret void
}
