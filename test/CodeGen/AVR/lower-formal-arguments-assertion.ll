; RUN: llc < %s -march=avr | FileCheck %s
; XFAIL:

; Test case for an assertion error.
; 
; Error:
; ```
; "LowerFormalArguments didn't emit the correct number of values!"
; ```
; in `lib/CodeGen/SelectionDAG/SelectionDAGBuilder.cpp`

define void @foo(i1) {
  ret void
}
