; RUN: llc < %s -asm-verbose=false | FileCheck %s

; Test that basic 32-bit integer operations assemble as expected.

target datalayout = "e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-unknown"

declare i32 @llvm.ctlz.i32(i32, i1)
declare i32 @llvm.cttz.i32(i32, i1)
declare i32 @llvm.ctpop.i32(i32)

; CHECK-LABEL: add32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: add (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @add32(i32 %x, i32 %y) {
  %a = add i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: sub32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: sub (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @sub32(i32 %x, i32 %y) {
  %a = sub i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: mul32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: mul (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @mul32(i32 %x, i32 %y) {
  %a = mul i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: sdiv32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: div_s (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @sdiv32(i32 %x, i32 %y) {
  %a = sdiv i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: udiv32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: div_u (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @udiv32(i32 %x, i32 %y) {
  %a = udiv i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: srem32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: rem_s (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @srem32(i32 %x, i32 %y) {
  %a = srem i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: urem32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: rem_u (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @urem32(i32 %x, i32 %y) {
  %a = urem i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: and32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: and (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @and32(i32 %x, i32 %y) {
  %a = and i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: ior32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: ior (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @ior32(i32 %x, i32 %y) {
  %a = or i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: xor32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: xor (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @xor32(i32 %x, i32 %y) {
  %a = xor i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: shl32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: shl (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @shl32(i32 %x, i32 %y) {
  %a = shl i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: shr32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: shr_u (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @shr32(i32 %x, i32 %y) {
  %a = lshr i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: sar32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32, i32{{$}}
; CHECK-NEXT: get_local 1{{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 3, pop{{$}}
; CHECK-NEXT: shr_s (get_local 3), (get_local 2){{$}}
; CHECK-NEXT: set_local 4, pop{{$}}
; CHECK-NEXT: return (get_local 4){{$}}
define i32 @sar32(i32 %x, i32 %y) {
  %a = ashr i32 %x, %y
  ret i32 %a
}

; CHECK-LABEL: clz32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 1, pop{{$}}
; CHECK-NEXT: clz (get_local 1){{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: return (get_local 2){{$}}
define i32 @clz32(i32 %x) {
  %a = call i32 @llvm.ctlz.i32(i32 %x, i1 false)
  ret i32 %a
}

; CHECK-LABEL: clz32_zero_undef:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 1, pop{{$}}
; CHECK-NEXT: clz (get_local 1){{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: return (get_local 2){{$}}
define i32 @clz32_zero_undef(i32 %x) {
  %a = call i32 @llvm.ctlz.i32(i32 %x, i1 true)
  ret i32 %a
}

; CHECK-LABEL: ctz32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 1, pop{{$}}
; CHECK-NEXT: ctz (get_local 1){{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: return (get_local 2){{$}}
define i32 @ctz32(i32 %x) {
  %a = call i32 @llvm.cttz.i32(i32 %x, i1 false)
  ret i32 %a
}

; CHECK-LABEL: ctz32_zero_undef:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 1, pop{{$}}
; CHECK-NEXT: ctz (get_local 1){{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: return (get_local 2){{$}}
define i32 @ctz32_zero_undef(i32 %x) {
  %a = call i32 @llvm.cttz.i32(i32 %x, i1 true)
  ret i32 %a
}

; CHECK-LABEL: popcnt32:
; CHECK-NEXT: .param i32{{$}}
; CHECK-NEXT: .result i32{{$}}
; CHECK-NEXT: .local i32, i32{{$}}
; CHECK-NEXT: get_local 0{{$}}
; CHECK-NEXT: set_local 1, pop{{$}}
; CHECK-NEXT: popcnt (get_local 1){{$}}
; CHECK-NEXT: set_local 2, pop{{$}}
; CHECK-NEXT: return (get_local 2){{$}}
define i32 @popcnt32(i32 %x) {
  %a = call i32 @llvm.ctpop.i32(i32 %x)
  ret i32 %a
}
