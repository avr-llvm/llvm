; RUN: llc < %s -mtriple=x86_64-unknown-unknown -march=x86-64 -mcpu=corei7-avx -mattr=+fma | FileCheck %s --check-prefix=CHECK --check-prefix=CHECK-FMA
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -march=x86-64 -mcpu=core-avx2 -mattr=+fma,+avx2 | FileCheck %s --check-prefix=CHECK --check-prefix=CHECK-FMA
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -march=x86-64 -mcpu=corei7-avx -mattr=+fma4 | FileCheck %s --check-prefix=CHECK --check-prefix=CHECK-FMA4
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=bdver2 -mattr=+avx,-fma | FileCheck %s --check-prefix=CHECK --check-prefix=CHECK-FMA4
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=bdver2 -mattr=-fma4 | FileCheck %s --check-prefix=CHECK --check-prefix=CHECK-FMA

; VFMADD
define <4 x float> @test_x86_fma_vfmadd_ss(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmadd_ss:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmadd213ss %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmadd_ss:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmaddss %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x float> @llvm.x86.fma.vfmadd.ss(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.fma.vfmadd.ss(<4 x float>, <4 x float>, <4 x float>)

define <2 x double> @test_x86_fma_vfmadd_sd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmadd_sd:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmadd213sd %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmadd_sd:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmaddsd %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <2 x double> @llvm.x86.fma.vfmadd.sd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2)
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.fma.vfmadd.sd(<2 x double>, <2 x double>, <2 x double>)

define <4 x float> @test_x86_fma_vfmadd_ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmadd_ps:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmadd213ps %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmadd_ps:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmaddps %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x float> @llvm.x86.fma.vfmadd.ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.fma.vfmadd.ps(<4 x float>, <4 x float>, <4 x float>)

define <2 x double> @test_x86_fma_vfmadd_pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmadd_pd:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmadd213pd %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmadd_pd:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmaddpd %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <2 x double> @llvm.x86.fma.vfmadd.pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2)
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.fma.vfmadd.pd(<2 x double>, <2 x double>, <2 x double>)

define <8 x float> @test_x86_fma_vfmadd_ps_256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmadd_ps_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmadd213ps %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmadd_ps_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmaddps %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <8 x float> @llvm.x86.fma.vfmadd.ps.256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2)
  ret <8 x float> %res
}
declare <8 x float> @llvm.x86.fma.vfmadd.ps.256(<8 x float>, <8 x float>, <8 x float>)

define <4 x double> @test_x86_fma_vfmadd_pd_256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmadd_pd_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmadd213pd %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmadd_pd_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmaddpd %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x double> @llvm.x86.fma.vfmadd.pd.256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2)
  ret <4 x double> %res
}
declare <4 x double> @llvm.x86.fma.vfmadd.pd.256(<4 x double>, <4 x double>, <4 x double>)

; VFMSUB
define <4 x float> @test_x86_fma_vfmsub_ss(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmsub_ss:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmsub213ss %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmsub_ss:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmsubss %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x float> @llvm.x86.fma.vfmsub.ss(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.fma.vfmsub.ss(<4 x float>, <4 x float>, <4 x float>)

define <2 x double> @test_x86_fma_vfmsub_sd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmsub_sd:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmsub213sd %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmsub_sd:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmsubsd %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <2 x double> @llvm.x86.fma.vfmsub.sd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2)
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.fma.vfmsub.sd(<2 x double>, <2 x double>, <2 x double>)

define <4 x float> @test_x86_fma_vfmsub_ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmsub_ps:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmsub213ps %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmsub_ps:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmsubps %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x float> @llvm.x86.fma.vfmsub.ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.fma.vfmsub.ps(<4 x float>, <4 x float>, <4 x float>)

define <2 x double> @test_x86_fma_vfmsub_pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmsub_pd:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmsub213pd %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmsub_pd:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmsubpd %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <2 x double> @llvm.x86.fma.vfmsub.pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2)
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.fma.vfmsub.pd(<2 x double>, <2 x double>, <2 x double>)

define <8 x float> @test_x86_fma_vfmsub_ps_256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmsub_ps_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmsub213ps %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmsub_ps_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmsubps %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <8 x float> @llvm.x86.fma.vfmsub.ps.256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2)
  ret <8 x float> %res
}
declare <8 x float> @llvm.x86.fma.vfmsub.ps.256(<8 x float>, <8 x float>, <8 x float>)

define <4 x double> @test_x86_fma_vfmsub_pd_256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmsub_pd_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmsub213pd %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmsub_pd_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmsubpd %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x double> @llvm.x86.fma.vfmsub.pd.256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2)
  ret <4 x double> %res
}
declare <4 x double> @llvm.x86.fma.vfmsub.pd.256(<4 x double>, <4 x double>, <4 x double>)

; VFNMADD
define <4 x float> @test_x86_fma_vfnmadd_ss(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmadd_ss:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmadd213ss %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmadd_ss:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmaddss %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x float> @llvm.x86.fma.vfnmadd.ss(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.fma.vfnmadd.ss(<4 x float>, <4 x float>, <4 x float>)

define <2 x double> @test_x86_fma_vfnmadd_sd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmadd_sd:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmadd213sd %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmadd_sd:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmaddsd %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <2 x double> @llvm.x86.fma.vfnmadd.sd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2)
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.fma.vfnmadd.sd(<2 x double>, <2 x double>, <2 x double>)

define <4 x float> @test_x86_fma_vfnmadd_ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmadd_ps:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmadd213ps %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmadd_ps:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmaddps %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x float> @llvm.x86.fma.vfnmadd.ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.fma.vfnmadd.ps(<4 x float>, <4 x float>, <4 x float>)

define <2 x double> @test_x86_fma_vfnmadd_pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmadd_pd:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmadd213pd %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmadd_pd:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmaddpd %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <2 x double> @llvm.x86.fma.vfnmadd.pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2)
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.fma.vfnmadd.pd(<2 x double>, <2 x double>, <2 x double>)

define <8 x float> @test_x86_fma_vfnmadd_ps_256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmadd_ps_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmadd213ps %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmadd_ps_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmaddps %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <8 x float> @llvm.x86.fma.vfnmadd.ps.256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2)
  ret <8 x float> %res
}
declare <8 x float> @llvm.x86.fma.vfnmadd.ps.256(<8 x float>, <8 x float>, <8 x float>)

define <4 x double> @test_x86_fma_vfnmadd_pd_256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmadd_pd_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmadd213pd %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmadd_pd_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmaddpd %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x double> @llvm.x86.fma.vfnmadd.pd.256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2)
  ret <4 x double> %res
}
declare <4 x double> @llvm.x86.fma.vfnmadd.pd.256(<4 x double>, <4 x double>, <4 x double>)

; VFNMSUB
define <4 x float> @test_x86_fma_vfnmsub_ss(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmsub_ss:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmsub213ss %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmsub_ss:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmsubss %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x float> @llvm.x86.fma.vfnmsub.ss(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.fma.vfnmsub.ss(<4 x float>, <4 x float>, <4 x float>)

define <2 x double> @test_x86_fma_vfnmsub_sd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmsub_sd:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmsub213sd %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmsub_sd:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmsubsd %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <2 x double> @llvm.x86.fma.vfnmsub.sd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2)
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.fma.vfnmsub.sd(<2 x double>, <2 x double>, <2 x double>)

define <4 x float> @test_x86_fma_vfnmsub_ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmsub_ps:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmsub213ps %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmsub_ps:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmsubps %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x float> @llvm.x86.fma.vfnmsub.ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.fma.vfnmsub.ps(<4 x float>, <4 x float>, <4 x float>)

define <2 x double> @test_x86_fma_vfnmsub_pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmsub_pd:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmsub213pd %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmsub_pd:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmsubpd %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <2 x double> @llvm.x86.fma.vfnmsub.pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2)
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.fma.vfnmsub.pd(<2 x double>, <2 x double>, <2 x double>)

define <8 x float> @test_x86_fma_vfnmsub_ps_256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmsub_ps_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmsub213ps %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmsub_ps_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmsubps %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <8 x float> @llvm.x86.fma.vfnmsub.ps.256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2)
  ret <8 x float> %res
}
declare <8 x float> @llvm.x86.fma.vfnmsub.ps.256(<8 x float>, <8 x float>, <8 x float>)

define <4 x double> @test_x86_fma_vfnmsub_pd_256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfnmsub_pd_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfnmsub213pd %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfnmsub_pd_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfnmsubpd %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x double> @llvm.x86.fma.vfnmsub.pd.256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2)
  ret <4 x double> %res
}
declare <4 x double> @llvm.x86.fma.vfnmsub.pd.256(<4 x double>, <4 x double>, <4 x double>)

; VFMADDSUB
define <4 x float> @test_x86_fma_vfmaddsub_ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmaddsub_ps:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmaddsub213ps %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmaddsub_ps:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmaddsubps %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x float> @llvm.x86.fma.vfmaddsub.ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.fma.vfmaddsub.ps(<4 x float>, <4 x float>, <4 x float>)

define <2 x double> @test_x86_fma_vfmaddsub_pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmaddsub_pd:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmaddsub213pd %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmaddsub_pd:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmaddsubpd %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <2 x double> @llvm.x86.fma.vfmaddsub.pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2)
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.fma.vfmaddsub.pd(<2 x double>, <2 x double>, <2 x double>)

define <8 x float> @test_x86_fma_vfmaddsub_ps_256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmaddsub_ps_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmaddsub213ps %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmaddsub_ps_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmaddsubps %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <8 x float> @llvm.x86.fma.vfmaddsub.ps.256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2)
  ret <8 x float> %res
}
declare <8 x float> @llvm.x86.fma.vfmaddsub.ps.256(<8 x float>, <8 x float>, <8 x float>)

define <4 x double> @test_x86_fma_vfmaddsub_pd_256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmaddsub_pd_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmaddsub213pd %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmaddsub_pd_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmaddsubpd %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x double> @llvm.x86.fma.vfmaddsub.pd.256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2)
  ret <4 x double> %res
}
declare <4 x double> @llvm.x86.fma.vfmaddsub.pd.256(<4 x double>, <4 x double>, <4 x double>)

; VFMSUBADD
define <4 x float> @test_x86_fma_vfmsubadd_ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmsubadd_ps:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmsubadd213ps %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmsubadd_ps:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmsubaddps %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x float> @llvm.x86.fma.vfmsubadd.ps(<4 x float> %a0, <4 x float> %a1, <4 x float> %a2)
  ret <4 x float> %res
}
declare <4 x float> @llvm.x86.fma.vfmsubadd.ps(<4 x float>, <4 x float>, <4 x float>)

define <2 x double> @test_x86_fma_vfmsubadd_pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmsubadd_pd:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmsubadd213pd %xmm2, %xmm1, %xmm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmsubadd_pd:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmsubaddpd %xmm2, %xmm1, %xmm0, %xmm0
; CHECK-FMA4-NEXT:    retq
  %res = call <2 x double> @llvm.x86.fma.vfmsubadd.pd(<2 x double> %a0, <2 x double> %a1, <2 x double> %a2)
  ret <2 x double> %res
}
declare <2 x double> @llvm.x86.fma.vfmsubadd.pd(<2 x double>, <2 x double>, <2 x double>)

define <8 x float> @test_x86_fma_vfmsubadd_ps_256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmsubadd_ps_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmsubadd213ps %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmsubadd_ps_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmsubaddps %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <8 x float> @llvm.x86.fma.vfmsubadd.ps.256(<8 x float> %a0, <8 x float> %a1, <8 x float> %a2)
  ret <8 x float> %res
}
declare <8 x float> @llvm.x86.fma.vfmsubadd.ps.256(<8 x float>, <8 x float>, <8 x float>)

define <4 x double> @test_x86_fma_vfmsubadd_pd_256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2) #0 {
; CHECK-FMA-LABEL: test_x86_fma_vfmsubadd_pd_256:
; CHECK-FMA:       # BB#0:
; CHECK-FMA-NEXT:    vfmsubadd213pd %ymm2, %ymm1, %ymm0
; CHECK-FMA-NEXT:    retq
;
; CHECK-FMA4-LABEL: test_x86_fma_vfmsubadd_pd_256:
; CHECK-FMA4:       # BB#0:
; CHECK-FMA4-NEXT:    vfmsubaddpd %ymm2, %ymm1, %ymm0, %ymm0
; CHECK-FMA4-NEXT:    retq
  %res = call <4 x double> @llvm.x86.fma.vfmsubadd.pd.256(<4 x double> %a0, <4 x double> %a1, <4 x double> %a2)
  ret <4 x double> %res
}
declare <4 x double> @llvm.x86.fma.vfmsubadd.pd.256(<4 x double>, <4 x double>, <4 x double>)

attributes #0 = { nounwind }
