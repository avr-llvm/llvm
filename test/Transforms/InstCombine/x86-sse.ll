; RUN: opt < %s -instcombine -S | FileCheck %s
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define float @test_rcp_ss_0(float %a) {
; CHECK-LABEL: @test_rcp_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
; CHECK-NEXT: %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
; CHECK-NEXT: %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
; CHECK-NEXT: %5 = tail call <4 x float> @llvm.x86.sse.rcp.ss(<4 x float> %4)
; CHECK-NEXT: %6 = extractelement <4 x float> %5, i32 0
; CHECK-NEXT: ret float %6
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = tail call <4 x float> @llvm.x86.sse.rcp.ss(<4 x float> %4)
  %6 = extractelement <4 x float> %5, i32 0
  ret float %6
}

define float @test_sqrt_ss_0(float %a) {
; CHECK-LABEL: @test_sqrt_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
; CHECK-NEXT: %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
; CHECK-NEXT: %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
; CHECK-NEXT: %5 = tail call <4 x float> @llvm.x86.sse.sqrt.ss(<4 x float> %4)
; CHECK-NEXT: %6 = extractelement <4 x float> %5, i32 0
; CHECK-NEXT: ret float %6
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = tail call <4 x float> @llvm.x86.sse.sqrt.ss(<4 x float> %4)
  %6 = extractelement <4 x float> %5, i32 0
  ret float %6
}

define float @test_rsqrt_ss_0(float %a) {
; CHECK-LABEL: @test_rsqrt_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
; CHECK-NEXT: %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
; CHECK-NEXT: %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
; CHECK-NEXT: %5 = tail call <4 x float> @llvm.x86.sse.rsqrt.ss(<4 x float> %4)
; CHECK-NEXT: %6 = extractelement <4 x float> %5, i32 0
; CHECK-NEXT: ret float %6
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = tail call <4 x float> @llvm.x86.sse.rsqrt.ss(<4 x float> %4)
  %6 = extractelement <4 x float> %5, i32 0
  ret float %6
}

define float @test_add_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_add_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
; CHECK-NEXT: %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
; CHECK-NEXT: %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
; CHECK-NEXT: %5 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
; CHECK-NEXT: %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
; CHECK-NEXT: %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
; CHECK-NEXT: %9 = tail call <4 x float> @llvm.x86.sse.add.ss(<4 x float> %4, <4 x float> %8)
; CHECK-NEXT: %r = extractelement <4 x float> %9, i32 0
; CHECK-NEXT: ret float %r
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call <4 x float> @llvm.x86.sse.add.ss(<4 x float> %4, <4 x float> %8)
  %r = extractelement <4 x float> %9, i32 0
  ret float %r
}

define float @test_sub_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_sub_ss_0
; CHECK-NEXT: %1 = fsub float %a, %b
; CHECK-NEXT: ret float %1
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call <4 x float> @llvm.x86.sse.sub.ss(<4 x float> %4, <4 x float> %8)
  %r = extractelement <4 x float> %9, i32 0
  ret float %r
}

define float @test_mul_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_mul_ss_0
; CHECK-NEXT: %1 = fmul float %a, %b
; CHECK-NEXT: ret float %1
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call <4 x float> @llvm.x86.sse.mul.ss(<4 x float> %4, <4 x float> %8)
  %r = extractelement <4 x float> %9, i32 0
  ret float %r
}

define float @test_div_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_div_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
; CHECK-NEXT: %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
; CHECK-NEXT: %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
; CHECK-NEXT: %5 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
; CHECK-NEXT: %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
; CHECK-NEXT: %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
; CHECK-NEXT: %9 = tail call <4 x float> @llvm.x86.sse.div.ss(<4 x float> %4, <4 x float> %8)
; CHECK-NEXT: %r = extractelement <4 x float> %9, i32 0
; CHECK-NEXT: ret float %r
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call <4 x float> @llvm.x86.sse.div.ss(<4 x float> %4, <4 x float> %8)
  %r = extractelement <4 x float> %9, i32 0
  ret float %r
}

define float @test_min_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_min_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call <4 x float> @llvm.x86.sse.min.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: %4 = extractelement <4 x float> %3, i32 0
; CHECK-NEXT: ret float %4
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call <4 x float> @llvm.x86.sse.min.ss(<4 x float> %4, <4 x float> %8)
  %10 = extractelement <4 x float> %9, i32 0
  ret float %10
}

define float @test_max_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_max_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call <4 x float> @llvm.x86.sse.max.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: %4 = extractelement <4 x float> %3, i32 0
; CHECK-NEXT: ret float %4
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call <4 x float> @llvm.x86.sse.max.ss(<4 x float> %4, <4 x float> %8)
  %10 = extractelement <4 x float> %9, i32 0
  ret float %10
}

define float @test_cmp_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_cmp_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
; CHECK-NEXT: %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
; CHECK-NEXT: %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
; CHECK-NEXT: %5 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
; CHECK-NEXT: %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
; CHECK-NEXT: %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
; CHECK-NEXT: %9 = tail call <4 x float> @llvm.x86.sse.cmp.ss(<4 x float> %4, <4 x float> %8, i8 0)
; CHECK-NEXT: %r = extractelement <4 x float> %9, i32 0
; CHECK-NEXT: ret float %r
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call <4 x float> @llvm.x86.sse.cmp.ss(<4 x float> %4, <4 x float> %8, i8 0)
  %r = extractelement <4 x float> %9, i32 0
  ret float %r
}

define i32 @test_comieq_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_comieq_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.comieq.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.comieq.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_comige_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_comige_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.comige.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.comige.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_comigt_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_comigt_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.comigt.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.comigt.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_comile_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_comile_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.comile.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.comile.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_comilt_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_comilt_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.comilt.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.comilt.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_comineq_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_comineq_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.comineq.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.comineq.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_ucomieq_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_ucomieq_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.ucomieq.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.ucomieq.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_ucomige_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_ucomige_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.ucomige.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.ucomige.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_ucomigt_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_ucomigt_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.ucomigt.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.ucomigt.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_ucomile_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_ucomile_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.ucomile.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.ucomile.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_ucomilt_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_ucomilt_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.ucomilt.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.ucomilt.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

define i32 @test_ucomineq_ss_0(float %a, float %b) {
; CHECK-LABEL: @test_ucomineq_ss_0
; CHECK-NEXT: %1 = insertelement <4 x float> undef, float %a, i32 0
; CHECK-NEXT: %2 = insertelement <4 x float> undef, float %b, i32 0
; CHECK-NEXT: %3 = tail call i32 @llvm.x86.sse.ucomineq.ss(<4 x float> %1, <4 x float> %2)
; CHECK-NEXT: ret i32 %3
  %1 = insertelement <4 x float> undef, float %a, i32 0
  %2 = insertelement <4 x float> %1, float 1.000000e+00, i32 1
  %3 = insertelement <4 x float> %2, float 2.000000e+00, i32 2
  %4 = insertelement <4 x float> %3, float 3.000000e+00, i32 3
  %5 = insertelement <4 x float> undef, float %b, i32 0
  %6 = insertelement <4 x float> %5, float 4.000000e+00, i32 1
  %7 = insertelement <4 x float> %6, float 5.000000e+00, i32 2
  %8 = insertelement <4 x float> %7, float 6.000000e+00, i32 3
  %9 = tail call i32 @llvm.x86.sse.ucomineq.ss(<4 x float> %4, <4 x float> %8)
  ret i32 %9
}

declare <4 x float> @llvm.x86.sse.rcp.ss(<4 x float>)
declare <4 x float> @llvm.x86.sse.sqrt.ss(<4 x float>)
declare <4 x float> @llvm.x86.sse.rsqrt.ss(<4 x float>)

declare <4 x float> @llvm.x86.sse.add.ss(<4 x float>, <4 x float>)
declare <4 x float> @llvm.x86.sse.sub.ss(<4 x float>, <4 x float>)
declare <4 x float> @llvm.x86.sse.mul.ss(<4 x float>, <4 x float>)
declare <4 x float> @llvm.x86.sse.div.ss(<4 x float>, <4 x float>)
declare <4 x float> @llvm.x86.sse.min.ss(<4 x float>, <4 x float>)
declare <4 x float> @llvm.x86.sse.max.ss(<4 x float>, <4 x float>)
declare <4 x float> @llvm.x86.sse.cmp.ss(<4 x float>, <4 x float>, i8)

declare i32 @llvm.x86.sse.comieq.ss(<4 x float>, <4 x float>)
declare i32 @llvm.x86.sse.comige.ss(<4 x float>, <4 x float>)
declare i32 @llvm.x86.sse.comigt.ss(<4 x float>, <4 x float>)
declare i32 @llvm.x86.sse.comile.ss(<4 x float>, <4 x float>)
declare i32 @llvm.x86.sse.comilt.ss(<4 x float>, <4 x float>)
declare i32 @llvm.x86.sse.comineq.ss(<4 x float>, <4 x float>)

declare i32 @llvm.x86.sse.ucomieq.ss(<4 x float>, <4 x float>)
declare i32 @llvm.x86.sse.ucomige.ss(<4 x float>, <4 x float>)
declare i32 @llvm.x86.sse.ucomigt.ss(<4 x float>, <4 x float>)
declare i32 @llvm.x86.sse.ucomile.ss(<4 x float>, <4 x float>)
declare i32 @llvm.x86.sse.ucomilt.ss(<4 x float>, <4 x float>)
declare i32 @llvm.x86.sse.ucomineq.ss(<4 x float>, <4 x float>)
