; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+sse2 | FileCheck %s --check-prefix=ALL --check-prefix=SSE2
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx | FileCheck %s --check-prefix=ALL  --check-prefix=AVX

;
; Signed Integer to Double
;

define <2 x double> @sitofp_2vf64(<2 x i64> %a) {
; SSE2-LABEL: sitofp_2vf64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm0
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm1 = xmm1[0],xmm0[0]
; SSE2-NEXT:    movapd %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_2vf64:
; AVX:       # BB#0:
; AVX-NEXT:    vpextrq $1, %xmm0, %rax
; AVX-NEXT:    vcvtsi2sdq %rax, %xmm0, %xmm1
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2sdq %rax, %xmm0, %xmm0
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; AVX-NEXT:    retq
  %cvt = sitofp <2 x i64> %a to <2 x double>
  ret <2 x double> %cvt
}

define <2 x double> @sitofp_2vf64_i32(<4 x i32> %a) {
; SSE2-LABEL: sitofp_2vf64_i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    cvtdq2pd %xmm0, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_2vf64_i32:
; AVX:       # BB#0:
; AVX-NEXT:    vcvtdq2pd %xmm0, %xmm0
; AVX-NEXT:    retq
  %shuf = shufflevector <4 x i32> %a, <4 x i32> undef, <2 x i32> <i32 0, i32 1>
  %cvt = sitofp <2 x i32> %shuf to <2 x double>
  ret <2 x double> %cvt
}

define <2 x double> @sitofp_2vf64_i16(<8 x i16> %a) {
; SSE2-LABEL: sitofp_2vf64_i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[0,1,0,3]
; SSE2-NEXT:    pshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,5,5,6,7]
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    movswq %ax, %rax
; SSE2-NEXT:    movd %xmm0, %rcx
; SSE2-NEXT:    movswq %cx, %rcx
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2sdq %rcx, %xmm0
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm1
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_2vf64_i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxwq {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    movswq %ax, %rax
; AVX-NEXT:    vpextrq $1, %xmm0, %rcx
; AVX-NEXT:    movswq %cx, %rcx
; AVX-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2sdq %rcx, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2sdq %rax, %xmm0, %xmm1
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm1[0],xmm0[0]
; AVX-NEXT:    retq
  %shuf = shufflevector <8 x i16> %a, <8 x i16> undef, <2 x i32> <i32 0, i32 1>
  %cvt = sitofp <2 x i16> %shuf to <2 x double>
  ret <2 x double> %cvt
}

define <2 x double> @sitofp_2vf64_i8(<16 x i8> %a) {
; SSE2-LABEL: sitofp_2vf64_i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0,0,1,1]
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    movsbq %al, %rax
; SSE2-NEXT:    movd %xmm0, %rcx
; SSE2-NEXT:    movsbq %cl, %rcx
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2sdq %rcx, %xmm0
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm1
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_2vf64_i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxbq {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,zero,zero,zero,zero,xmm0[1],zero,zero,zero,zero,zero,zero,zero
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    movsbq %al, %rax
; AVX-NEXT:    vpextrq $1, %xmm0, %rcx
; AVX-NEXT:    movsbq %cl, %rcx
; AVX-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2sdq %rcx, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2sdq %rax, %xmm0, %xmm1
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm1[0],xmm0[0]
; AVX-NEXT:    retq
  %shuf = shufflevector <16 x i8> %a, <16 x i8> undef, <2 x i32> <i32 0, i32 1>
  %cvt = sitofp <2 x i8> %shuf to <2 x double>
  ret <2 x double> %cvt
}

define <4 x double> @sitofp_4vf64(<4 x i64> %a) {
; SSE2-LABEL: sitofp_4vf64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm0
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm2 = xmm2[0],xmm0[0]
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm3
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm1[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm0
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm3 = xmm3[0],xmm0[0]
; SSE2-NEXT:    movapd %xmm2, %xmm0
; SSE2-NEXT:    movapd %xmm3, %xmm1
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_4vf64:
; AVX:       # BB#0:
; AVX-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX-NEXT:    vpextrq $1, %xmm1, %rax
; AVX-NEXT:    vcvtsi2sdq %rax, %xmm0, %xmm2
; AVX-NEXT:    vmovq %xmm1, %rax
; AVX-NEXT:    vcvtsi2sdq %rax, %xmm0, %xmm1
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; AVX-NEXT:    vpextrq $1, %xmm0, %rax
; AVX-NEXT:    vcvtsi2sdq %rax, %xmm0, %xmm2
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2sdq %rax, %xmm0, %xmm0
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; AVX-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX-NEXT:    retq
  %cvt = sitofp <4 x i64> %a to <4 x double>
  ret <4 x double> %cvt
}

define <4 x double> @sitofp_4vf64_i32(<4 x i32> %a) {
; SSE2-LABEL: sitofp_4vf64_i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    cvtdq2pd %xmm0, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    cvtdq2pd %xmm0, %xmm1
; SSE2-NEXT:    movaps %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_4vf64_i32:
; AVX:       # BB#0:
; AVX-NEXT:    vcvtdq2pd %xmm0, %ymm0
; AVX-NEXT:    retq
  %cvt = sitofp <4 x i32> %a to <4 x double>
  ret <4 x double> %cvt
}

define <4 x double> @sitofp_4vf64_i16(<8 x i16> %a) {
; SSE2-LABEL: sitofp_4vf64_i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[0,1,0,3]
; SSE2-NEXT:    pshufhw {{.*#+}} xmm1 = xmm1[0,1,2,3,5,5,6,7]
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    movswq %ax, %rax
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[2,3,0,1]
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    movswq %ax, %rax
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm1
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm2 = xmm2[0],xmm1[0]
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[0,1,2,1]
; SSE2-NEXT:    pshuflw {{.*#+}} xmm0 = xmm0[2,1,2,3,4,5,6,7]
; SSE2-NEXT:    pshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,7,5,6,7]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    movswq %ax, %rax
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    movswq %ax, %rax
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2sdq %rax, %xmm0
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm1 = xmm1[0],xmm0[0]
; SSE2-NEXT:    movapd %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_4vf64_i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxwd {{.*#+}} xmm0 = xmm0[0],zero,xmm0[1],zero,xmm0[2],zero,xmm0[3],zero
; AVX-NEXT:    vpslld $16, %xmm0, %xmm0
; AVX-NEXT:    vpsrad $16, %xmm0, %xmm0
; AVX-NEXT:    vcvtdq2pd %xmm0, %ymm0
; AVX-NEXT:    retq
  %shuf = shufflevector <8 x i16> %a, <8 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %cvt = sitofp <4 x i16> %shuf to <4 x double>
  ret <4 x double> %cvt
}

define <4 x double> @sitofp_4vf64_i8(<16 x i8> %a) {
; SSE2-LABEL: sitofp_4vf64_i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSE2-NEXT:    psrad $24, %xmm1
; SSE2-NEXT:    cvtdq2pd %xmm1, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[2,3,0,1]
; SSE2-NEXT:    cvtdq2pd %xmm1, %xmm1
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_4vf64_i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovsxbd %xmm0, %xmm0
; AVX-NEXT:    vcvtdq2pd %xmm0, %ymm0
; AVX-NEXT:    retq
  %shuf = shufflevector <16 x i8> %a, <16 x i8> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %cvt = sitofp <4 x i8> %shuf to <4 x double>
  ret <4 x double> %cvt
}

;
; Unsigned Integer to Double
;

define <2 x double> @uitofp_2vf64(<2 x i64> %a) {
; SSE2-LABEL: uitofp_2vf64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa {{.*#+}} xmm1 = [1127219200,1160773632,0,0]
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    movapd {{.*#+}} xmm3 = [4.503600e+15,1.934281e+25]
; SSE2-NEXT:    subpd %xmm3, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm4 = xmm0[2,3,0,1]
; SSE2-NEXT:    addpd %xmm4, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; SSE2-NEXT:    subpd %xmm3, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm2[2,3,0,1]
; SSE2-NEXT:    addpd %xmm2, %xmm1
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_2vf64:
; AVX:       # BB#0:
; AVX-NEXT:    vmovdqa {{.*#+}} xmm1 = [1127219200,1160773632,0,0]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm2 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; AVX-NEXT:    vmovapd {{.*#+}} xmm3 = [4.503600e+15,1.934281e+25]
; AVX-NEXT:    vsubpd %xmm3, %xmm2, %xmm2
; AVX-NEXT:    vhaddpd %xmm2, %xmm2, %xmm2
; AVX-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; AVX-NEXT:    vsubpd %xmm3, %xmm0, %xmm0
; AVX-NEXT:    vhaddpd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm2[0],xmm0[0]
; AVX-NEXT:    retq
  %cvt = uitofp <2 x i64> %a to <2 x double>
  ret <2 x double> %cvt
}

define <2 x double> @uitofp_2vf64_i32(<4 x i32> %a) {
; SSE2-LABEL: uitofp_2vf64_i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm1, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    movdqa {{.*#+}} xmm1 = [1127219200,1160773632,0,0]
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    movapd {{.*#+}} xmm3 = [4.503600e+15,1.934281e+25]
; SSE2-NEXT:    subpd %xmm3, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm4 = xmm0[2,3,0,1]
; SSE2-NEXT:    addpd %xmm4, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; SSE2-NEXT:    subpd %xmm3, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm2[2,3,0,1]
; SSE2-NEXT:    addpd %xmm2, %xmm1
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_2vf64_i32:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxdq {{.*#+}} xmm0 = xmm0[0],zero,xmm0[1],zero
; AVX-NEXT:    vmovdqa {{.*#+}} xmm1 = [1127219200,1160773632,0,0]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm2 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; AVX-NEXT:    vmovapd {{.*#+}} xmm3 = [4.503600e+15,1.934281e+25]
; AVX-NEXT:    vsubpd %xmm3, %xmm2, %xmm2
; AVX-NEXT:    vhaddpd %xmm2, %xmm2, %xmm2
; AVX-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; AVX-NEXT:    vsubpd %xmm3, %xmm0, %xmm0
; AVX-NEXT:    vhaddpd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm2[0],xmm0[0]
; AVX-NEXT:    retq
  %shuf = shufflevector <4 x i32> %a, <4 x i32> undef, <2 x i32> <i32 0, i32 1>
  %cvt = uitofp <2 x i32> %shuf to <2 x double>
  ret <2 x double> %cvt
}

define <2 x double> @uitofp_2vf64_i16(<8 x i16> %a) {
; SSE2-LABEL: uitofp_2vf64_i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm1, %xmm1
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    movdqa {{.*#+}} xmm1 = [1127219200,1160773632,0,0]
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    movapd {{.*#+}} xmm3 = [4.503600e+15,1.934281e+25]
; SSE2-NEXT:    subpd %xmm3, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm4 = xmm0[2,3,0,1]
; SSE2-NEXT:    addpd %xmm4, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; SSE2-NEXT:    subpd %xmm3, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm2[2,3,0,1]
; SSE2-NEXT:    addpd %xmm2, %xmm1
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_2vf64_i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxwq {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero
; AVX-NEXT:    vmovdqa {{.*#+}} xmm1 = [1127219200,1160773632,0,0]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm2 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; AVX-NEXT:    vmovapd {{.*#+}} xmm3 = [4.503600e+15,1.934281e+25]
; AVX-NEXT:    vsubpd %xmm3, %xmm2, %xmm2
; AVX-NEXT:    vhaddpd %xmm2, %xmm2, %xmm2
; AVX-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; AVX-NEXT:    vsubpd %xmm3, %xmm0, %xmm0
; AVX-NEXT:    vhaddpd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm2[0],xmm0[0]
; AVX-NEXT:    retq
  %shuf = shufflevector <8 x i16> %a, <8 x i16> undef, <2 x i32> <i32 0, i32 1>
  %cvt = uitofp <2 x i16> %shuf to <2 x double>
  ret <2 x double> %cvt
}

define <2 x double> @uitofp_2vf64_i8(<16 x i8> %a) {
; SSE2-LABEL: uitofp_2vf64_i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm1, %xmm1
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    movdqa {{.*#+}} xmm1 = [1127219200,1160773632,0,0]
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    movapd {{.*#+}} xmm3 = [4.503600e+15,1.934281e+25]
; SSE2-NEXT:    subpd %xmm3, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm4 = xmm0[2,3,0,1]
; SSE2-NEXT:    addpd %xmm4, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; SSE2-NEXT:    subpd %xmm3, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm2[2,3,0,1]
; SSE2-NEXT:    addpd %xmm2, %xmm1
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_2vf64_i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxbq {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,zero,zero,zero,zero,xmm0[1],zero,zero,zero,zero,zero,zero,zero
; AVX-NEXT:    vmovdqa {{.*#+}} xmm1 = [1127219200,1160773632,0,0]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm2 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; AVX-NEXT:    vmovapd {{.*#+}} xmm3 = [4.503600e+15,1.934281e+25]
; AVX-NEXT:    vsubpd %xmm3, %xmm2, %xmm2
; AVX-NEXT:    vhaddpd %xmm2, %xmm2, %xmm2
; AVX-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; AVX-NEXT:    vsubpd %xmm3, %xmm0, %xmm0
; AVX-NEXT:    vhaddpd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm2[0],xmm0[0]
; AVX-NEXT:    retq
  %shuf = shufflevector <16 x i8> %a, <16 x i8> undef, <2 x i32> <i32 0, i32 1>
  %cvt = uitofp <2 x i8> %shuf to <2 x double>
  ret <2 x double> %cvt
}

define <4 x double> @uitofp_4vf64(<4 x i64> %a) {
; SSE2-LABEL: uitofp_4vf64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa {{.*#+}} xmm2 = [1127219200,1160773632,0,0]
; SSE2-NEXT:    pshufd {{.*#+}} xmm3 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSE2-NEXT:    movapd {{.*#+}} xmm4 = [4.503600e+15,1.934281e+25]
; SSE2-NEXT:    subpd %xmm4, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm0[2,3,0,1]
; SSE2-NEXT:    addpd %xmm5, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm3 = xmm3[0],xmm2[0],xmm3[1],xmm2[1]
; SSE2-NEXT:    subpd %xmm4, %xmm3
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm3[2,3,0,1]
; SSE2-NEXT:    addpd %xmm3, %xmm5
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm5[0]
; SSE2-NEXT:    pshufd {{.*#+}} xmm3 = xmm1[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; SSE2-NEXT:    subpd %xmm4, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm1[2,3,0,1]
; SSE2-NEXT:    addpd %xmm5, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm3 = xmm3[0],xmm2[0],xmm3[1],xmm2[1]
; SSE2-NEXT:    subpd %xmm4, %xmm3
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm3[2,3,0,1]
; SSE2-NEXT:    addpd %xmm3, %xmm2
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_4vf64:
; AVX:       # BB#0:
; AVX-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX-NEXT:    vmovdqa {{.*#+}} xmm2 = [1127219200,1160773632,0,0]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm3 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; AVX-NEXT:    vmovapd {{.*#+}} xmm4 = [4.503600e+15,1.934281e+25]
; AVX-NEXT:    vsubpd %xmm4, %xmm3, %xmm3
; AVX-NEXT:    vhaddpd %xmm3, %xmm3, %xmm3
; AVX-NEXT:    vpshufd {{.*#+}} xmm1 = xmm1[2,3,0,1]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; AVX-NEXT:    vsubpd %xmm4, %xmm1, %xmm1
; AVX-NEXT:    vhaddpd %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm1 = xmm3[0],xmm1[0]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm3 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; AVX-NEXT:    vsubpd %xmm4, %xmm3, %xmm3
; AVX-NEXT:    vhaddpd %xmm3, %xmm3, %xmm3
; AVX-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX-NEXT:    vpunpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; AVX-NEXT:    vsubpd %xmm4, %xmm0, %xmm0
; AVX-NEXT:    vhaddpd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm3[0],xmm0[0]
; AVX-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX-NEXT:    retq
  %cvt = uitofp <4 x i64> %a to <4 x double>
  ret <4 x double> %cvt
}

define <4 x double> @uitofp_4vf64_i32(<4 x i32> %a) {
; SSE2-LABEL: uitofp_4vf64_i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm1, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[2,2,3,3]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    movdqa {{.*#+}} xmm3 = [1127219200,1160773632,0,0]
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1]
; SSE2-NEXT:    movapd {{.*#+}} xmm4 = [4.503600e+15,1.934281e+25]
; SSE2-NEXT:    subpd %xmm4, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm0[2,3,0,1]
; SSE2-NEXT:    addpd %xmm5, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm3[0],xmm1[1],xmm3[1]
; SSE2-NEXT:    subpd %xmm4, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm1[2,3,0,1]
; SSE2-NEXT:    addpd %xmm1, %xmm5
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm5[0]
; SSE2-NEXT:    pand .LCPI13_2(%rip), %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm2[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm3[0],xmm2[1],xmm3[1]
; SSE2-NEXT:    subpd %xmm4, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm2[2,3,0,1]
; SSE2-NEXT:    addpd %xmm2, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm5 = xmm5[0],xmm3[0],xmm5[1],xmm3[1]
; SSE2-NEXT:    subpd %xmm4, %xmm5
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm5[2,3,0,1]
; SSE2-NEXT:    addpd %xmm5, %xmm2
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_4vf64_i32:
; AVX:       # BB#0:
; AVX-NEXT:    vpand .LCPI13_0(%rip), %xmm0, %xmm1
; AVX-NEXT:    vcvtdq2pd %xmm1, %ymm1
; AVX-NEXT:    vpsrld $16, %xmm0, %xmm0
; AVX-NEXT:    vcvtdq2pd %xmm0, %ymm0
; AVX-NEXT:    vmulpd .LCPI13_1(%rip), %ymm0, %ymm0
; AVX-NEXT:    vaddpd %ymm1, %ymm0, %ymm0
; AVX-NEXT:    retq
  %cvt = uitofp <4 x i32> %a to <4 x double>
  ret <4 x double> %cvt
}

define <4 x double> @uitofp_4vf64_i16(<8 x i16> %a) {
; SSE2-LABEL: uitofp_4vf64_i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm1, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[0,1,2,1]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    movdqa {{.*#+}} xmm3 = [1127219200,1160773632,0,0]
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1]
; SSE2-NEXT:    movapd {{.*#+}} xmm4 = [4.503600e+15,1.934281e+25]
; SSE2-NEXT:    subpd %xmm4, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm0[2,3,0,1]
; SSE2-NEXT:    addpd %xmm5, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm3[0],xmm1[1],xmm3[1]
; SSE2-NEXT:    subpd %xmm4, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm1[2,3,0,1]
; SSE2-NEXT:    addpd %xmm1, %xmm5
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm5[0]
; SSE2-NEXT:    pshuflw {{.*#+}} xmm1 = xmm2[2,1,2,3,4,5,6,7]
; SSE2-NEXT:    pshufhw {{.*#+}} xmm2 = xmm1[0,1,2,3,7,5,6,7]
; SSE2-NEXT:    pand .LCPI14_2(%rip), %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm2[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm3[0],xmm2[1],xmm3[1]
; SSE2-NEXT:    subpd %xmm4, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm2[2,3,0,1]
; SSE2-NEXT:    addpd %xmm2, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm5 = xmm5[0],xmm3[0],xmm5[1],xmm3[1]
; SSE2-NEXT:    subpd %xmm4, %xmm5
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm5[2,3,0,1]
; SSE2-NEXT:    addpd %xmm5, %xmm2
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_4vf64_i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxwd {{.*#+}} xmm0 = xmm0[0],zero,xmm0[1],zero,xmm0[2],zero,xmm0[3],zero
; AVX-NEXT:    vcvtdq2pd %xmm0, %ymm0
; AVX-NEXT:    retq
  %shuf = shufflevector <8 x i16> %a, <8 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %cvt = uitofp <4 x i16> %shuf to <4 x double>
  ret <4 x double> %cvt
}

define <4 x double> @uitofp_4vf64_i8(<16 x i8> %a) {
; SSE2-LABEL: uitofp_4vf64_i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    pxor %xmm2, %xmm2
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3],xmm0[4],xmm2[4],xmm0[5],xmm2[5],xmm0[6],xmm2[6],xmm0[7],xmm2[7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSE2-NEXT:    movdqa {{.*#+}} xmm2 = [1127219200,1160773632,0,0]
; SSE2-NEXT:    pshufd {{.*#+}} xmm4 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSE2-NEXT:    movapd {{.*#+}} xmm3 = [4.503600e+15,1.934281e+25]
; SSE2-NEXT:    subpd %xmm3, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm0[2,3,0,1]
; SSE2-NEXT:    addpd %xmm5, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm4 = xmm4[0],xmm2[0],xmm4[1],xmm2[1]
; SSE2-NEXT:    subpd %xmm3, %xmm4
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm4[2,3,0,1]
; SSE2-NEXT:    addpd %xmm4, %xmm5
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm0 = xmm0[0],xmm5[0]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[0,1,2,1]
; SSE2-NEXT:    pshuflw {{.*#+}} xmm1 = xmm1[2,1,2,3,4,5,6,7]
; SSE2-NEXT:    pshufhw {{.*#+}} xmm4 = xmm1[0,1,2,3,7,5,6,7]
; SSE2-NEXT:    pand .LCPI15_2(%rip), %xmm4
; SSE2-NEXT:    pshufd {{.*#+}} xmm5 = xmm4[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm4 = xmm4[0],xmm2[0],xmm4[1],xmm2[1]
; SSE2-NEXT:    subpd %xmm3, %xmm4
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm4[2,3,0,1]
; SSE2-NEXT:    addpd %xmm4, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm5 = xmm5[0],xmm2[0],xmm5[1],xmm2[1]
; SSE2-NEXT:    subpd %xmm3, %xmm5
; SSE2-NEXT:    pshufd {{.*#+}}  xmm2 = xmm5[2,3,0,1]
; SSE2-NEXT:    addpd %xmm5, %xmm2
; SSE2-NEXT:    unpcklpd {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_4vf64_i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxbd {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX-NEXT:    vcvtdq2pd %xmm0, %ymm0
; AVX-NEXT:    retq
  %shuf = shufflevector <16 x i8> %a, <16 x i8> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %cvt = uitofp <4 x i8> %shuf to <4 x double>
  ret <4 x double> %cvt
}

;
; Signed Integer to Float
;

define <4 x float> @sitofp_4vf32(<4 x i32> %a) {
; SSE2-LABEL: sitofp_4vf32:
; SSE2:       # BB#0:
; SSE2-NEXT:    cvtdq2ps %xmm0, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_4vf32:
; AVX:       # BB#0:
; AVX-NEXT:    vcvtdq2ps %xmm0, %xmm0
; AVX-NEXT:    retq
  %cvt = sitofp <4 x i32> %a to <4 x float>
  ret <4 x float> %cvt
}

define <4 x float> @sitofp_4vf32_i64(<2 x i64> %a) {
; SSE2-LABEL: sitofp_4vf32_i64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm0
; SSE2-NEXT:    unpcklps {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1]
; SSE2-NEXT:    movaps %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_4vf32_i64:
; AVX:       # BB#0:
; AVX-NEXT:    vpextrq $1, %xmm0, %rax
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm1
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm0
; AVX-NEXT:    vinsertps {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[2,3]
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm1
; AVX-NEXT:    vinsertps {{.*#+}} xmm0 = xmm0[0,1],xmm1[0],xmm0[3]
; AVX-NEXT:    vinsertps {{.*#+}} xmm0 = xmm0[0,1,2],xmm1[0]
; AVX-NEXT:    retq
  %cvt = sitofp <2 x i64> %a to <2 x float>
  %ext = shufflevector <2 x float> %cvt, <2 x float> undef, <4 x i32> <i32 0, i32 1, i32 undef, i32 undef>
  ret <4 x float> %ext
}

define <4 x float> @sitofp_4vf32_i16(<8 x i16> %a) {
; SSE2-LABEL: sitofp_4vf32_i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    pslld $16, %xmm0
; SSE2-NEXT:    psrad $16, %xmm0
; SSE2-NEXT:    cvtdq2ps %xmm0, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_4vf32_i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxwd {{.*#+}} xmm0 = xmm0[0],zero,xmm0[1],zero,xmm0[2],zero,xmm0[3],zero
; AVX-NEXT:    vpslld $16, %xmm0, %xmm0
; AVX-NEXT:    vpsrad $16, %xmm0, %xmm0
; AVX-NEXT:    vcvtdq2ps %xmm0, %xmm0
; AVX-NEXT:    retq
  %shuf = shufflevector <8 x i16> %a, <8 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %cvt = sitofp <4 x i16> %shuf to <4 x float>
  ret <4 x float> %cvt
}

define <4 x float> @sitofp_4vf32_i8(<16 x i8> %a) {
; SSE2-LABEL: sitofp_4vf32_i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $24, %xmm0
; SSE2-NEXT:    cvtdq2ps %xmm0, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_4vf32_i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovsxbd %xmm0, %xmm0
; AVX-NEXT:    vcvtdq2ps %xmm0, %xmm0
; AVX-NEXT:    retq
  %shuf = shufflevector <16 x i8> %a, <16 x i8> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %cvt = sitofp <4 x i8> %shuf to <4 x float>
  ret <4 x float> %cvt
}

define <8 x float> @sitofp_8vf32(<8 x i32> %a) {
; SSE2-LABEL: sitofp_8vf32:
; SSE2:       # BB#0:
; SSE2-NEXT:    cvtdq2ps %xmm0, %xmm0
; SSE2-NEXT:    cvtdq2ps %xmm1, %xmm1
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_8vf32:
; AVX:       # BB#0:
; AVX-NEXT:    vcvtdq2ps %ymm0, %ymm0
; AVX-NEXT:    retq
  %cvt = sitofp <8 x i32> %a to <8 x float>
  ret <8 x float> %cvt
}

define <4 x float> @sitofp_4vf32_4i64(<4 x i64> %a) {
; SSE2-LABEL: sitofp_4vf32_4i64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm3
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm2
; SSE2-NEXT:    unpcklps {{.*#+}} xmm2 = xmm2[0],xmm3[0],xmm2[1],xmm3[1]
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[2,3,0,1]
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm0
; SSE2-NEXT:    unpcklps {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    unpcklps {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1]
; SSE2-NEXT:    movaps %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_4vf32_4i64:
; AVX:       # BB#0:
; AVX-NEXT:    vpextrq $1, %xmm0, %rax
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm1
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm2
; AVX-NEXT:    vinsertps {{.*#+}} xmm1 = xmm2[0],xmm1[0],xmm2[2,3]
; AVX-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm2
; AVX-NEXT:    vinsertps {{.*#+}} xmm1 = xmm1[0,1],xmm2[0],xmm1[3]
; AVX-NEXT:    vpextrq $1, %xmm0, %rax
; AVX-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm0
; AVX-NEXT:    vinsertps {{.*#+}} xmm0 = xmm1[0,1,2],xmm0[0]
; AVX-NEXT:    vzeroupper
; AVX-NEXT:    retq
  %cvt = sitofp <4 x i64> %a to <4 x float>
  ret <4 x float> %cvt
}

define <8 x float> @sitofp_8vf32_i16(<8 x i16> %a) {
; SSE2-LABEL: sitofp_8vf32_i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSE2-NEXT:    pslld $16, %xmm1
; SSE2-NEXT:    psrad $16, %xmm1
; SSE2-NEXT:    cvtdq2ps %xmm1, %xmm2
; SSE2-NEXT:    punpckhwd {{.*#+}} xmm0 = xmm0[4,4,5,5,6,6,7,7]
; SSE2-NEXT:    pslld $16, %xmm0
; SSE2-NEXT:    psrad $16, %xmm0
; SSE2-NEXT:    cvtdq2ps %xmm0, %xmm1
; SSE2-NEXT:    movaps %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_8vf32_i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovsxwd %xmm0, %xmm1
; AVX-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX-NEXT:    vpmovsxwd %xmm0, %xmm0
; AVX-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX-NEXT:    vcvtdq2ps %ymm0, %ymm0
; AVX-NEXT:    retq
  %cvt = sitofp <8 x i16> %a to <8 x float>
  ret <8 x float> %cvt
}

define <8 x float> @sitofp_8vf32_i8(<16 x i8> %a) {
; SSE2-LABEL: sitofp_8vf32_i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $24, %xmm1
; SSE2-NEXT:    cvtdq2ps %xmm1, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $24, %xmm0
; SSE2-NEXT:    cvtdq2ps %xmm0, %xmm1
; SSE2-NEXT:    movaps %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: sitofp_8vf32_i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovsxbd %xmm0, %xmm1
; AVX-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; AVX-NEXT:    vpmovsxbd %xmm0, %xmm0
; AVX-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX-NEXT:    vcvtdq2ps %ymm0, %ymm0
; AVX-NEXT:    retq
  %shuf = shufflevector <16 x i8> %a, <16 x i8> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %cvt = sitofp <8 x i8> %shuf to <8 x float>
  ret <8 x float> %cvt
}

;
; Unsigned Integer to Float
;

define <4 x float> @uitofp_4vf32(<4 x i32> %a) {
; SSE2-LABEL: uitofp_4vf32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa {{.*#+}} xmm1 = [65535,65535,65535,65535]
; SSE2-NEXT:    pand %xmm0, %xmm1
; SSE2-NEXT:    por .LCPI24_1(%rip), %xmm1
; SSE2-NEXT:    psrld $16, %xmm0
; SSE2-NEXT:    por .LCPI24_2(%rip), %xmm0
; SSE2-NEXT:    addps .LCPI24_3(%rip), %xmm0
; SSE2-NEXT:    addps %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_4vf32:
; AVX:       # BB#0:
; AVX-NEXT:    vpblendw {{.*#+}} xmm1 = xmm0[0],mem[1],xmm0[2],mem[3],xmm0[4],mem[5],xmm0[6],mem[7]
; AVX-NEXT:    vpsrld $16, %xmm0, %xmm0
; AVX-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0],mem[1],xmm0[2],mem[3],xmm0[4],mem[5],xmm0[6],mem[7]
; AVX-NEXT:    vaddps .LCPI24_2(%rip), %xmm0, %xmm0
; AVX-NEXT:    vaddps %xmm0, %xmm1, %xmm0
; AVX-NEXT:    retq
  %cvt = uitofp <4 x i32> %a to <4 x float>
  ret <4 x float> %cvt
}

define <4 x float> @uitofp_4vf32_i64(<2 x i64> %a) {
; SSE2-LABEL: uitofp_4vf32_i64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    testq %rax, %rax
; SSE2-NEXT:    js .LBB25_1
; SSE2-NEXT:  # BB#2:
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm0
; SSE2-NEXT:    jmp .LBB25_3
; SSE2-NEXT:  .LBB25_1:
; SSE2-NEXT:    shrq %rax
; SSE2-NEXT:    orq %rax, %rcx
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2ssq %rcx, %xmm0
; SSE2-NEXT:    addss %xmm0, %xmm0
; SSE2-NEXT:  .LBB25_3:
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[2,3,0,1]
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    testq %rax, %rax
; SSE2-NEXT:    js .LBB25_4
; SSE2-NEXT:  # BB#5:
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm1
; SSE2-NEXT:    unpcklps {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
; SSE2-NEXT:  .LBB25_4:
; SSE2-NEXT:    shrq %rax
; SSE2-NEXT:    orq %rax, %rcx
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    cvtsi2ssq %rcx, %xmm1
; SSE2-NEXT:    addss %xmm1, %xmm1
; SSE2-NEXT:    unpcklps {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_4vf32_i64:
; AVX:       # BB#0:
; AVX-NEXT:    vpextrq $1, %xmm0, %rax
; AVX-NEXT:    movl %eax, %ecx
; AVX-NEXT:    andl $1, %ecx
; AVX-NEXT:    testq %rax, %rax
; AVX-NEXT:    js .LBB25_1
; AVX-NEXT:  # BB#2:
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm1
; AVX-NEXT:    jmp .LBB25_3
; AVX-NEXT:  .LBB25_1:
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    orq %rax, %rcx
; AVX-NEXT:    vcvtsi2ssq %rcx, %xmm0, %xmm1
; AVX-NEXT:    vaddss %xmm1, %xmm1, %xmm1
; AVX-NEXT:  .LBB25_3:
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    movl %eax, %ecx
; AVX-NEXT:    andl $1, %ecx
; AVX-NEXT:    testq %rax, %rax
; AVX-NEXT:    js .LBB25_4
; AVX-NEXT:  # BB#5:
; AVX-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm0
; AVX-NEXT:    jmp .LBB25_6
; AVX-NEXT:  .LBB25_4:
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    orq %rax, %rcx
; AVX-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2ssq %rcx, %xmm0, %xmm0
; AVX-NEXT:    vaddss %xmm0, %xmm0, %xmm0
; AVX-NEXT:  .LBB25_6:
; AVX-NEXT:    vinsertps {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[2,3]
; AVX-NEXT:    vxorps %xmm1, %xmm1, %xmm1
; AVX-NEXT:    testq %rax, %rax
; AVX-NEXT:    js .LBB25_8
; AVX-NEXT:  # BB#7:
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm1
; AVX-NEXT:  .LBB25_8:
; AVX-NEXT:    vinsertps {{.*#+}} xmm0 = xmm0[0,1],xmm1[0],xmm0[3]
; AVX-NEXT:    vinsertps {{.*#+}} xmm0 = xmm0[0,1,2],xmm1[0]
; AVX-NEXT:    retq
  %cvt = uitofp <2 x i64> %a to <2 x float>
  %ext = shufflevector <2 x float> %cvt, <2 x float> undef, <4 x i32> <i32 0, i32 1, i32 undef, i32 undef>
  ret <4 x float> %ext
}

define <4 x float> @uitofp_4vf32_i16(<8 x i16> %a) {
; SSE2-LABEL: uitofp_4vf32_i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm1, %xmm1
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3]
; SSE2-NEXT:    cvtdq2ps %xmm0, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_4vf32_i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxwd {{.*#+}} xmm0 = xmm0[0],zero,xmm0[1],zero,xmm0[2],zero,xmm0[3],zero
; AVX-NEXT:    vcvtdq2ps %xmm0, %xmm0
; AVX-NEXT:    retq
  %shuf = shufflevector <8 x i16> %a, <8 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %cvt = uitofp <4 x i16> %shuf to <4 x float>
  ret <4 x float> %cvt
}

define <4 x float> @uitofp_4vf32_i8(<16 x i8> %a) {
; SSE2-LABEL: uitofp_4vf32_i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm1, %xmm1
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3]
; SSE2-NEXT:    cvtdq2ps %xmm0, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_4vf32_i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxbd {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX-NEXT:    vcvtdq2ps %xmm0, %xmm0
; AVX-NEXT:    retq
  %shuf = shufflevector <16 x i8> %a, <16 x i8> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %cvt = uitofp <4 x i8> %shuf to <4 x float>
  ret <4 x float> %cvt
}

define <8 x float> @uitofp_8vf32(<8 x i32> %a) {
; SSE2-LABEL: uitofp_8vf32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa {{.*#+}} xmm2 = [65535,65535,65535,65535]
; SSE2-NEXT:    movdqa %xmm0, %xmm3
; SSE2-NEXT:    pand %xmm2, %xmm3
; SSE2-NEXT:    movdqa {{.*#+}} xmm4 = [1258291200,1258291200,1258291200,1258291200]
; SSE2-NEXT:    por %xmm4, %xmm3
; SSE2-NEXT:    psrld $16, %xmm0
; SSE2-NEXT:    movdqa {{.*#+}} xmm5 = [1392508928,1392508928,1392508928,1392508928]
; SSE2-NEXT:    por %xmm5, %xmm0
; SSE2-NEXT:    movaps {{.*#+}} xmm6 = [-5.497642e+11,-5.497642e+11,-5.497642e+11,-5.497642e+11]
; SSE2-NEXT:    addps %xmm6, %xmm0
; SSE2-NEXT:    addps %xmm3, %xmm0
; SSE2-NEXT:    pand %xmm1, %xmm2
; SSE2-NEXT:    por %xmm4, %xmm2
; SSE2-NEXT:    psrld $16, %xmm1
; SSE2-NEXT:    por %xmm5, %xmm1
; SSE2-NEXT:    addps %xmm6, %xmm1
; SSE2-NEXT:    addps %xmm2, %xmm1
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_8vf32:
; AVX:       # BB#0:
; AVX-NEXT:    vandps .LCPI28_0(%rip), %ymm0, %ymm1
; AVX-NEXT:    vcvtdq2ps %ymm1, %ymm1
; AVX-NEXT:    vpsrld $16, %xmm0, %xmm2
; AVX-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX-NEXT:    vpsrld $16, %xmm0, %xmm0
; AVX-NEXT:    vinsertf128 $1, %xmm0, %ymm2, %ymm0
; AVX-NEXT:    vcvtdq2ps %ymm0, %ymm0
; AVX-NEXT:    vmulps .LCPI28_1(%rip), %ymm0, %ymm0
; AVX-NEXT:    vaddps %ymm1, %ymm0, %ymm0
; AVX-NEXT:    retq
  %cvt = uitofp <8 x i32> %a to <8 x float>
  ret <8 x float> %cvt
}

define <4 x float> @uitofp_4vf32_4i64(<4 x i64> %a) {
; SSE2-LABEL: uitofp_4vf32_4i64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    testq %rax, %rax
; SSE2-NEXT:    js .LBB29_1
; SSE2-NEXT:  # BB#2:
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm3
; SSE2-NEXT:    jmp .LBB29_3
; SSE2-NEXT:  .LBB29_1:
; SSE2-NEXT:    shrq %rax
; SSE2-NEXT:    orq %rax, %rcx
; SSE2-NEXT:    cvtsi2ssq %rcx, %xmm3
; SSE2-NEXT:    addss %xmm3, %xmm3
; SSE2-NEXT:  .LBB29_3:
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    testq %rax, %rax
; SSE2-NEXT:    js .LBB29_4
; SSE2-NEXT:  # BB#5:
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm2
; SSE2-NEXT:    jmp .LBB29_6
; SSE2-NEXT:  .LBB29_4:
; SSE2-NEXT:    shrq %rax
; SSE2-NEXT:    orq %rax, %rcx
; SSE2-NEXT:    cvtsi2ssq %rcx, %xmm2
; SSE2-NEXT:    addss %xmm2, %xmm2
; SSE2-NEXT:  .LBB29_6:
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[2,3,0,1]
; SSE2-NEXT:    movd %xmm1, %rax
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    testq %rax, %rax
; SSE2-NEXT:    js .LBB29_7
; SSE2-NEXT:  # BB#8:
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm1
; SSE2-NEXT:    jmp .LBB29_9
; SSE2-NEXT:  .LBB29_7:
; SSE2-NEXT:    shrq %rax
; SSE2-NEXT:    orq %rax, %rcx
; SSE2-NEXT:    xorps %xmm1, %xmm1
; SSE2-NEXT:    cvtsi2ssq %rcx, %xmm1
; SSE2-NEXT:    addss %xmm1, %xmm1
; SSE2-NEXT:  .LBB29_9:
; SSE2-NEXT:    unpcklps {{.*#+}} xmm2 = xmm2[0],xmm3[0],xmm2[1],xmm3[1]
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    testq %rax, %rax
; SSE2-NEXT:    js .LBB29_10
; SSE2-NEXT:  # BB#11:
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2ssq %rax, %xmm0
; SSE2-NEXT:    jmp .LBB29_12
; SSE2-NEXT:  .LBB29_10:
; SSE2-NEXT:    shrq %rax
; SSE2-NEXT:    orq %rax, %rcx
; SSE2-NEXT:    xorps %xmm0, %xmm0
; SSE2-NEXT:    cvtsi2ssq %rcx, %xmm0
; SSE2-NEXT:    addss %xmm0, %xmm0
; SSE2-NEXT:  .LBB29_12:
; SSE2-NEXT:    unpcklps {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    unpcklps {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1]
; SSE2-NEXT:    movaps %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_4vf32_4i64:
; AVX:       # BB#0:
; AVX-NEXT:    vpextrq $1, %xmm0, %rax
; AVX-NEXT:    movl %eax, %ecx
; AVX-NEXT:    andl $1, %ecx
; AVX-NEXT:    testq %rax, %rax
; AVX-NEXT:    js .LBB29_1
; AVX-NEXT:  # BB#2:
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm1
; AVX-NEXT:    jmp .LBB29_3
; AVX-NEXT:  .LBB29_1:
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    orq %rax, %rcx
; AVX-NEXT:    vcvtsi2ssq %rcx, %xmm0, %xmm1
; AVX-NEXT:    vaddss %xmm1, %xmm1, %xmm1
; AVX-NEXT:  .LBB29_3:
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    movl %eax, %ecx
; AVX-NEXT:    andl $1, %ecx
; AVX-NEXT:    testq %rax, %rax
; AVX-NEXT:    js .LBB29_4
; AVX-NEXT:  # BB#5:
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm2
; AVX-NEXT:    jmp .LBB29_6
; AVX-NEXT:  .LBB29_4:
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    orq %rax, %rcx
; AVX-NEXT:    vcvtsi2ssq %rcx, %xmm0, %xmm2
; AVX-NEXT:    vaddss %xmm2, %xmm2, %xmm2
; AVX-NEXT:  .LBB29_6:
; AVX-NEXT:    vinsertps {{.*#+}} xmm1 = xmm2[0],xmm1[0],xmm2[2,3]
; AVX-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    movl %eax, %ecx
; AVX-NEXT:    andl $1, %ecx
; AVX-NEXT:    testq %rax, %rax
; AVX-NEXT:    js .LBB29_7
; AVX-NEXT:  # BB#8:
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm2
; AVX-NEXT:    jmp .LBB29_9
; AVX-NEXT:  .LBB29_7:
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    orq %rax, %rcx
; AVX-NEXT:    vcvtsi2ssq %rcx, %xmm0, %xmm2
; AVX-NEXT:    vaddss %xmm2, %xmm2, %xmm2
; AVX-NEXT:  .LBB29_9:
; AVX-NEXT:    vinsertps {{.*#+}} xmm1 = xmm1[0,1],xmm2[0],xmm1[3]
; AVX-NEXT:    vpextrq $1, %xmm0, %rax
; AVX-NEXT:    movl %eax, %ecx
; AVX-NEXT:    andl $1, %ecx
; AVX-NEXT:    testq %rax, %rax
; AVX-NEXT:    js .LBB29_10
; AVX-NEXT:  # BB#11:
; AVX-NEXT:    vxorps %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vcvtsi2ssq %rax, %xmm0, %xmm0
; AVX-NEXT:    vinsertps {{.*#+}} xmm0 = xmm1[0,1,2],xmm0[0]
; AVX-NEXT:    vzeroupper
; AVX-NEXT:    retq
; AVX-NEXT:  .LBB29_10:
; AVX-NEXT:    shrq %rax
; AVX-NEXT:    orq %rax, %rcx
; AVX-NEXT:    vcvtsi2ssq %rcx, %xmm0, %xmm0
; AVX-NEXT:    vaddss %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vinsertps {{.*#+}} xmm0 = xmm1[0,1,2],xmm0[0]
; AVX-NEXT:    vzeroupper
; AVX-NEXT:    retq
  %cvt = uitofp <4 x i64> %a to <4 x float>
  ret <4 x float> %cvt
}

define <8 x float> @uitofp_8vf32_i16(<8 x i16> %a) {
; SSE2-LABEL: uitofp_8vf32_i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm1, %xmm1
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3]
; SSE2-NEXT:    cvtdq2ps %xmm2, %xmm2
; SSE2-NEXT:    punpckhwd {{.*#+}} xmm0 = xmm0[4,4,5,5,6,6,7,7]
; SSE2-NEXT:    pand .LCPI30_0(%rip), %xmm0
; SSE2-NEXT:    cvtdq2ps %xmm0, %xmm1
; SSE2-NEXT:    movaps %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_8vf32_i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vpunpckhwd {{.*#+}} xmm1 = xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; AVX-NEXT:    vpmovzxwd {{.*#+}} xmm0 = xmm0[0],zero,xmm0[1],zero,xmm0[2],zero,xmm0[3],zero
; AVX-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX-NEXT:    vcvtdq2ps %ymm0, %ymm0
; AVX-NEXT:    retq
  %cvt = uitofp <8 x i16> %a to <8 x float>
  ret <8 x float> %cvt
}

define <8 x float> @uitofp_8vf32_i8(<16 x i8> %a) {
; SSE2-LABEL: uitofp_8vf32_i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %xmm1, %xmm1
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3],xmm2[4],xmm1[4],xmm2[5],xmm1[5],xmm2[6],xmm1[6],xmm2[7],xmm1[7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3]
; SSE2-NEXT:    cvtdq2ps %xmm2, %xmm2
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpckhwd {{.*#+}} xmm0 = xmm0[4,4,5,5,6,6,7,7]
; SSE2-NEXT:    pand .LCPI31_0(%rip), %xmm0
; SSE2-NEXT:    cvtdq2ps %xmm0, %xmm1
; SSE2-NEXT:    movaps %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; AVX-LABEL: uitofp_8vf32_i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxbw {{.*#+}} xmm1 = xmm0[0],zero,xmm0[1],zero,xmm0[2],zero,xmm0[3],zero,xmm0[4],zero,xmm0[5],zero,xmm0[6],zero,xmm0[7],zero
; AVX-NEXT:    vpmovzxbd {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX-NEXT:    vpunpckhwd {{.*#+}} xmm1 = xmm1[4,4,5,5,6,6,7,7]
; AVX-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX-NEXT:    vandps .LCPI31_0(%rip), %ymm0, %ymm0
; AVX-NEXT:    vcvtdq2ps %ymm0, %ymm0
; AVX-NEXT:    retq
  %shuf = shufflevector <16 x i8> %a, <16 x i8> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %cvt = uitofp <8 x i8> %shuf to <8 x float>
  ret <8 x float> %cvt
}
