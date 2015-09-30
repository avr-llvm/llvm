; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+sse2 | FileCheck %s --check-prefix=SSE --check-prefix=SSE2
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+ssse3 | FileCheck %s --check-prefix=SSE --check-prefix=SSSE3
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+sse4.1 | FileCheck %s --check-prefix=SSE --check-prefix=SSE41
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+avx | FileCheck %s --check-prefix=AVX --check-prefix=AVX1
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+avx2 | FileCheck %s --check-prefix=AVX --check-prefix=AVX2
;
; Just one 32-bit run to make sure we do reasonable things there.
; RUN: llc < %s -mtriple=i686-unknown-unknown -mcpu=i686 -mattr=+sse4.1 | FileCheck %s --check-prefix=X32-SSE41

define <8 x i16> @sext_16i8_to_8i16(<16 x i8> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_16i8_to_8i16:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    psraw $8, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_16i8_to_8i16:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    psraw $8, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_16i8_to_8i16:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbw %xmm0, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: sext_16i8_to_8i16:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxbw %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: sext_16i8_to_8i16:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxbw %xmm0, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = shufflevector <16 x i8> %A, <16 x i8> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %C = sext <8 x i8> %B to <8 x i16>
  ret <8 x i16> %C
}

define <16 x i16> @sext_16i8_to_16i16(<16 x i8> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_16i8_to_16i16:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3],xmm2[4],xmm0[4],xmm2[5],xmm0[5],xmm2[6],xmm0[6],xmm2[7],xmm0[7]
; SSE2-NEXT:    psraw $8, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSE2-NEXT:    psraw $8, %xmm1
; SSE2-NEXT:    movdqa %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_16i8_to_16i16:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3],xmm2[4],xmm0[4],xmm2[5],xmm0[5],xmm2[6],xmm0[6],xmm2[7],xmm0[7]
; SSSE3-NEXT:    psraw $8, %xmm2
; SSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSSE3-NEXT:    psraw $8, %xmm1
; SSSE3-NEXT:    movdqa %xmm2, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_16i8_to_16i16:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbw %xmm0, %xmm2
; SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE41-NEXT:    pmovsxbw %xmm0, %xmm1
; SSE41-NEXT:    movdqa %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: sext_16i8_to_16i16:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxbw %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX1-NEXT:    vpmovsxbw %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: sext_16i8_to_16i16:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovsxbw %xmm0, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: sext_16i8_to_16i16:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxbw %xmm0, %xmm2
; X32-SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; X32-SSE41-NEXT:    pmovsxbw %xmm0, %xmm1
; X32-SSE41-NEXT:    movdqa %xmm2, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = sext <16 x i8> %A to <16 x i16>
  ret <16 x i16> %B
}

define <4 x i32> @sext_16i8_to_4i32(<16 x i8> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_16i8_to_4i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $24, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_16i8_to_4i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    psrad $24, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_16i8_to_4i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbd %xmm0, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: sext_16i8_to_4i32:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxbd %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: sext_16i8_to_4i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxbd %xmm0, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = shufflevector <16 x i8> %A, <16 x i8> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %C = sext <4 x i8> %B to <4 x i32>
  ret <4 x i32> %C
}

define <8 x i32> @sext_16i8_to_8i32(<16 x i8> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_16i8_to_8i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3]
; SSE2-NEXT:    psrad $24, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSE2-NEXT:    psrad $24, %xmm1
; SSE2-NEXT:    movdqa %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_16i8_to_8i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3]
; SSSE3-NEXT:    psrad $24, %xmm2
; SSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSSE3-NEXT:    psrad $24, %xmm1
; SSSE3-NEXT:    movdqa %xmm2, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_16i8_to_8i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbd %xmm0, %xmm2
; SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; SSE41-NEXT:    pmovsxbd %xmm0, %xmm1
; SSE41-NEXT:    movdqa %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: sext_16i8_to_8i32:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxbd %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; AVX1-NEXT:    vpmovsxbd %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: sext_16i8_to_8i32:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovzxbd {{.*#+}} ymm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero,xmm0[4],zero,zero,zero,xmm0[5],zero,zero,zero,xmm0[6],zero,zero,zero,xmm0[7],zero,zero,zero
; AVX2-NEXT:    vpslld $24, %ymm0, %ymm0
; AVX2-NEXT:    vpsrad $24, %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: sext_16i8_to_8i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxbd %xmm0, %xmm2
; X32-SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; X32-SSE41-NEXT:    pmovsxbd %xmm0, %xmm1
; X32-SSE41-NEXT:    movdqa %xmm2, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = shufflevector <16 x i8> %A, <16 x i8> undef, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %C = sext <8 x i8> %B to <8 x i32>
  ret <8 x i32> %C
}

define <2 x i64> @sext_16i8_to_2i64(<16 x i8> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_16i8_to_2i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    psrad $24, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_16i8_to_2i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    movdqa %xmm0, %xmm1
; SSSE3-NEXT:    psrad $31, %xmm1
; SSSE3-NEXT:    psrad $24, %xmm0
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_16i8_to_2i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbq %xmm0, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: sext_16i8_to_2i64:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxbq %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: sext_16i8_to_2i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxbq %xmm0, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = shufflevector <16 x i8> %A, <16 x i8> undef, <2 x i32> <i32 0, i32 1>
  %C = sext <2 x i8> %B to <2 x i64>
  ret <2 x i64> %C
}

define <4 x i64> @sext_16i8_to_4i64(<16 x i8> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_16i8_to_4i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3]
; SSE2-NEXT:    movdqa %xmm2, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    psrad $24, %xmm2
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; SSE2-NEXT:    psrld $16, %xmm0
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSE2-NEXT:    movdqa %xmm1, %xmm0
; SSE2-NEXT:    psrad $31, %xmm0
; SSE2-NEXT:    psrad $24, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1]
; SSE2-NEXT:    movdqa %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_16i8_to_4i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3]
; SSSE3-NEXT:    movdqa %xmm2, %xmm1
; SSSE3-NEXT:    psrad $31, %xmm1
; SSSE3-NEXT:    psrad $24, %xmm2
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; SSSE3-NEXT:    psrld $16, %xmm0
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSSE3-NEXT:    movdqa %xmm1, %xmm0
; SSSE3-NEXT:    psrad $31, %xmm0
; SSSE3-NEXT:    psrad $24, %xmm1
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1]
; SSSE3-NEXT:    movdqa %xmm2, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_16i8_to_4i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbq %xmm0, %xmm2
; SSE41-NEXT:    psrld $16, %xmm0
; SSE41-NEXT:    pmovsxbq %xmm0, %xmm1
; SSE41-NEXT:    movdqa %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: sext_16i8_to_4i64:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxbq %xmm0, %xmm1
; AVX1-NEXT:    vpsrld $16, %xmm0, %xmm0
; AVX1-NEXT:    vpmovsxbq %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: sext_16i8_to_4i64:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $24, %xmm0, %xmm0
; AVX2-NEXT:    vpsrad $24, %xmm0, %xmm0
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: sext_16i8_to_4i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxbq %xmm0, %xmm2
; X32-SSE41-NEXT:    psrld $16, %xmm0
; X32-SSE41-NEXT:    pmovsxbq %xmm0, %xmm1
; X32-SSE41-NEXT:    movdqa %xmm2, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = shufflevector <16 x i8> %A, <16 x i8> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %C = sext <4 x i8> %B to <4 x i64>
  ret <4 x i64> %C
}

define <4 x i32> @sext_8i16_to_4i32(<8 x i16> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_8i16_to_4i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $16, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_8i16_to_4i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    psrad $16, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_8i16_to_4i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxwd %xmm0, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: sext_8i16_to_4i32:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxwd %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: sext_8i16_to_4i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxwd %xmm0, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = shufflevector <8 x i16> %A, <8 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %C = sext <4 x i16> %B to <4 x i32>
  ret <4 x i32> %C
}

define <8 x i32> @sext_8i16_to_8i32(<8 x i16> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_8i16_to_8i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSE2-NEXT:    psrad $16, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSE2-NEXT:    psrad $16, %xmm1
; SSE2-NEXT:    movdqa %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_8i16_to_8i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSSE3-NEXT:    psrad $16, %xmm2
; SSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSSE3-NEXT:    psrad $16, %xmm1
; SSSE3-NEXT:    movdqa %xmm2, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_8i16_to_8i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxwd %xmm0, %xmm2
; SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE41-NEXT:    pmovsxwd %xmm0, %xmm1
; SSE41-NEXT:    movdqa %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: sext_8i16_to_8i32:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxwd %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX1-NEXT:    vpmovsxwd %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: sext_8i16_to_8i32:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovsxwd %xmm0, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: sext_8i16_to_8i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxwd %xmm0, %xmm2
; X32-SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; X32-SSE41-NEXT:    pmovsxwd %xmm0, %xmm1
; X32-SSE41-NEXT:    movdqa %xmm2, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = sext <8 x i16> %A to <8 x i32>
  ret <8 x i32> %B
}

define <2 x i64> @sext_8i16_to_2i64(<8 x i16> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_8i16_to_2i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    psrad $16, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_8i16_to_2i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    movdqa %xmm0, %xmm1
; SSSE3-NEXT:    psrad $31, %xmm1
; SSSE3-NEXT:    psrad $16, %xmm0
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_8i16_to_2i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxwq %xmm0, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: sext_8i16_to_2i64:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxwq %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: sext_8i16_to_2i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxwq %xmm0, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = shufflevector <8 x i16> %A, <8 x i16> undef, <2 x i32> <i32 0, i32 1>
  %C = sext <2 x i16> %B to <2 x i64>
  ret <2 x i64> %C
}

define <4 x i64> @sext_8i16_to_4i64(<8 x i16> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_8i16_to_4i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSE2-NEXT:    movdqa %xmm2, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    psrad $16, %xmm2
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSE2-NEXT:    movdqa %xmm1, %xmm0
; SSE2-NEXT:    psrad $31, %xmm0
; SSE2-NEXT:    psrad $16, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1]
; SSE2-NEXT:    movdqa %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_8i16_to_4i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSSE3-NEXT:    movdqa %xmm2, %xmm1
; SSSE3-NEXT:    psrad $31, %xmm1
; SSSE3-NEXT:    psrad $16, %xmm2
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; SSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSSE3-NEXT:    movdqa %xmm1, %xmm0
; SSSE3-NEXT:    psrad $31, %xmm0
; SSSE3-NEXT:    psrad $16, %xmm1
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1]
; SSSE3-NEXT:    movdqa %xmm2, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_8i16_to_4i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxwq %xmm0, %xmm2
; SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; SSE41-NEXT:    pmovsxwq %xmm0, %xmm1
; SSE41-NEXT:    movdqa %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: sext_8i16_to_4i64:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxwq %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; AVX1-NEXT:    vpmovsxwq %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: sext_8i16_to_4i64:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovzxwd {{.*#+}} xmm0 = xmm0[0],zero,xmm0[1],zero,xmm0[2],zero,xmm0[3],zero
; AVX2-NEXT:    vpslld $16, %xmm0, %xmm0
; AVX2-NEXT:    vpsrad $16, %xmm0, %xmm0
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: sext_8i16_to_4i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxwq %xmm0, %xmm2
; X32-SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; X32-SSE41-NEXT:    pmovsxwq %xmm0, %xmm1
; X32-SSE41-NEXT:    movdqa %xmm2, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = shufflevector <8 x i16> %A, <8 x i16> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %C = sext <4 x i16> %B to <4 x i64>
  ret <4 x i64> %C
}

define <2 x i64> @sext_4i32_to_2i64(<4 x i32> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_4i32_to_2i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_4i32_to_2i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movdqa %xmm0, %xmm1
; SSSE3-NEXT:    psrad $31, %xmm1
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_4i32_to_2i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxdq %xmm0, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: sext_4i32_to_2i64:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxdq %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: sext_4i32_to_2i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxdq %xmm0, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = shufflevector <4 x i32> %A, <4 x i32> undef, <2 x i32> <i32 0, i32 1>
  %C = sext <2 x i32> %B to <2 x i64>
  ret <2 x i64> %C
}

define <4 x i64> @sext_4i32_to_4i64(<4 x i32> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_4i32_to_4i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_4i32_to_4i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movdqa %xmm0, %xmm2
; SSSE3-NEXT:    psrad $31, %xmm2
; SSSE3-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSSE3-NEXT:    movdqa %xmm1, %xmm2
; SSSE3-NEXT:    psrad $31, %xmm2
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_4i32_to_4i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxdq %xmm0, %xmm2
; SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE41-NEXT:    pmovsxdq %xmm0, %xmm1
; SSE41-NEXT:    movdqa %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: sext_4i32_to_4i64:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: sext_4i32_to_4i64:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: sext_4i32_to_4i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pmovsxdq %xmm0, %xmm2
; X32-SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; X32-SSE41-NEXT:    pmovsxdq %xmm0, %xmm1
; X32-SSE41-NEXT:    movdqa %xmm2, %xmm0
; X32-SSE41-NEXT:    retl
entry:
  %B = sext <4 x i32> %A to <4 x i64>
  ret <4 x i64> %B
}

define <2 x i64> @load_sext_2i1_to_2i64(<2 x i1> *%ptr) {
; SSE-LABEL: load_sext_2i1_to_2i64:
; SSE:       # BB#0: # %entry
; SSE-NEXT:    movzbl (%rdi), %eax
; SSE-NEXT:    movq %rax, %rcx
; SSE-NEXT:    shlq $62, %rcx
; SSE-NEXT:    sarq $63, %rcx
; SSE-NEXT:    movd %rcx, %xmm1
; SSE-NEXT:    shlq $63, %rax
; SSE-NEXT:    sarq $63, %rax
; SSE-NEXT:    movd %rax, %xmm0
; SSE-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE-NEXT:    retq
;
; AVX-LABEL: load_sext_2i1_to_2i64:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    movzbl (%rdi), %eax
; AVX-NEXT:    movq %rax, %rcx
; AVX-NEXT:    shlq $62, %rcx
; AVX-NEXT:    sarq $63, %rcx
; AVX-NEXT:    vmovq %rcx, %xmm0
; AVX-NEXT:    shlq $63, %rax
; AVX-NEXT:    sarq $63, %rax
; AVX-NEXT:    vmovq %rax, %xmm1
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm1[0],xmm0[0]
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_2i1_to_2i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    movzbl (%eax), %eax
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shll $31, %ecx
; X32-SSE41-NEXT:    sarl $31, %ecx
; X32-SSE41-NEXT:    movd %ecx, %xmm0
; X32-SSE41-NEXT:    pinsrd $1, %ecx, %xmm0
; X32-SSE41-NEXT:    shll $30, %eax
; X32-SSE41-NEXT:    sarl $31, %eax
; X32-SSE41-NEXT:    pinsrd $2, %eax, %xmm0
; X32-SSE41-NEXT:    pinsrd $3, %eax, %xmm0
; X32-SSE41-NEXT:    retl
entry:
 %X = load <2 x i1>, <2 x i1>* %ptr
 %Y = sext <2 x i1> %X to <2 x i64>
 ret <2 x i64> %Y
}

define <2 x i64> @load_sext_2i8_to_2i64(<2 x i8> *%ptr) {
; SSE2-LABEL: load_sext_2i8_to_2i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movzwl (%rdi), %eax
; SSE2-NEXT:    movd %eax, %xmm0
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    psrad $24, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_2i8_to_2i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movzwl (%rdi), %eax
; SSSE3-NEXT:    movd %eax, %xmm0
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    movdqa %xmm0, %xmm1
; SSSE3-NEXT:    psrad $31, %xmm1
; SSSE3-NEXT:    psrad $24, %xmm0
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_2i8_to_2i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbq (%rdi), %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: load_sext_2i8_to_2i64:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxbq (%rdi), %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_2i8_to_2i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxbq (%eax), %xmm0
; X32-SSE41-NEXT:    retl
entry:
 %X = load <2 x i8>, <2 x i8>* %ptr
 %Y = sext <2 x i8> %X to <2 x i64>
 ret <2 x i64> %Y
}

define <4 x i32> @load_sext_4i1_to_4i32(<4 x i1> *%ptr) {
; SSE2-LABEL: load_sext_4i1_to_4i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movzbl (%rdi), %eax
; SSE2-NEXT:    movq %rax, %rcx
; SSE2-NEXT:    shlq $60, %rcx
; SSE2-NEXT:    sarq $63, %rcx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movq %rax, %rcx
; SSE2-NEXT:    shlq $62, %rcx
; SSE2-NEXT:    sarq $63, %rcx
; SSE2-NEXT:    movd %ecx, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1]
; SSE2-NEXT:    movq %rax, %rcx
; SSE2-NEXT:    shlq $61, %rcx
; SSE2-NEXT:    sarq $63, %rcx
; SSE2-NEXT:    movd %ecx, %xmm2
; SSE2-NEXT:    shlq $63, %rax
; SSE2-NEXT:    sarq $63, %rax
; SSE2-NEXT:    movd %eax, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_4i1_to_4i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movzbl (%rdi), %eax
; SSSE3-NEXT:    movq %rax, %rcx
; SSSE3-NEXT:    shlq $60, %rcx
; SSSE3-NEXT:    sarq $63, %rcx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movq %rax, %rcx
; SSSE3-NEXT:    shlq $62, %rcx
; SSSE3-NEXT:    sarq $63, %rcx
; SSSE3-NEXT:    movd %ecx, %xmm1
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1]
; SSSE3-NEXT:    movq %rax, %rcx
; SSSE3-NEXT:    shlq $61, %rcx
; SSSE3-NEXT:    sarq $63, %rcx
; SSSE3-NEXT:    movd %ecx, %xmm2
; SSSE3-NEXT:    shlq $63, %rax
; SSSE3-NEXT:    sarq $63, %rax
; SSSE3-NEXT:    movd %eax, %xmm0
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_4i1_to_4i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    movzbl (%rdi), %eax
; SSE41-NEXT:    movq %rax, %rcx
; SSE41-NEXT:    shlq $62, %rcx
; SSE41-NEXT:    sarq $63, %rcx
; SSE41-NEXT:    movq %rax, %rdx
; SSE41-NEXT:    shlq $63, %rdx
; SSE41-NEXT:    sarq $63, %rdx
; SSE41-NEXT:    movd %edx, %xmm0
; SSE41-NEXT:    pinsrd $1, %ecx, %xmm0
; SSE41-NEXT:    movq %rax, %rcx
; SSE41-NEXT:    shlq $61, %rcx
; SSE41-NEXT:    sarq $63, %rcx
; SSE41-NEXT:    pinsrd $2, %ecx, %xmm0
; SSE41-NEXT:    shlq $60, %rax
; SSE41-NEXT:    sarq $63, %rax
; SSE41-NEXT:    pinsrd $3, %eax, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: load_sext_4i1_to_4i32:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    movzbl (%rdi), %eax
; AVX-NEXT:    movq %rax, %rcx
; AVX-NEXT:    shlq $62, %rcx
; AVX-NEXT:    sarq $63, %rcx
; AVX-NEXT:    movq %rax, %rdx
; AVX-NEXT:    shlq $63, %rdx
; AVX-NEXT:    sarq $63, %rdx
; AVX-NEXT:    vmovd %edx, %xmm0
; AVX-NEXT:    vpinsrd $1, %ecx, %xmm0, %xmm0
; AVX-NEXT:    movq %rax, %rcx
; AVX-NEXT:    shlq $61, %rcx
; AVX-NEXT:    sarq $63, %rcx
; AVX-NEXT:    vpinsrd $2, %ecx, %xmm0, %xmm0
; AVX-NEXT:    shlq $60, %rax
; AVX-NEXT:    sarq $63, %rax
; AVX-NEXT:    vpinsrd $3, %eax, %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_4i1_to_4i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    movl (%eax), %eax
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shll $30, %ecx
; X32-SSE41-NEXT:    sarl $31, %ecx
; X32-SSE41-NEXT:    movl %eax, %edx
; X32-SSE41-NEXT:    shll $31, %edx
; X32-SSE41-NEXT:    sarl $31, %edx
; X32-SSE41-NEXT:    movd %edx, %xmm0
; X32-SSE41-NEXT:    pinsrd $1, %ecx, %xmm0
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shll $29, %ecx
; X32-SSE41-NEXT:    sarl $31, %ecx
; X32-SSE41-NEXT:    pinsrd $2, %ecx, %xmm0
; X32-SSE41-NEXT:    shll $28, %eax
; X32-SSE41-NEXT:    sarl $31, %eax
; X32-SSE41-NEXT:    pinsrd $3, %eax, %xmm0
; X32-SSE41-NEXT:    retl
entry:
 %X = load <4 x i1>, <4 x i1>* %ptr
 %Y = sext <4 x i1> %X to <4 x i32>
 ret <4 x i32> %Y
}

define <4 x i32> @load_sext_4i8_to_4i32(<4 x i8> *%ptr) {
; SSE2-LABEL: load_sext_4i8_to_4i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movd {{.*#+}} xmm0 = mem[0],zero,zero,zero
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $24, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_4i8_to_4i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movd {{.*#+}} xmm0 = mem[0],zero,zero,zero
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    psrad $24, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_4i8_to_4i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbd (%rdi), %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: load_sext_4i8_to_4i32:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxbd (%rdi), %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_4i8_to_4i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxbd (%eax), %xmm0
; X32-SSE41-NEXT:    retl
entry:
 %X = load <4 x i8>, <4 x i8>* %ptr
 %Y = sext <4 x i8> %X to <4 x i32>
 ret <4 x i32> %Y
}

define <4 x i64> @load_sext_4i1_to_4i64(<4 x i1> *%ptr) {
; SSE2-LABEL: load_sext_4i1_to_4i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movzbl (%rdi), %eax
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $3, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm2
; SSE2-NEXT:    shrl $2, %eax
; SSE2-NEXT:    andl $1, %eax
; SSE2-NEXT:    movd %eax, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm2[0,1,1,3]
; SSE2-NEXT:    psllq $63, %xmm0
; SSE2-NEXT:    psrad $31, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,3,3]
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm2[2,1,3,3]
; SSE2-NEXT:    psllq $63, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[1,1,3,3]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_4i1_to_4i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movzbl (%rdi), %eax
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $3, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm1
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm2
; SSSE3-NEXT:    shrl $2, %eax
; SSSE3-NEXT:    andl $1, %eax
; SSSE3-NEXT:    movd %eax, %xmm0
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1]
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; SSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm2[0,1,1,3]
; SSSE3-NEXT:    psllq $63, %xmm0
; SSSE3-NEXT:    psrad $31, %xmm0
; SSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,3,3]
; SSSE3-NEXT:    pshufd {{.*#+}} xmm1 = xmm2[2,1,3,3]
; SSSE3-NEXT:    psllq $63, %xmm1
; SSSE3-NEXT:    psrad $31, %xmm1
; SSSE3-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[1,1,3,3]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_4i1_to_4i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    movzbl (%rdi), %eax
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    movl %eax, %edx
; SSE41-NEXT:    andl $1, %edx
; SSE41-NEXT:    movd %edx, %xmm1
; SSE41-NEXT:    pinsrd $1, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $2, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrd $2, %ecx, %xmm1
; SSE41-NEXT:    shrl $3, %eax
; SSE41-NEXT:    andl $1, %eax
; SSE41-NEXT:    pinsrd $3, %eax, %xmm1
; SSE41-NEXT:    pmovzxdq {{.*#+}} xmm0 = xmm1[0],zero,xmm1[1],zero
; SSE41-NEXT:    psllq $63, %xmm0
; SSE41-NEXT:    psrad $31, %xmm0
; SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,3,3]
; SSE41-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[2,2,3,3]
; SSE41-NEXT:    psllq $63, %xmm1
; SSE41-NEXT:    psrad $31, %xmm1
; SSE41-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[1,1,3,3]
; SSE41-NEXT:    retq
;
; AVX1-LABEL: load_sext_4i1_to_4i64:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    movzbl (%rdi), %eax
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $62, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    movq %rax, %rdx
; AVX1-NEXT:    shlq $63, %rdx
; AVX1-NEXT:    sarq $63, %rdx
; AVX1-NEXT:    vmovd %edx, %xmm0
; AVX1-NEXT:    vpinsrd $1, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $61, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrd $2, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    shlq $60, %rax
; AVX1-NEXT:    sarq $63, %rax
; AVX1-NEXT:    vpinsrd $3, %eax, %xmm0, %xmm0
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: load_sext_4i1_to_4i64:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    movzbl (%rdi), %eax
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $60, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vmovq %rcx, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $61, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vmovq %rcx, %xmm1
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm1[0],xmm0[0]
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $62, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vmovq %rcx, %xmm1
; AVX2-NEXT:    shlq $63, %rax
; AVX2-NEXT:    sarq $63, %rax
; AVX2-NEXT:    vmovq %rax, %xmm2
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} xmm1 = xmm2[0],xmm1[0]
; AVX2-NEXT:    vinserti128 $1, %xmm0, %ymm1, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_4i1_to_4i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    movzbl (%eax), %eax
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    movl %eax, %edx
; X32-SSE41-NEXT:    andl $1, %edx
; X32-SSE41-NEXT:    movd %edx, %xmm1
; X32-SSE41-NEXT:    pinsrd $1, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $2, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrd $2, %ecx, %xmm1
; X32-SSE41-NEXT:    shrl $3, %eax
; X32-SSE41-NEXT:    andl $1, %eax
; X32-SSE41-NEXT:    pinsrd $3, %eax, %xmm1
; X32-SSE41-NEXT:    pmovzxdq {{.*#+}} xmm0 = xmm1[0],zero,xmm1[1],zero
; X32-SSE41-NEXT:    psllq $63, %xmm0
; X32-SSE41-NEXT:    psrad $31, %xmm0
; X32-SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,3,3]
; X32-SSE41-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[2,2,3,3]
; X32-SSE41-NEXT:    psllq $63, %xmm1
; X32-SSE41-NEXT:    psrad $31, %xmm1
; X32-SSE41-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[1,1,3,3]
; X32-SSE41-NEXT:    retl
entry:
 %X = load <4 x i1>, <4 x i1>* %ptr
 %Y = sext <4 x i1> %X to <4 x i64>
 ret <4 x i64> %Y
}

define <4 x i64> @load_sext_4i8_to_4i64(<4 x i8> *%ptr) {
; SSE2-LABEL: load_sext_4i8_to_4i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movsbq 1(%rdi), %rax
; SSE2-NEXT:    movd %rax, %xmm1
; SSE2-NEXT:    movsbq (%rdi), %rax
; SSE2-NEXT:    movd %rax, %xmm0
; SSE2-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE2-NEXT:    movsbq 3(%rdi), %rax
; SSE2-NEXT:    movd %rax, %xmm2
; SSE2-NEXT:    movsbq 2(%rdi), %rax
; SSE2-NEXT:    movd %rax, %xmm1
; SSE2-NEXT:    punpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_4i8_to_4i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movsbq 1(%rdi), %rax
; SSSE3-NEXT:    movd %rax, %xmm1
; SSSE3-NEXT:    movsbq (%rdi), %rax
; SSSE3-NEXT:    movd %rax, %xmm0
; SSSE3-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSSE3-NEXT:    movsbq 3(%rdi), %rax
; SSSE3-NEXT:    movd %rax, %xmm2
; SSSE3-NEXT:    movsbq 2(%rdi), %rax
; SSSE3-NEXT:    movd %rax, %xmm1
; SSSE3-NEXT:    punpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_4i8_to_4i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbq (%rdi), %xmm0
; SSE41-NEXT:    pmovsxbq 2(%rdi), %xmm1
; SSE41-NEXT:    retq
;
; AVX1-LABEL: load_sext_4i8_to_4i64:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxbd (%rdi), %xmm0
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: load_sext_4i8_to_4i64:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovsxbq (%rdi), %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_4i8_to_4i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxbq (%eax), %xmm0
; X32-SSE41-NEXT:    pmovsxbq 2(%eax), %xmm1
; X32-SSE41-NEXT:    retl
entry:
 %X = load <4 x i8>, <4 x i8>* %ptr
 %Y = sext <4 x i8> %X to <4 x i64>
 ret <4 x i64> %Y
}

define <8 x i16> @load_sext_8i1_to_8i16(<8 x i1> *%ptr) {
; SSE2-LABEL: load_sext_8i1_to_8i16:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movzbl (%rdi), %eax
; SSE2-NEXT:    movq %rax, %rcx
; SSE2-NEXT:    shlq $56, %rcx
; SSE2-NEXT:    sarq $63, %rcx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movq %rax, %rcx
; SSE2-NEXT:    shlq $60, %rcx
; SSE2-NEXT:    sarq $63, %rcx
; SSE2-NEXT:    movd %ecx, %xmm2
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSE2-NEXT:    movq %rax, %rcx
; SSE2-NEXT:    shlq $58, %rcx
; SSE2-NEXT:    sarq $63, %rcx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movq %rax, %rcx
; SSE2-NEXT:    shlq $62, %rcx
; SSE2-NEXT:    sarq $63, %rcx
; SSE2-NEXT:    movd %ecx, %xmm1
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3]
; SSE2-NEXT:    movq %rax, %rcx
; SSE2-NEXT:    shlq $57, %rcx
; SSE2-NEXT:    sarq $63, %rcx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movq %rax, %rcx
; SSE2-NEXT:    shlq $61, %rcx
; SSE2-NEXT:    sarq $63, %rcx
; SSE2-NEXT:    movd %ecx, %xmm2
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSE2-NEXT:    movq %rax, %rcx
; SSE2-NEXT:    shlq $59, %rcx
; SSE2-NEXT:    sarq $63, %rcx
; SSE2-NEXT:    movd %ecx, %xmm3
; SSE2-NEXT:    shlq $63, %rax
; SSE2-NEXT:    sarq $63, %rax
; SSE2-NEXT:    movd %eax, %xmm0
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_8i1_to_8i16:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movzbl (%rdi), %eax
; SSSE3-NEXT:    movq %rax, %rcx
; SSSE3-NEXT:    shlq $56, %rcx
; SSSE3-NEXT:    sarq $63, %rcx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movq %rax, %rcx
; SSSE3-NEXT:    shlq $60, %rcx
; SSSE3-NEXT:    sarq $63, %rcx
; SSSE3-NEXT:    movd %ecx, %xmm2
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSSE3-NEXT:    movq %rax, %rcx
; SSSE3-NEXT:    shlq $58, %rcx
; SSSE3-NEXT:    sarq $63, %rcx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movq %rax, %rcx
; SSSE3-NEXT:    shlq $62, %rcx
; SSSE3-NEXT:    sarq $63, %rcx
; SSSE3-NEXT:    movd %ecx, %xmm1
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3]
; SSSE3-NEXT:    movq %rax, %rcx
; SSSE3-NEXT:    shlq $57, %rcx
; SSSE3-NEXT:    sarq $63, %rcx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movq %rax, %rcx
; SSSE3-NEXT:    shlq $61, %rcx
; SSSE3-NEXT:    sarq $63, %rcx
; SSSE3-NEXT:    movd %ecx, %xmm2
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSSE3-NEXT:    movq %rax, %rcx
; SSSE3-NEXT:    shlq $59, %rcx
; SSSE3-NEXT:    sarq $63, %rcx
; SSSE3-NEXT:    movd %ecx, %xmm3
; SSSE3-NEXT:    shlq $63, %rax
; SSSE3-NEXT:    sarq $63, %rax
; SSSE3-NEXT:    movd %eax, %xmm0
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_8i1_to_8i16:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    movzbl (%rdi), %eax
; SSE41-NEXT:    movq %rax, %rcx
; SSE41-NEXT:    shlq $62, %rcx
; SSE41-NEXT:    sarq $63, %rcx
; SSE41-NEXT:    movq %rax, %rdx
; SSE41-NEXT:    shlq $63, %rdx
; SSE41-NEXT:    sarq $63, %rdx
; SSE41-NEXT:    movd %edx, %xmm0
; SSE41-NEXT:    pinsrw $1, %ecx, %xmm0
; SSE41-NEXT:    movq %rax, %rcx
; SSE41-NEXT:    shlq $61, %rcx
; SSE41-NEXT:    sarq $63, %rcx
; SSE41-NEXT:    pinsrw $2, %ecx, %xmm0
; SSE41-NEXT:    movq %rax, %rcx
; SSE41-NEXT:    shlq $60, %rcx
; SSE41-NEXT:    sarq $63, %rcx
; SSE41-NEXT:    pinsrw $3, %ecx, %xmm0
; SSE41-NEXT:    movq %rax, %rcx
; SSE41-NEXT:    shlq $59, %rcx
; SSE41-NEXT:    sarq $63, %rcx
; SSE41-NEXT:    pinsrw $4, %ecx, %xmm0
; SSE41-NEXT:    movq %rax, %rcx
; SSE41-NEXT:    shlq $58, %rcx
; SSE41-NEXT:    sarq $63, %rcx
; SSE41-NEXT:    pinsrw $5, %ecx, %xmm0
; SSE41-NEXT:    movq %rax, %rcx
; SSE41-NEXT:    shlq $57, %rcx
; SSE41-NEXT:    sarq $63, %rcx
; SSE41-NEXT:    pinsrw $6, %ecx, %xmm0
; SSE41-NEXT:    shlq $56, %rax
; SSE41-NEXT:    sarq $63, %rax
; SSE41-NEXT:    pinsrw $7, %eax, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: load_sext_8i1_to_8i16:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    movzbl (%rdi), %eax
; AVX-NEXT:    movq %rax, %rcx
; AVX-NEXT:    shlq $62, %rcx
; AVX-NEXT:    sarq $63, %rcx
; AVX-NEXT:    movq %rax, %rdx
; AVX-NEXT:    shlq $63, %rdx
; AVX-NEXT:    sarq $63, %rdx
; AVX-NEXT:    vmovd %edx, %xmm0
; AVX-NEXT:    vpinsrw $1, %ecx, %xmm0, %xmm0
; AVX-NEXT:    movq %rax, %rcx
; AVX-NEXT:    shlq $61, %rcx
; AVX-NEXT:    sarq $63, %rcx
; AVX-NEXT:    vpinsrw $2, %ecx, %xmm0, %xmm0
; AVX-NEXT:    movq %rax, %rcx
; AVX-NEXT:    shlq $60, %rcx
; AVX-NEXT:    sarq $63, %rcx
; AVX-NEXT:    vpinsrw $3, %ecx, %xmm0, %xmm0
; AVX-NEXT:    movq %rax, %rcx
; AVX-NEXT:    shlq $59, %rcx
; AVX-NEXT:    sarq $63, %rcx
; AVX-NEXT:    vpinsrw $4, %ecx, %xmm0, %xmm0
; AVX-NEXT:    movq %rax, %rcx
; AVX-NEXT:    shlq $58, %rcx
; AVX-NEXT:    sarq $63, %rcx
; AVX-NEXT:    vpinsrw $5, %ecx, %xmm0, %xmm0
; AVX-NEXT:    movq %rax, %rcx
; AVX-NEXT:    shlq $57, %rcx
; AVX-NEXT:    sarq $63, %rcx
; AVX-NEXT:    vpinsrw $6, %ecx, %xmm0, %xmm0
; AVX-NEXT:    shlq $56, %rax
; AVX-NEXT:    sarq $63, %rax
; AVX-NEXT:    vpinsrw $7, %eax, %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_8i1_to_8i16:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    movl (%eax), %eax
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shll $30, %ecx
; X32-SSE41-NEXT:    sarl $31, %ecx
; X32-SSE41-NEXT:    movl %eax, %edx
; X32-SSE41-NEXT:    shll $31, %edx
; X32-SSE41-NEXT:    sarl $31, %edx
; X32-SSE41-NEXT:    movd %edx, %xmm0
; X32-SSE41-NEXT:    pinsrw $1, %ecx, %xmm0
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shll $29, %ecx
; X32-SSE41-NEXT:    sarl $31, %ecx
; X32-SSE41-NEXT:    pinsrw $2, %ecx, %xmm0
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shll $28, %ecx
; X32-SSE41-NEXT:    sarl $31, %ecx
; X32-SSE41-NEXT:    pinsrw $3, %ecx, %xmm0
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shll $27, %ecx
; X32-SSE41-NEXT:    sarl $31, %ecx
; X32-SSE41-NEXT:    pinsrw $4, %ecx, %xmm0
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shll $26, %ecx
; X32-SSE41-NEXT:    sarl $31, %ecx
; X32-SSE41-NEXT:    pinsrw $5, %ecx, %xmm0
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shll $25, %ecx
; X32-SSE41-NEXT:    sarl $31, %ecx
; X32-SSE41-NEXT:    pinsrw $6, %ecx, %xmm0
; X32-SSE41-NEXT:    shll $24, %eax
; X32-SSE41-NEXT:    sarl $31, %eax
; X32-SSE41-NEXT:    pinsrw $7, %eax, %xmm0
; X32-SSE41-NEXT:    retl
entry:
 %X = load <8 x i1>, <8 x i1>* %ptr
 %Y = sext <8 x i1> %X to <8 x i16>
 ret <8 x i16> %Y
}

define <8 x i16> @load_sext_8i8_to_8i16(<8 x i8> *%ptr) {
; SSE2-LABEL: load_sext_8i8_to_8i16:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movq {{.*#+}} xmm0 = mem[0],zero
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    psraw $8, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_8i8_to_8i16:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movq {{.*#+}} xmm0 = mem[0],zero
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    psraw $8, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_8i8_to_8i16:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbw (%rdi), %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: load_sext_8i8_to_8i16:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxbw (%rdi), %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_8i8_to_8i16:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxbw (%eax), %xmm0
; X32-SSE41-NEXT:    retl
entry:
 %X = load <8 x i8>, <8 x i8>* %ptr
 %Y = sext <8 x i8> %X to <8 x i16>
 ret <8 x i16> %Y
}

define <8 x i32> @load_sext_8i1_to_8i32(<8 x i1> *%ptr) {
; SSE2-LABEL: load_sext_8i1_to_8i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movzbl (%rdi), %eax
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $6, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $2, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm2
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm1
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $4, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $5, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm2
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $3, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    shrl $7, %eax
; SSE2-NEXT:    movzwl %ax, %eax
; SSE2-NEXT:    movd %eax, %xmm3
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3]
; SSE2-NEXT:    movdqa %xmm1, %xmm0
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    pslld $31, %xmm0
; SSE2-NEXT:    psrad $31, %xmm0
; SSE2-NEXT:    punpckhwd {{.*#+}} xmm1 = xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSE2-NEXT:    pslld $31, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_8i1_to_8i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movzbl (%rdi), %eax
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $6, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $2, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm2
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm1
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $4, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $5, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm2
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $3, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    shrl $7, %eax
; SSSE3-NEXT:    movzwl %ax, %eax
; SSSE3-NEXT:    movd %eax, %xmm3
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3]
; SSSE3-NEXT:    movdqa %xmm1, %xmm0
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    pslld $31, %xmm0
; SSSE3-NEXT:    psrad $31, %xmm0
; SSSE3-NEXT:    punpckhwd {{.*#+}} xmm1 = xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSSE3-NEXT:    pslld $31, %xmm1
; SSSE3-NEXT:    psrad $31, %xmm1
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_8i1_to_8i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    movzbl (%rdi), %eax
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    movl %eax, %edx
; SSE41-NEXT:    andl $1, %edx
; SSE41-NEXT:    movd %edx, %xmm1
; SSE41-NEXT:    pinsrw $1, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $2, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrw $2, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $3, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrw $3, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $4, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrw $4, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $5, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrw $5, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $6, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrw $6, %ecx, %xmm1
; SSE41-NEXT:    shrl $7, %eax
; SSE41-NEXT:    movzwl %ax, %eax
; SSE41-NEXT:    pinsrw $7, %eax, %xmm1
; SSE41-NEXT:    pmovzxwd {{.*#+}} xmm0 = xmm1[0],zero,xmm1[1],zero,xmm1[2],zero,xmm1[3],zero
; SSE41-NEXT:    pslld $31, %xmm0
; SSE41-NEXT:    psrad $31, %xmm0
; SSE41-NEXT:    punpckhwd {{.*#+}} xmm1 = xmm1[4,4,5,5,6,6,7,7]
; SSE41-NEXT:    pslld $31, %xmm1
; SSE41-NEXT:    psrad $31, %xmm1
; SSE41-NEXT:    retq
;
; AVX1-LABEL: load_sext_8i1_to_8i32:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    movzbl (%rdi), %eax
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $58, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    movq %rax, %rdx
; AVX1-NEXT:    shlq $59, %rdx
; AVX1-NEXT:    sarq $63, %rdx
; AVX1-NEXT:    vmovd %edx, %xmm0
; AVX1-NEXT:    vpinsrd $1, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $57, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrd $2, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $56, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrd $3, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $62, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    movq %rax, %rdx
; AVX1-NEXT:    shlq $63, %rdx
; AVX1-NEXT:    sarq $63, %rdx
; AVX1-NEXT:    vmovd %edx, %xmm1
; AVX1-NEXT:    vpinsrd $1, %ecx, %xmm1, %xmm1
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $61, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrd $2, %ecx, %xmm1, %xmm1
; AVX1-NEXT:    shlq $60, %rax
; AVX1-NEXT:    sarq $63, %rax
; AVX1-NEXT:    vpinsrd $3, %eax, %xmm1, %xmm1
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: load_sext_8i1_to_8i32:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    movzbl (%rdi), %eax
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $58, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    movq %rax, %rdx
; AVX2-NEXT:    shlq $59, %rdx
; AVX2-NEXT:    sarq $63, %rdx
; AVX2-NEXT:    vmovd %edx, %xmm0
; AVX2-NEXT:    vpinsrd $1, %ecx, %xmm0, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $57, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrd $2, %ecx, %xmm0, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $56, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrd $3, %ecx, %xmm0, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $62, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    movq %rax, %rdx
; AVX2-NEXT:    shlq $63, %rdx
; AVX2-NEXT:    sarq $63, %rdx
; AVX2-NEXT:    vmovd %edx, %xmm1
; AVX2-NEXT:    vpinsrd $1, %ecx, %xmm1, %xmm1
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $61, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrd $2, %ecx, %xmm1, %xmm1
; AVX2-NEXT:    shlq $60, %rax
; AVX2-NEXT:    sarq $63, %rax
; AVX2-NEXT:    vpinsrd $3, %eax, %xmm1, %xmm1
; AVX2-NEXT:    vinserti128 $1, %xmm0, %ymm1, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_8i1_to_8i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    movzbl (%eax), %eax
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    movl %eax, %edx
; X32-SSE41-NEXT:    andl $1, %edx
; X32-SSE41-NEXT:    movd %edx, %xmm1
; X32-SSE41-NEXT:    pinsrw $1, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $2, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrw $2, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $3, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrw $3, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $4, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrw $4, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $5, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrw $5, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $6, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrw $6, %ecx, %xmm1
; X32-SSE41-NEXT:    shrl $7, %eax
; X32-SSE41-NEXT:    pinsrw $7, %eax, %xmm1
; X32-SSE41-NEXT:    pmovzxwd {{.*#+}} xmm0 = xmm1[0],zero,xmm1[1],zero,xmm1[2],zero,xmm1[3],zero
; X32-SSE41-NEXT:    pslld $31, %xmm0
; X32-SSE41-NEXT:    psrad $31, %xmm0
; X32-SSE41-NEXT:    punpckhwd {{.*#+}} xmm1 = xmm1[4,4,5,5,6,6,7,7]
; X32-SSE41-NEXT:    pslld $31, %xmm1
; X32-SSE41-NEXT:    psrad $31, %xmm1
; X32-SSE41-NEXT:    retl
entry:
 %X = load <8 x i1>, <8 x i1>* %ptr
 %Y = sext <8 x i1> %X to <8 x i32>
 ret <8 x i32> %Y
}

define <8 x i32> @load_sext_8i8_to_8i32(<8 x i8> *%ptr) {
; SSE2-LABEL: load_sext_8i8_to_8i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movd {{.*#+}} xmm0 = mem[0],zero,zero,zero
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $24, %xmm0
; SSE2-NEXT:    movd {{.*#+}} xmm1 = mem[0],zero,zero,zero
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $24, %xmm1
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_8i8_to_8i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movd {{.*#+}} xmm0 = mem[0],zero,zero,zero
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    psrad $24, %xmm0
; SSSE3-NEXT:    movd {{.*#+}} xmm1 = mem[0],zero,zero,zero
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    psrad $24, %xmm1
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_8i8_to_8i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbd (%rdi), %xmm0
; SSE41-NEXT:    pmovsxbd 4(%rdi), %xmm1
; SSE41-NEXT:    retq
;
; AVX1-LABEL: load_sext_8i8_to_8i32:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxbw (%rdi), %xmm0
; AVX1-NEXT:    vpmovsxwd %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX1-NEXT:    vpmovsxwd %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: load_sext_8i8_to_8i32:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovsxbd (%rdi), %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_8i8_to_8i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxbd (%eax), %xmm0
; X32-SSE41-NEXT:    pmovsxbd 4(%eax), %xmm1
; X32-SSE41-NEXT:    retl
entry:
 %X = load <8 x i8>, <8 x i8>* %ptr
 %Y = sext <8 x i8> %X to <8 x i32>
 ret <8 x i32> %Y
}

define <16 x i16> @load_sext_16i1_to_16i16(<16 x i1> *%ptr) {
; SSE2-LABEL: load_sext_16i1_to_16i16:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movzwl (%rdi), %eax
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $14, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $6, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm1
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $10, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $2, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm2
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3],xmm2[4],xmm0[4],xmm2[5],xmm0[5],xmm2[6],xmm0[6],xmm2[7],xmm0[7]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3],xmm2[4],xmm1[4],xmm2[5],xmm1[5],xmm2[6],xmm1[6],xmm2[7],xmm1[7]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $12, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $4, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm3
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm3 = xmm3[0],xmm0[0],xmm3[1],xmm0[1],xmm3[2],xmm0[2],xmm3[3],xmm0[3],xmm3[4],xmm0[4],xmm3[5],xmm0[5],xmm3[6],xmm0[6],xmm3[7],xmm0[7]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm1
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $8, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm3[0],xmm1[1],xmm3[1],xmm1[2],xmm3[2],xmm1[3],xmm3[3],xmm1[4],xmm3[4],xmm1[5],xmm3[5],xmm1[6],xmm3[6],xmm1[7],xmm3[7]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3],xmm1[4],xmm2[4],xmm1[5],xmm2[5],xmm1[6],xmm2[6],xmm1[7],xmm2[7]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $13, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $5, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm2
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3],xmm2[4],xmm0[4],xmm2[5],xmm0[5],xmm2[6],xmm0[6],xmm2[7],xmm0[7]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $9, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm3
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm0
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3],xmm0[4],xmm3[4],xmm0[5],xmm3[5],xmm0[6],xmm3[6],xmm0[7],xmm3[7]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3],xmm0[4],xmm2[4],xmm0[5],xmm2[5],xmm0[6],xmm2[6],xmm0[7],xmm2[7]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $11, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm2
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $3, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm3
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm3 = xmm3[0],xmm2[0],xmm3[1],xmm2[1],xmm3[2],xmm2[2],xmm3[3],xmm2[3],xmm3[4],xmm2[4],xmm3[5],xmm2[5],xmm3[6],xmm2[6],xmm3[7],xmm2[7]
; SSE2-NEXT:    movl %eax, %ecx
; SSE2-NEXT:    shrl $7, %ecx
; SSE2-NEXT:    andl $1, %ecx
; SSE2-NEXT:    movd %ecx, %xmm2
; SSE2-NEXT:    shrl $15, %eax
; SSE2-NEXT:    movzwl %ax, %eax
; SSE2-NEXT:    movd %eax, %xmm4
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm4[0],xmm2[1],xmm4[1],xmm2[2],xmm4[2],xmm2[3],xmm4[3],xmm2[4],xmm4[4],xmm2[5],xmm4[5],xmm2[6],xmm4[6],xmm2[7],xmm4[7]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm3 = xmm3[0],xmm2[0],xmm3[1],xmm2[1],xmm3[2],xmm2[2],xmm3[3],xmm2[3],xmm3[4],xmm2[4],xmm3[5],xmm2[5],xmm3[6],xmm2[6],xmm3[7],xmm2[7]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3],xmm0[4],xmm3[4],xmm0[5],xmm3[5],xmm0[6],xmm3[6],xmm0[7],xmm3[7]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSE2-NEXT:    movdqa %xmm1, %xmm0
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    psllw $15, %xmm0
; SSE2-NEXT:    psraw $15, %xmm0
; SSE2-NEXT:    punpckhbw {{.*#+}} xmm1 = xmm1[8],xmm0[8],xmm1[9],xmm0[9],xmm1[10],xmm0[10],xmm1[11],xmm0[11],xmm1[12],xmm0[12],xmm1[13],xmm0[13],xmm1[14],xmm0[14],xmm1[15],xmm0[15]
; SSE2-NEXT:    psllw $15, %xmm1
; SSE2-NEXT:    psraw $15, %xmm1
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_16i1_to_16i16:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movzwl (%rdi), %eax
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $14, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $6, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm1
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $10, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $2, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm2
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3],xmm2[4],xmm0[4],xmm2[5],xmm0[5],xmm2[6],xmm0[6],xmm2[7],xmm0[7]
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1],xmm2[2],xmm1[2],xmm2[3],xmm1[3],xmm2[4],xmm1[4],xmm2[5],xmm1[5],xmm2[6],xmm1[6],xmm2[7],xmm1[7]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $12, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $4, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm3
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm3 = xmm3[0],xmm0[0],xmm3[1],xmm0[1],xmm3[2],xmm0[2],xmm3[3],xmm0[3],xmm3[4],xmm0[4],xmm3[5],xmm0[5],xmm3[6],xmm0[6],xmm3[7],xmm0[7]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm1
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $8, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm3[0],xmm1[1],xmm3[1],xmm1[2],xmm3[2],xmm1[3],xmm3[3],xmm1[4],xmm3[4],xmm1[5],xmm3[5],xmm1[6],xmm3[6],xmm1[7],xmm3[7]
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3],xmm1[4],xmm2[4],xmm1[5],xmm2[5],xmm1[6],xmm2[6],xmm1[7],xmm2[7]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $13, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $5, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm2
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1],xmm2[2],xmm0[2],xmm2[3],xmm0[3],xmm2[4],xmm0[4],xmm2[5],xmm0[5],xmm2[6],xmm0[6],xmm2[7],xmm0[7]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $9, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm3
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm0
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3],xmm0[4],xmm3[4],xmm0[5],xmm3[5],xmm0[6],xmm3[6],xmm0[7],xmm3[7]
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1],xmm0[2],xmm2[2],xmm0[3],xmm2[3],xmm0[4],xmm2[4],xmm0[5],xmm2[5],xmm0[6],xmm2[6],xmm0[7],xmm2[7]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $11, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm2
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $3, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm3
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm3 = xmm3[0],xmm2[0],xmm3[1],xmm2[1],xmm3[2],xmm2[2],xmm3[3],xmm2[3],xmm3[4],xmm2[4],xmm3[5],xmm2[5],xmm3[6],xmm2[6],xmm3[7],xmm2[7]
; SSSE3-NEXT:    movl %eax, %ecx
; SSSE3-NEXT:    shrl $7, %ecx
; SSSE3-NEXT:    andl $1, %ecx
; SSSE3-NEXT:    movd %ecx, %xmm2
; SSSE3-NEXT:    shrl $15, %eax
; SSSE3-NEXT:    movzwl %ax, %eax
; SSSE3-NEXT:    movd %eax, %xmm4
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm2 = xmm2[0],xmm4[0],xmm2[1],xmm4[1],xmm2[2],xmm4[2],xmm2[3],xmm4[3],xmm2[4],xmm4[4],xmm2[5],xmm4[5],xmm2[6],xmm4[6],xmm2[7],xmm4[7]
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm3 = xmm3[0],xmm2[0],xmm3[1],xmm2[1],xmm3[2],xmm2[2],xmm3[3],xmm2[3],xmm3[4],xmm2[4],xmm3[5],xmm2[5],xmm3[6],xmm2[6],xmm3[7],xmm2[7]
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3],xmm0[4],xmm3[4],xmm0[5],xmm3[5],xmm0[6],xmm3[6],xmm0[7],xmm3[7]
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm0[0],xmm1[1],xmm0[1],xmm1[2],xmm0[2],xmm1[3],xmm0[3],xmm1[4],xmm0[4],xmm1[5],xmm0[5],xmm1[6],xmm0[6],xmm1[7],xmm0[7]
; SSSE3-NEXT:    movdqa %xmm1, %xmm0
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    psllw $15, %xmm0
; SSSE3-NEXT:    psraw $15, %xmm0
; SSSE3-NEXT:    punpckhbw {{.*#+}} xmm1 = xmm1[8],xmm0[8],xmm1[9],xmm0[9],xmm1[10],xmm0[10],xmm1[11],xmm0[11],xmm1[12],xmm0[12],xmm1[13],xmm0[13],xmm1[14],xmm0[14],xmm1[15],xmm0[15]
; SSSE3-NEXT:    psllw $15, %xmm1
; SSSE3-NEXT:    psraw $15, %xmm1
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_16i1_to_16i16:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    movzwl (%rdi), %eax
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    movl %eax, %edx
; SSE41-NEXT:    andl $1, %edx
; SSE41-NEXT:    movd %edx, %xmm1
; SSE41-NEXT:    pinsrb $1, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $2, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $2, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $3, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $3, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $4, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $4, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $5, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $5, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $6, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $6, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $7, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $7, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $8, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $8, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $9, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $9, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $10, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $10, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $11, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $11, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $12, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $12, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $13, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $13, %ecx, %xmm1
; SSE41-NEXT:    movl %eax, %ecx
; SSE41-NEXT:    shrl $14, %ecx
; SSE41-NEXT:    andl $1, %ecx
; SSE41-NEXT:    pinsrb $14, %ecx, %xmm1
; SSE41-NEXT:    shrl $15, %eax
; SSE41-NEXT:    movzwl %ax, %eax
; SSE41-NEXT:    pinsrb $15, %eax, %xmm1
; SSE41-NEXT:    pmovzxbw {{.*#+}} xmm0 = xmm1[0],zero,xmm1[1],zero,xmm1[2],zero,xmm1[3],zero,xmm1[4],zero,xmm1[5],zero,xmm1[6],zero,xmm1[7],zero
; SSE41-NEXT:    psllw $15, %xmm0
; SSE41-NEXT:    psraw $15, %xmm0
; SSE41-NEXT:    punpckhbw {{.*#+}} xmm1 = xmm1[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15]
; SSE41-NEXT:    psllw $15, %xmm1
; SSE41-NEXT:    psraw $15, %xmm1
; SSE41-NEXT:    retq
;
; AVX1-LABEL: load_sext_16i1_to_16i16:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    movzwl (%rdi), %eax
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $54, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    movq %rax, %rdx
; AVX1-NEXT:    shlq $55, %rdx
; AVX1-NEXT:    sarq $63, %rdx
; AVX1-NEXT:    vmovd %edx, %xmm0
; AVX1-NEXT:    vpinsrw $1, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $53, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $2, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $52, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $3, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $51, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $4, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $50, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $5, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $49, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $6, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $48, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $7, %ecx, %xmm0, %xmm0
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $62, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    movq %rax, %rdx
; AVX1-NEXT:    shlq $63, %rdx
; AVX1-NEXT:    sarq $63, %rdx
; AVX1-NEXT:    vmovd %edx, %xmm1
; AVX1-NEXT:    vpinsrw $1, %ecx, %xmm1, %xmm1
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $61, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $2, %ecx, %xmm1, %xmm1
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $60, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $3, %ecx, %xmm1, %xmm1
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $59, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $4, %ecx, %xmm1, %xmm1
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $58, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $5, %ecx, %xmm1, %xmm1
; AVX1-NEXT:    movq %rax, %rcx
; AVX1-NEXT:    shlq $57, %rcx
; AVX1-NEXT:    sarq $63, %rcx
; AVX1-NEXT:    vpinsrw $6, %ecx, %xmm1, %xmm1
; AVX1-NEXT:    shlq $56, %rax
; AVX1-NEXT:    sarq $63, %rax
; AVX1-NEXT:    vpinsrw $7, %eax, %xmm1, %xmm1
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: load_sext_16i1_to_16i16:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    movzwl (%rdi), %eax
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $54, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    movq %rax, %rdx
; AVX2-NEXT:    shlq $55, %rdx
; AVX2-NEXT:    sarq $63, %rdx
; AVX2-NEXT:    vmovd %edx, %xmm0
; AVX2-NEXT:    vpinsrw $1, %ecx, %xmm0, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $53, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $2, %ecx, %xmm0, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $52, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $3, %ecx, %xmm0, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $51, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $4, %ecx, %xmm0, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $50, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $5, %ecx, %xmm0, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $49, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $6, %ecx, %xmm0, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $48, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $7, %ecx, %xmm0, %xmm0
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $62, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    movq %rax, %rdx
; AVX2-NEXT:    shlq $63, %rdx
; AVX2-NEXT:    sarq $63, %rdx
; AVX2-NEXT:    vmovd %edx, %xmm1
; AVX2-NEXT:    vpinsrw $1, %ecx, %xmm1, %xmm1
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $61, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $2, %ecx, %xmm1, %xmm1
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $60, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $3, %ecx, %xmm1, %xmm1
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $59, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $4, %ecx, %xmm1, %xmm1
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $58, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $5, %ecx, %xmm1, %xmm1
; AVX2-NEXT:    movq %rax, %rcx
; AVX2-NEXT:    shlq $57, %rcx
; AVX2-NEXT:    sarq $63, %rcx
; AVX2-NEXT:    vpinsrw $6, %ecx, %xmm1, %xmm1
; AVX2-NEXT:    shlq $56, %rax
; AVX2-NEXT:    sarq $63, %rax
; AVX2-NEXT:    vpinsrw $7, %eax, %xmm1, %xmm1
; AVX2-NEXT:    vinserti128 $1, %xmm0, %ymm1, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_16i1_to_16i16:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    movzwl (%eax), %eax
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    movl %eax, %edx
; X32-SSE41-NEXT:    andl $1, %edx
; X32-SSE41-NEXT:    movd %edx, %xmm1
; X32-SSE41-NEXT:    pinsrb $1, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $2, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $2, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $3, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $3, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $4, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $4, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $5, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $5, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $6, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $6, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $7, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $7, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $8, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $8, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $9, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $9, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $10, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $10, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $11, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $11, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $12, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $12, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $13, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $13, %ecx, %xmm1
; X32-SSE41-NEXT:    movl %eax, %ecx
; X32-SSE41-NEXT:    shrl $14, %ecx
; X32-SSE41-NEXT:    andl $1, %ecx
; X32-SSE41-NEXT:    pinsrb $14, %ecx, %xmm1
; X32-SSE41-NEXT:    shrl $15, %eax
; X32-SSE41-NEXT:    pinsrb $15, %eax, %xmm1
; X32-SSE41-NEXT:    pmovzxbw {{.*#+}} xmm0 = xmm1[0],zero,xmm1[1],zero,xmm1[2],zero,xmm1[3],zero,xmm1[4],zero,xmm1[5],zero,xmm1[6],zero,xmm1[7],zero
; X32-SSE41-NEXT:    psllw $15, %xmm0
; X32-SSE41-NEXT:    psraw $15, %xmm0
; X32-SSE41-NEXT:    punpckhbw {{.*#+}} xmm1 = xmm1[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15]
; X32-SSE41-NEXT:    psllw $15, %xmm1
; X32-SSE41-NEXT:    psraw $15, %xmm1
; X32-SSE41-NEXT:    retl
entry:
 %X = load <16 x i1>, <16 x i1>* %ptr
 %Y = sext <16 x i1> %X to <16 x i16>
 ret <16 x i16> %Y
}

define <16 x i16> @load_sext_16i8_to_16i16(<16 x i8> *%ptr) {
; SSE2-LABEL: load_sext_16i8_to_16i16:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movq {{.*#+}} xmm0 = mem[0],zero
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    psraw $8, %xmm0
; SSE2-NEXT:    movq {{.*#+}} xmm1 = mem[0],zero
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    psraw $8, %xmm1
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_16i8_to_16i16:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movq {{.*#+}} xmm0 = mem[0],zero
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    psraw $8, %xmm0
; SSSE3-NEXT:    movq {{.*#+}} xmm1 = mem[0],zero
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    psraw $8, %xmm1
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_16i8_to_16i16:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbw (%rdi), %xmm0
; SSE41-NEXT:    pmovsxbw 8(%rdi), %xmm1
; SSE41-NEXT:    retq
;
; AVX1-LABEL: load_sext_16i8_to_16i16:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxbw (%rdi), %xmm0
; AVX1-NEXT:    vpmovsxbw 8(%rdi), %xmm1
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: load_sext_16i8_to_16i16:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovsxbw (%rdi), %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_16i8_to_16i16:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxbw (%eax), %xmm0
; X32-SSE41-NEXT:    pmovsxbw 8(%eax), %xmm1
; X32-SSE41-NEXT:    retl
entry:
 %X = load <16 x i8>, <16 x i8>* %ptr
 %Y = sext <16 x i8> %X to <16 x i16>
 ret <16 x i16> %Y
}

define <2 x i64> @load_sext_2i16_to_2i64(<2 x i16> *%ptr) {
; SSE2-LABEL: load_sext_2i16_to_2i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movd {{.*#+}} xmm0 = mem[0],zero,zero,zero
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    psrad $16, %xmm0
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_2i16_to_2i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movd {{.*#+}} xmm0 = mem[0],zero,zero,zero
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    movdqa %xmm0, %xmm1
; SSSE3-NEXT:    psrad $31, %xmm1
; SSSE3-NEXT:    psrad $16, %xmm0
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_2i16_to_2i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxwq (%rdi), %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: load_sext_2i16_to_2i64:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxwq (%rdi), %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_2i16_to_2i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxwq (%eax), %xmm0
; X32-SSE41-NEXT:    retl
entry:
 %X = load <2 x i16>, <2 x i16>* %ptr
 %Y = sext <2 x i16> %X to <2 x i64>
 ret <2 x i64> %Y
}

define <4 x i32> @load_sext_4i16_to_4i32(<4 x i16> *%ptr) {
; SSE2-LABEL: load_sext_4i16_to_4i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movq {{.*#+}} xmm0 = mem[0],zero
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $16, %xmm0
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_4i16_to_4i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movq {{.*#+}} xmm0 = mem[0],zero
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    psrad $16, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_4i16_to_4i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxwd (%rdi), %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: load_sext_4i16_to_4i32:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxwd (%rdi), %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_4i16_to_4i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxwd (%eax), %xmm0
; X32-SSE41-NEXT:    retl
entry:
 %X = load <4 x i16>, <4 x i16>* %ptr
 %Y = sext <4 x i16> %X to <4 x i32>
 ret <4 x i32> %Y
}

define <4 x i64> @load_sext_4i16_to_4i64(<4 x i16> *%ptr) {
; SSE2-LABEL: load_sext_4i16_to_4i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movswq 2(%rdi), %rax
; SSE2-NEXT:    movd %rax, %xmm1
; SSE2-NEXT:    movswq (%rdi), %rax
; SSE2-NEXT:    movd %rax, %xmm0
; SSE2-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE2-NEXT:    movswq 6(%rdi), %rax
; SSE2-NEXT:    movd %rax, %xmm2
; SSE2-NEXT:    movswq 4(%rdi), %rax
; SSE2-NEXT:    movd %rax, %xmm1
; SSE2-NEXT:    punpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_4i16_to_4i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movswq 2(%rdi), %rax
; SSSE3-NEXT:    movd %rax, %xmm1
; SSSE3-NEXT:    movswq (%rdi), %rax
; SSSE3-NEXT:    movd %rax, %xmm0
; SSSE3-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSSE3-NEXT:    movswq 6(%rdi), %rax
; SSSE3-NEXT:    movd %rax, %xmm2
; SSSE3-NEXT:    movswq 4(%rdi), %rax
; SSSE3-NEXT:    movd %rax, %xmm1
; SSSE3-NEXT:    punpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm2[0]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_4i16_to_4i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxwq (%rdi), %xmm0
; SSE41-NEXT:    pmovsxwq 4(%rdi), %xmm1
; SSE41-NEXT:    retq
;
; AVX1-LABEL: load_sext_4i16_to_4i64:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxwd (%rdi), %xmm0
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: load_sext_4i16_to_4i64:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovsxwq (%rdi), %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_4i16_to_4i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxwq (%eax), %xmm0
; X32-SSE41-NEXT:    pmovsxwq 4(%eax), %xmm1
; X32-SSE41-NEXT:    retl
entry:
 %X = load <4 x i16>, <4 x i16>* %ptr
 %Y = sext <4 x i16> %X to <4 x i64>
 ret <4 x i64> %Y
}

define <8 x i32> @load_sext_8i16_to_8i32(<8 x i16> *%ptr) {
; SSE2-LABEL: load_sext_8i16_to_8i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movq {{.*#+}} xmm0 = mem[0],zero
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $16, %xmm0
; SSE2-NEXT:    movq {{.*#+}} xmm1 = mem[0],zero
; SSE2-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3]
; SSE2-NEXT:    psrad $16, %xmm1
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_8i16_to_8i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movq {{.*#+}} xmm0 = mem[0],zero
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    psrad $16, %xmm0
; SSSE3-NEXT:    movq {{.*#+}} xmm1 = mem[0],zero
; SSSE3-NEXT:    punpcklwd {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3]
; SSSE3-NEXT:    psrad $16, %xmm1
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_8i16_to_8i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxwd (%rdi), %xmm0
; SSE41-NEXT:    pmovsxwd 8(%rdi), %xmm1
; SSE41-NEXT:    retq
;
; AVX1-LABEL: load_sext_8i16_to_8i32:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxwd (%rdi), %xmm0
; AVX1-NEXT:    vpmovsxwd 8(%rdi), %xmm1
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: load_sext_8i16_to_8i32:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovsxwd (%rdi), %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_8i16_to_8i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxwd (%eax), %xmm0
; X32-SSE41-NEXT:    pmovsxwd 8(%eax), %xmm1
; X32-SSE41-NEXT:    retl
entry:
 %X = load <8 x i16>, <8 x i16>* %ptr
 %Y = sext <8 x i16> %X to <8 x i32>
 ret <8 x i32> %Y
}

define <2 x i64> @load_sext_2i32_to_2i64(<2 x i32> *%ptr) {
; SSE2-LABEL: load_sext_2i32_to_2i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movq {{.*#+}} xmm0 = mem[0],zero
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $31, %xmm1
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_2i32_to_2i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movq {{.*#+}} xmm0 = mem[0],zero
; SSSE3-NEXT:    movdqa %xmm0, %xmm1
; SSSE3-NEXT:    psrad $31, %xmm1
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_2i32_to_2i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxdq (%rdi), %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: load_sext_2i32_to_2i64:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxdq (%rdi), %xmm0
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_2i32_to_2i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxdq (%eax), %xmm0
; X32-SSE41-NEXT:    retl
entry:
 %X = load <2 x i32>, <2 x i32>* %ptr
 %Y = sext <2 x i32> %X to <2 x i64>
 ret <2 x i64> %Y
}

define <4 x i64> @load_sext_4i32_to_4i64(<4 x i32> *%ptr) {
; SSE2-LABEL: load_sext_4i32_to_4i64:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    movdqa (%rdi), %xmm0
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: load_sext_4i32_to_4i64:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    movdqa (%rdi), %xmm0
; SSSE3-NEXT:    movdqa %xmm0, %xmm2
; SSSE3-NEXT:    psrad $31, %xmm2
; SSSE3-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSSE3-NEXT:    movdqa %xmm1, %xmm2
; SSSE3-NEXT:    psrad $31, %xmm2
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: load_sext_4i32_to_4i64:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxdq (%rdi), %xmm0
; SSE41-NEXT:    pmovsxdq 8(%rdi), %xmm1
; SSE41-NEXT:    retq
;
; AVX1-LABEL: load_sext_4i32_to_4i64:
; AVX1:       # BB#0: # %entry
; AVX1-NEXT:    vpmovsxdq (%rdi), %xmm0
; AVX1-NEXT:    vpmovsxdq 8(%rdi), %xmm1
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: load_sext_4i32_to_4i64:
; AVX2:       # BB#0: # %entry
; AVX2-NEXT:    vpmovsxdq (%rdi), %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: load_sext_4i32_to_4i64:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-SSE41-NEXT:    pmovsxdq (%eax), %xmm0
; X32-SSE41-NEXT:    pmovsxdq 8(%eax), %xmm1
; X32-SSE41-NEXT:    retl
entry:
 %X = load <4 x i32>, <4 x i32>* %ptr
 %Y = sext <4 x i32> %X to <4 x i64>
 ret <4 x i64> %Y
}

define i32 @sext_2i8_to_i32(<16 x i8> %A) nounwind uwtable readnone ssp {
; SSE2-LABEL: sext_2i8_to_i32:
; SSE2:       # BB#0: # %entry
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    psraw $8, %xmm0
; SSE2-NEXT:    movd %xmm0, %eax
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_2i8_to_i32:
; SSSE3:       # BB#0: # %entry
; SSSE3-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSSE3-NEXT:    psraw $8, %xmm0
; SSSE3-NEXT:    movd %xmm0, %eax
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_2i8_to_i32:
; SSE41:       # BB#0: # %entry
; SSE41-NEXT:    pmovsxbw %xmm0, %xmm0
; SSE41-NEXT:    movd %xmm0, %eax
; SSE41-NEXT:    retq
;
; AVX-LABEL: sext_2i8_to_i32:
; AVX:       # BB#0: # %entry
; AVX-NEXT:    vpmovsxbw %xmm0, %xmm0
; AVX-NEXT:    vmovd %xmm0, %eax
; AVX-NEXT:    retq
;
; X32-SSE41-LABEL: sext_2i8_to_i32:
; X32-SSE41:       # BB#0: # %entry
; X32-SSE41-NEXT:    pushl %eax
; X32-SSE41-NEXT:  .Ltmp0:
; X32-SSE41-NEXT:    .cfi_def_cfa_offset 8
; X32-SSE41-NEXT:    pmovsxbw %xmm0, %xmm0
; X32-SSE41-NEXT:    movd %xmm0, %eax
; X32-SSE41-NEXT:    popl %edx
; X32-SSE41-NEXT:    retl
entry:
  %Shuf = shufflevector <16 x i8> %A, <16 x i8> undef, <2 x i32> <i32 0, i32 1>
  %Ex = sext <2 x i8> %Shuf to <2 x i16>
  %Bc = bitcast <2 x i16> %Ex to i32
  ret i32 %Bc
}

define <4 x i64> @sext_4i1_to_4i64(<4 x i1> %mask) {
; SSE2-LABEL: sext_4i1_to_4i64:
; SSE2:       # BB#0:
; SSE2-NEXT:    pslld $31, %xmm0
; SSE2-NEXT:    psrad $31, %xmm0
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_4i1_to_4i64:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pslld $31, %xmm0
; SSSE3-NEXT:    psrad $31, %xmm0
; SSSE3-NEXT:    movdqa %xmm0, %xmm2
; SSSE3-NEXT:    psrad $31, %xmm2
; SSSE3-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSSE3-NEXT:    movdqa %xmm1, %xmm2
; SSSE3-NEXT:    psrad $31, %xmm2
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_4i1_to_4i64:
; SSE41:       # BB#0:
; SSE41-NEXT:    pslld $31, %xmm0
; SSE41-NEXT:    psrad $31, %xmm0
; SSE41-NEXT:    pmovsxdq %xmm0, %xmm2
; SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE41-NEXT:    pmovsxdq %xmm0, %xmm1
; SSE41-NEXT:    movdqa %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: sext_4i1_to_4i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpslld $31, %xmm0, %xmm0
; AVX1-NEXT:    vpsrad $31, %xmm0, %xmm0
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: sext_4i1_to_4i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpslld $31, %xmm0, %xmm0
; AVX2-NEXT:    vpsrad $31, %xmm0, %xmm0
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: sext_4i1_to_4i64:
; X32-SSE41:       # BB#0:
; X32-SSE41-NEXT:    pslld $31, %xmm0
; X32-SSE41-NEXT:    psrad $31, %xmm0
; X32-SSE41-NEXT:    pmovsxdq %xmm0, %xmm2
; X32-SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; X32-SSE41-NEXT:    pmovsxdq %xmm0, %xmm1
; X32-SSE41-NEXT:    movdqa %xmm2, %xmm0
; X32-SSE41-NEXT:    retl
  %extmask = sext <4 x i1> %mask to <4 x i64>
  ret <4 x i64> %extmask
}

define <4 x i64> @sext_4i8_to_4i64(<4 x i8> %mask) {
; SSE2-LABEL: sext_4i8_to_4i64:
; SSE2:       # BB#0:
; SSE2-NEXT:    pslld $24, %xmm0
; SSE2-NEXT:    psrad $24, %xmm0
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psrad $31, %xmm2
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; SSE2-NEXT:    retq
;
; SSSE3-LABEL: sext_4i8_to_4i64:
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pslld $24, %xmm0
; SSSE3-NEXT:    psrad $24, %xmm0
; SSSE3-NEXT:    movdqa %xmm0, %xmm2
; SSSE3-NEXT:    psrad $31, %xmm2
; SSSE3-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSSE3-NEXT:    movdqa %xmm1, %xmm2
; SSSE3-NEXT:    psrad $31, %xmm2
; SSSE3-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: sext_4i8_to_4i64:
; SSE41:       # BB#0:
; SSE41-NEXT:    pslld $24, %xmm0
; SSE41-NEXT:    psrad $24, %xmm0
; SSE41-NEXT:    pmovsxdq %xmm0, %xmm2
; SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE41-NEXT:    pmovsxdq %xmm0, %xmm1
; SSE41-NEXT:    movdqa %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: sext_4i8_to_4i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpslld $24, %xmm0, %xmm0
; AVX1-NEXT:    vpsrad $24, %xmm0, %xmm0
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm1
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; AVX1-NEXT:    vpmovsxdq %xmm0, %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: sext_4i8_to_4i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpslld $24, %xmm0, %xmm0
; AVX2-NEXT:    vpsrad $24, %xmm0, %xmm0
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    retq
;
; X32-SSE41-LABEL: sext_4i8_to_4i64:
; X32-SSE41:       # BB#0:
; X32-SSE41-NEXT:    pslld $24, %xmm0
; X32-SSE41-NEXT:    psrad $24, %xmm0
; X32-SSE41-NEXT:    pmovsxdq %xmm0, %xmm2
; X32-SSE41-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; X32-SSE41-NEXT:    pmovsxdq %xmm0, %xmm1
; X32-SSE41-NEXT:    movdqa %xmm2, %xmm0
; X32-SSE41-NEXT:    retl
  %extmask = sext <4 x i8> %mask to <4 x i64>
  ret <4 x i64> %extmask
}
