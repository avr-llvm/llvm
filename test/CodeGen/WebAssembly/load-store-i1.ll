; RUN: llc < %s -asm-verbose=false | FileCheck %s

; Test that i1 extending loads and truncating stores are assembled properly.

target datalayout = "e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-unknown"

; CHECK-LABEL: (func $load_u_i1_i32
; CHECK:      (set_local @1 (load_u_i8_i32 @0))
; CHECK-NEXT: (return @1)
define i32 @load_u_i1_i32(i1* %p) {
  %v = load i1, i1* %p
  %e = zext i1 %v to i32
  ret i32 %e
}

; CHECK-LABEL: (func $load_s_i1_i32
; CHECK:      (set_local @1 (load_u_i8_i32 @0))
; CHECK-NEXT: (set_local @2 (immediate 31))
; CHECK-NEXT: (set_local @3 (shl @1 @2))
; CHECK-NEXT: (set_local @4 (shr_s @3 @2))
; CHECK-NEXT: (return @4)
define i32 @load_s_i1_i32(i1* %p) {
  %v = load i1, i1* %p
  %e = sext i1 %v to i32
  ret i32 %e
}

; CHECK-LABEL: (func $load_u_i1_i64
; CHECK:      (set_local @1 (load_u_i8_i64 @0))
; CHECK-NEXT: (return @1)
define i64 @load_u_i1_i64(i1* %p) {
  %v = load i1, i1* %p
  %e = zext i1 %v to i64
  ret i64 %e
}

; CHECK-LABEL: (func $load_s_i1_i64
; CHECK:      (set_local @1 (load_u_i8_i64 @0))
; CHECK-NEXT: (set_local @2 (immediate 63))
; CHECK-NEXT: (set_local @3 (shl @1 @2))
; CHECK-NEXT: (set_local @4 (shr_s @3 @2))
; CHECK-NEXT: (return @4)
define i64 @load_s_i1_i64(i1* %p) {
  %v = load i1, i1* %p
  %e = sext i1 %v to i64
  ret i64 %e
}

; CHECK-LABEL: (func $store_i32_i1
; CHECK:      (set_local @2 (immediate 1))
; CHECK-NEXT: (set_local @3 (and @1 @2))
; CHECK-NEXT: (store_i8 @0 @3)
define void @store_i32_i1(i1* %p, i32 %v) {
  %t = trunc i32 %v to i1
  store i1 %t, i1* %p
  ret void
}

; CHECK-LABEL: (func $store_i64_i1
; CHECK:      (set_local @2 (immediate 1))
; CHECK-NEXT: (set_local @3 (and @1 @2))
; CHECK-NEXT: (store_i8 @0 @3)
define void @store_i64_i1(i1* %p, i64 %v) {
  %t = trunc i64 %v to i1
  store i1 %t, i1* %p
  ret void
}
