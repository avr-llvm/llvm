; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx,+fma -fp-contract=fast | FileCheck %s
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx,+fma4,+fma -fp-contract=fast | FileCheck %s
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx,+fma4 -fp-contract=fast | FileCheck %s --check-prefix=CHECK_FMA4

define <16 x float> @test_x86_fmadd_ps_y_wide(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
; CHECK-LABEL: test_x86_fmadd_ps_y_wide:
; CHECK:       # BB#0:
; CHECK-NEXT:    vfmadd213ps %ymm4, %ymm2, %ymm0
; CHECK-NEXT:    vfmadd213ps %ymm5, %ymm3, %ymm1
; CHECK-NEXT:    retq
;
; CHECK_FMA4-LABEL: test_x86_fmadd_ps_y_wide:
; CHECK_FMA4:       # BB#0:
; CHECK_FMA4-NEXT:    vfmaddps %ymm4, %ymm2, %ymm0, %ymm0
; CHECK_FMA4-NEXT:    vfmaddps %ymm5, %ymm3, %ymm1, %ymm1
; CHECK_FMA4-NEXT:    retq
  %x = fmul <16 x float> %a0, %a1
  %res = fadd <16 x float> %x, %a2
  ret <16 x float> %res
}

define <16 x float> @test_x86_fmsub_ps_y_wide(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
; CHECK-LABEL: test_x86_fmsub_ps_y_wide:
; CHECK:       # BB#0:
; CHECK-NEXT:    vfmsub213ps %ymm4, %ymm2, %ymm0
; CHECK-NEXT:    vfmsub213ps %ymm5, %ymm3, %ymm1
; CHECK-NEXT:    retq
;
; CHECK_FMA4-LABEL: test_x86_fmsub_ps_y_wide:
; CHECK_FMA4:       # BB#0:
; CHECK_FMA4-NEXT:    vfmsubps %ymm4, %ymm2, %ymm0, %ymm0
; CHECK_FMA4-NEXT:    vfmsubps %ymm5, %ymm3, %ymm1, %ymm1
; CHECK_FMA4-NEXT:    retq
  %x = fmul <16 x float> %a0, %a1
  %res = fsub <16 x float> %x, %a2
  ret <16 x float> %res
}

define <16 x float> @test_x86_fnmadd_ps_y_wide(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
; CHECK-LABEL: test_x86_fnmadd_ps_y_wide:
; CHECK:       # BB#0:
; CHECK-NEXT:    vfnmadd213ps %ymm4, %ymm2, %ymm0
; CHECK-NEXT:    vfnmadd213ps %ymm5, %ymm3, %ymm1
; CHECK-NEXT:    retq
;
; CHECK_FMA4-LABEL: test_x86_fnmadd_ps_y_wide:
; CHECK_FMA4:       # BB#0:
; CHECK_FMA4-NEXT:    vfnmaddps %ymm4, %ymm2, %ymm0, %ymm0
; CHECK_FMA4-NEXT:    vfnmaddps %ymm5, %ymm3, %ymm1, %ymm1
; CHECK_FMA4-NEXT:    retq
  %x = fmul <16 x float> %a0, %a1
  %res = fsub <16 x float> %a2, %x
  ret <16 x float> %res
}

define <16 x float> @test_x86_fnmsub_ps_y_wide(<16 x float> %a0, <16 x float> %a1, <16 x float> %a2) {
; CHECK-LABEL: test_x86_fnmsub_ps_y_wide:
; CHECK:       # BB#0:
; CHECK-NEXT:    vfnmsub213ps %ymm4, %ymm2, %ymm0
; CHECK-NEXT:    vfnmsub213ps %ymm5, %ymm3, %ymm1
; CHECK-NEXT:    retq
;
; CHECK_FMA4-LABEL: test_x86_fnmsub_ps_y_wide:
; CHECK_FMA4:       # BB#0:
; CHECK_FMA4-NEXT:    vfnmsubps %ymm4, %ymm2, %ymm0, %ymm0
; CHECK_FMA4-NEXT:    vfnmsubps %ymm5, %ymm3, %ymm1, %ymm1
; CHECK_FMA4-NEXT:    retq
  %x = fmul <16 x float> %a0, %a1
  %y = fsub <16 x float> <float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00, float -0.000000e+00>, %x
  %res = fsub <16 x float> %y, %a2
  ret <16 x float> %res
}

define <8 x double> @test_x86_fmadd_pd_y_wide(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
; CHECK-LABEL: test_x86_fmadd_pd_y_wide:
; CHECK:       # BB#0:
; CHECK-NEXT:    vfmadd213pd %ymm4, %ymm2, %ymm0
; CHECK-NEXT:    vfmadd213pd %ymm5, %ymm3, %ymm1
; CHECK-NEXT:    retq
;
; CHECK_FMA4-LABEL: test_x86_fmadd_pd_y_wide:
; CHECK_FMA4:       # BB#0:
; CHECK_FMA4-NEXT:    vfmaddpd %ymm4, %ymm2, %ymm0, %ymm0
; CHECK_FMA4-NEXT:    vfmaddpd %ymm5, %ymm3, %ymm1, %ymm1
; CHECK_FMA4-NEXT:    retq
  %x = fmul <8 x double> %a0, %a1
  %res = fadd <8 x double> %x, %a2
  ret <8 x double> %res
}

define <8 x double> @test_x86_fmsub_pd_y_wide(<8 x double> %a0, <8 x double> %a1, <8 x double> %a2) {
; CHECK-LABEL: test_x86_fmsub_pd_y_wide:
; CHECK:       # BB#0:
; CHECK-NEXT:    vfmsub213pd %ymm4, %ymm2, %ymm0
; CHECK-NEXT:    vfmsub213pd %ymm5, %ymm3, %ymm1
; CHECK-NEXT:    retq
;
; CHECK_FMA4-LABEL: test_x86_fmsub_pd_y_wide:
; CHECK_FMA4:       # BB#0:
; CHECK_FMA4-NEXT:    vfmsubpd %ymm4, %ymm2, %ymm0, %ymm0
; CHECK_FMA4-NEXT:    vfmsubpd %ymm5, %ymm3, %ymm1, %ymm1
; CHECK_FMA4-NEXT:    retq
  %x = fmul <8 x double> %a0, %a1
  %res = fsub <8 x double> %x, %a2
  ret <8 x double> %res
}
