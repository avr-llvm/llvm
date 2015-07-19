; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 | FileCheck %s --check-prefix=ALL --check-prefix=SSE --check-prefix=SSE2
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+sse4.1 | FileCheck %s --check-prefix=ALL --check-prefix=SSE --check-prefix=SSE41
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+avx | FileCheck %s --check-prefix=ALL --check-prefix=AVX --check-prefix=AVX1
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+avx2 | FileCheck %s --check-prefix=ALL --check-prefix=AVX --check-prefix=AVX2
;
; Just one 32-bit run to make sure we do reasonable things for i64 shifts.
; RUN: llc < %s -mtriple=i686-unknown-unknown -mcpu=x86-64 | FileCheck %s --check-prefix=ALL --check-prefix=X32-SSE --check-prefix=X32-SSE2

;
; Variable Shifts
;

define <2 x i64> @var_shift_v2i64(<2 x i64> %a, <2 x i64> %b) {
; SSE2-LABEL: var_shift_v2i64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    movd %xmm1, %rcx
; SSE2-NEXT:    sarq %cl, %rax
; SSE2-NEXT:    movd %rax, %xmm2
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm1[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rcx
; SSE2-NEXT:    sarq %cl, %rax
; SSE2-NEXT:    movd %rax, %xmm0
; SSE2-NEXT:    punpcklqdq {{.*#+}} xmm2 = xmm2[0],xmm0[0]
; SSE2-NEXT:    movdqa %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; SSE41-LABEL: var_shift_v2i64:
; SSE41:       # BB#0:
; SSE41-NEXT:    pextrq $1, %xmm0, %rax
; SSE41-NEXT:    pextrq $1, %xmm1, %rcx
; SSE41-NEXT:    sarq %cl, %rax
; SSE41-NEXT:    movd %rax, %xmm2
; SSE41-NEXT:    movd %xmm0, %rax
; SSE41-NEXT:    movd %xmm1, %rcx
; SSE41-NEXT:    sarq %cl, %rax
; SSE41-NEXT:    movd %rax, %xmm0
; SSE41-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; SSE41-NEXT:    retq
;
; AVX-LABEL: var_shift_v2i64:
; AVX:       # BB#0:
; AVX-NEXT:    vpextrq $1, %xmm0, %rax
; AVX-NEXT:    vpextrq $1, %xmm1, %rcx
; AVX-NEXT:    sarq %cl, %rax
; AVX-NEXT:    vmovq %rax, %xmm2
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    vmovq %xmm1, %rcx
; AVX-NEXT:    sarq %cl, %rax
; AVX-NEXT:    vmovq %rax, %xmm0
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; AVX-NEXT:    retq
;
; X32-SSE-LABEL: var_shift_v2i64:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    pushl %ebp
; X32-SSE-NEXT:  .Ltmp0:
; X32-SSE-NEXT:    .cfi_def_cfa_offset 8
; X32-SSE-NEXT:    pushl %ebx
; X32-SSE-NEXT:  .Ltmp1:
; X32-SSE-NEXT:    .cfi_def_cfa_offset 12
; X32-SSE-NEXT:    pushl %edi
; X32-SSE-NEXT:  .Ltmp2:
; X32-SSE-NEXT:    .cfi_def_cfa_offset 16
; X32-SSE-NEXT:    pushl %esi
; X32-SSE-NEXT:  .Ltmp3:
; X32-SSE-NEXT:    .cfi_def_cfa_offset 20
; X32-SSE-NEXT:  .Ltmp4:
; X32-SSE-NEXT:    .cfi_offset %esi, -20
; X32-SSE-NEXT:  .Ltmp5:
; X32-SSE-NEXT:    .cfi_offset %edi, -16
; X32-SSE-NEXT:  .Ltmp6:
; X32-SSE-NEXT:    .cfi_offset %ebx, -12
; X32-SSE-NEXT:  .Ltmp7:
; X32-SSE-NEXT:    .cfi_offset %ebp, -8
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[3,1,2,3]
; X32-SSE-NEXT:    movd %xmm2, %edx
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[2,3,0,1]
; X32-SSE-NEXT:    movd %xmm2, %esi
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm2 = xmm1[2,3,0,1]
; X32-SSE-NEXT:    movd %xmm2, %eax
; X32-SSE-NEXT:    movb %al, %cl
; X32-SSE-NEXT:    shrdl %cl, %edx, %esi
; X32-SSE-NEXT:    movd %xmm0, %edi
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; X32-SSE-NEXT:    movd %xmm0, %ebx
; X32-SSE-NEXT:    movd %xmm1, %ecx
; X32-SSE-NEXT:    shrdl %cl, %ebx, %edi
; X32-SSE-NEXT:    movl %ebx, %ebp
; X32-SSE-NEXT:    sarl %cl, %ebp
; X32-SSE-NEXT:    sarl $31, %ebx
; X32-SSE-NEXT:    testb $32, %cl
; X32-SSE-NEXT:    cmovnel %ebp, %edi
; X32-SSE-NEXT:    movd %edi, %xmm0
; X32-SSE-NEXT:    cmovel %ebp, %ebx
; X32-SSE-NEXT:    movl %edx, %edi
; X32-SSE-NEXT:    movb %al, %cl
; X32-SSE-NEXT:    sarl %cl, %edi
; X32-SSE-NEXT:    sarl $31, %edx
; X32-SSE-NEXT:    testb $32, %al
; X32-SSE-NEXT:    cmovnel %edi, %esi
; X32-SSE-NEXT:    movd %esi, %xmm1
; X32-SSE-NEXT:    movd %ebx, %xmm2
; X32-SSE-NEXT:    cmovel %edi, %edx
; X32-SSE-NEXT:    movd %edx, %xmm3
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm3[0],xmm2[1],xmm3[1]
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; X32-SSE-NEXT:    popl %esi
; X32-SSE-NEXT:    popl %edi
; X32-SSE-NEXT:    popl %ebx
; X32-SSE-NEXT:    popl %ebp
; X32-SSE-NEXT:    retl
  %shift = ashr <2 x i64> %a, %b
  ret <2 x i64> %shift
}

define <4 x i32> @var_shift_v4i32(<4 x i32> %a, <4 x i32> %b) {
; SSE2-LABEL: var_shift_v4i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psrldq {{.*#+}} xmm2 = xmm2[12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero
; SSE2-NEXT:    movdqa %xmm0, %xmm3
; SSE2-NEXT:    psrad %xmm2, %xmm3
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psrlq $32, %xmm2
; SSE2-NEXT:    movdqa %xmm0, %xmm4
; SSE2-NEXT:    psrad %xmm2, %xmm4
; SSE2-NEXT:    movsd {{.*#+}} xmm3 = xmm4[0],xmm3[1]
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm3[1,3,2,3]
; SSE2-NEXT:    pxor %xmm3, %xmm3
; SSE2-NEXT:    movdqa %xmm1, %xmm4
; SSE2-NEXT:    punpckhdq {{.*#+}} xmm4 = xmm4[2],xmm3[2],xmm4[3],xmm3[3]
; SSE2-NEXT:    movdqa %xmm0, %xmm5
; SSE2-NEXT:    psrad %xmm4, %xmm5
; SSE2-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm3[0],xmm1[1],xmm3[1]
; SSE2-NEXT:    psrad %xmm1, %xmm0
; SSE2-NEXT:    movsd {{.*#+}} xmm5 = xmm0[0],xmm5[1]
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm5[0,2,2,3]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; SSE2-NEXT:    retq
;
; SSE41-LABEL: var_shift_v4i32:
; SSE41:       # BB#0:
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psrldq {{.*#+}} xmm2 = xmm2[12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero
; SSE41-NEXT:    movdqa %xmm0, %xmm3
; SSE41-NEXT:    psrad %xmm2, %xmm3
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psrlq $32, %xmm2
; SSE41-NEXT:    movdqa %xmm0, %xmm4
; SSE41-NEXT:    psrad %xmm2, %xmm4
; SSE41-NEXT:    pblendw {{.*#+}} xmm4 = xmm4[0,1,2,3],xmm3[4,5,6,7]
; SSE41-NEXT:    pxor %xmm2, %xmm2
; SSE41-NEXT:    pmovzxdq {{.*#+}} xmm3 = xmm1[0],zero,xmm1[1],zero
; SSE41-NEXT:    punpckhdq {{.*#+}} xmm1 = xmm1[2],xmm2[2],xmm1[3],xmm2[3]
; SSE41-NEXT:    movdqa %xmm0, %xmm2
; SSE41-NEXT:    psrad %xmm1, %xmm2
; SSE41-NEXT:    psrad %xmm3, %xmm0
; SSE41-NEXT:    pblendw {{.*#+}} xmm0 = xmm0[0,1,2,3],xmm2[4,5,6,7]
; SSE41-NEXT:    pblendw {{.*#+}} xmm0 = xmm0[0,1],xmm4[2,3],xmm0[4,5],xmm4[6,7]
; SSE41-NEXT:    retq
;
; AVX1-LABEL: var_shift_v4i32:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpsrldq {{.*#+}} xmm2 = xmm1[12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero
; AVX1-NEXT:    vpsrad %xmm2, %xmm0, %xmm2
; AVX1-NEXT:    vpsrlq $32, %xmm1, %xmm3
; AVX1-NEXT:    vpsrad %xmm3, %xmm0, %xmm3
; AVX1-NEXT:    vpblendw {{.*#+}} xmm2 = xmm3[0,1,2,3],xmm2[4,5,6,7]
; AVX1-NEXT:    vpxor %xmm3, %xmm3, %xmm3
; AVX1-NEXT:    vpunpckhdq {{.*#+}} xmm3 = xmm1[2],xmm3[2],xmm1[3],xmm3[3]
; AVX1-NEXT:    vpsrad %xmm3, %xmm0, %xmm3
; AVX1-NEXT:    vpmovzxdq {{.*#+}} xmm1 = xmm1[0],zero,xmm1[1],zero
; AVX1-NEXT:    vpsrad %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1,2,3],xmm3[4,5,6,7]
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1],xmm2[2,3],xmm0[4,5],xmm2[6,7]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: var_shift_v4i32:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsravd %xmm1, %xmm0, %xmm0
; AVX2-NEXT:    retq
;
; X32-SSE-LABEL: var_shift_v4i32:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    movdqa %xmm1, %xmm2
; X32-SSE-NEXT:    psrldq {{.*#+}} xmm2 = xmm2[12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero,zero
; X32-SSE-NEXT:    movdqa %xmm0, %xmm3
; X32-SSE-NEXT:    psrad %xmm2, %xmm3
; X32-SSE-NEXT:    movdqa %xmm1, %xmm2
; X32-SSE-NEXT:    psrlq $32, %xmm2
; X32-SSE-NEXT:    movdqa %xmm0, %xmm4
; X32-SSE-NEXT:    psrad %xmm2, %xmm4
; X32-SSE-NEXT:    movsd {{.*#+}} xmm3 = xmm4[0],xmm3[1]
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm2 = xmm3[1,3,2,3]
; X32-SSE-NEXT:    pxor %xmm3, %xmm3
; X32-SSE-NEXT:    movdqa %xmm1, %xmm4
; X32-SSE-NEXT:    punpckhdq {{.*#+}} xmm4 = xmm4[2],xmm3[2],xmm4[3],xmm3[3]
; X32-SSE-NEXT:    movdqa %xmm0, %xmm5
; X32-SSE-NEXT:    psrad %xmm4, %xmm5
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm1 = xmm1[0],xmm3[0],xmm1[1],xmm3[1]
; X32-SSE-NEXT:    psrad %xmm1, %xmm0
; X32-SSE-NEXT:    movsd {{.*#+}} xmm5 = xmm0[0],xmm5[1]
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm0 = xmm5[0,2,2,3]
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; X32-SSE-NEXT:    retl
  %shift = ashr <4 x i32> %a, %b
  ret <4 x i32> %shift
}

define <8 x i16> @var_shift_v8i16(<8 x i16> %a, <8 x i16> %b) {
; SSE2-LABEL: var_shift_v8i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    psllw $12, %xmm1
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psraw $15, %xmm2
; SSE2-NEXT:    movdqa %xmm2, %xmm3
; SSE2-NEXT:    pandn %xmm0, %xmm3
; SSE2-NEXT:    psraw $8, %xmm0
; SSE2-NEXT:    pand %xmm2, %xmm0
; SSE2-NEXT:    por %xmm3, %xmm0
; SSE2-NEXT:    paddw %xmm1, %xmm1
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psraw $15, %xmm2
; SSE2-NEXT:    movdqa %xmm2, %xmm3
; SSE2-NEXT:    pandn %xmm0, %xmm3
; SSE2-NEXT:    psraw $4, %xmm0
; SSE2-NEXT:    pand %xmm2, %xmm0
; SSE2-NEXT:    por %xmm3, %xmm0
; SSE2-NEXT:    paddw %xmm1, %xmm1
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    psraw $15, %xmm2
; SSE2-NEXT:    movdqa %xmm2, %xmm3
; SSE2-NEXT:    pandn %xmm0, %xmm3
; SSE2-NEXT:    psraw $2, %xmm0
; SSE2-NEXT:    pand %xmm2, %xmm0
; SSE2-NEXT:    por %xmm3, %xmm0
; SSE2-NEXT:    paddw %xmm1, %xmm1
; SSE2-NEXT:    psraw $15, %xmm1
; SSE2-NEXT:    movdqa %xmm1, %xmm2
; SSE2-NEXT:    pandn %xmm0, %xmm2
; SSE2-NEXT:    psraw $1, %xmm0
; SSE2-NEXT:    pand %xmm1, %xmm0
; SSE2-NEXT:    por %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; SSE41-LABEL: var_shift_v8i16:
; SSE41:       # BB#0:
; SSE41-NEXT:    movdqa %xmm0, %xmm2
; SSE41-NEXT:    movdqa %xmm1, %xmm0
; SSE41-NEXT:    psllw $12, %xmm0
; SSE41-NEXT:    psllw $4, %xmm1
; SSE41-NEXT:    por %xmm0, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm3
; SSE41-NEXT:    paddw %xmm3, %xmm3
; SSE41-NEXT:    movdqa %xmm2, %xmm4
; SSE41-NEXT:    psraw $8, %xmm4
; SSE41-NEXT:    movdqa %xmm1, %xmm0
; SSE41-NEXT:    pblendvb %xmm4, %xmm2
; SSE41-NEXT:    movdqa %xmm2, %xmm1
; SSE41-NEXT:    psraw $4, %xmm1
; SSE41-NEXT:    movdqa %xmm3, %xmm0
; SSE41-NEXT:    pblendvb %xmm1, %xmm2
; SSE41-NEXT:    movdqa %xmm2, %xmm1
; SSE41-NEXT:    psraw $2, %xmm1
; SSE41-NEXT:    paddw %xmm3, %xmm3
; SSE41-NEXT:    movdqa %xmm3, %xmm0
; SSE41-NEXT:    pblendvb %xmm1, %xmm2
; SSE41-NEXT:    movdqa %xmm2, %xmm1
; SSE41-NEXT:    psraw $1, %xmm1
; SSE41-NEXT:    paddw %xmm3, %xmm3
; SSE41-NEXT:    movdqa %xmm3, %xmm0
; SSE41-NEXT:    pblendvb %xmm1, %xmm2
; SSE41-NEXT:    movdqa %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: var_shift_v8i16:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpsllw $12, %xmm1, %xmm2
; AVX1-NEXT:    vpsllw $4, %xmm1, %xmm1
; AVX1-NEXT:    vpor %xmm2, %xmm1, %xmm1
; AVX1-NEXT:    vpaddw %xmm1, %xmm1, %xmm2
; AVX1-NEXT:    vpsraw $8, %xmm0, %xmm3
; AVX1-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $4, %xmm0, %xmm1
; AVX1-NEXT:    vpblendvb %xmm2, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $2, %xmm0, %xmm1
; AVX1-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpblendvb %xmm2, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $1, %xmm0, %xmm1
; AVX1-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpblendvb %xmm2, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: var_shift_v8i16:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpmovzxwd {{.*#+}} ymm1 = xmm1[0],zero,xmm1[1],zero,xmm1[2],zero,xmm1[3],zero,xmm1[4],zero,xmm1[5],zero,xmm1[6],zero,xmm1[7],zero
; AVX2-NEXT:    vpmovsxwd %xmm0, %ymm0
; AVX2-NEXT:    vpsravd %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    vpshufb {{.*#+}} ymm0 = ymm0[0,1,4,5,8,9,12,13],zero,zero,zero,zero,zero,zero,zero,zero,ymm0[16,17,20,21,24,25,28,29],zero,zero,zero,zero,zero,zero,zero,zero
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,2,2,3]
; AVX2-NEXT:    vzeroupper
; AVX2-NEXT:    retq
;
; X32-SSE-LABEL: var_shift_v8i16:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    psllw $12, %xmm1
; X32-SSE-NEXT:    movdqa %xmm1, %xmm2
; X32-SSE-NEXT:    psraw $15, %xmm2
; X32-SSE-NEXT:    movdqa %xmm2, %xmm3
; X32-SSE-NEXT:    pandn %xmm0, %xmm3
; X32-SSE-NEXT:    psraw $8, %xmm0
; X32-SSE-NEXT:    pand %xmm2, %xmm0
; X32-SSE-NEXT:    por %xmm3, %xmm0
; X32-SSE-NEXT:    paddw %xmm1, %xmm1
; X32-SSE-NEXT:    movdqa %xmm1, %xmm2
; X32-SSE-NEXT:    psraw $15, %xmm2
; X32-SSE-NEXT:    movdqa %xmm2, %xmm3
; X32-SSE-NEXT:    pandn %xmm0, %xmm3
; X32-SSE-NEXT:    psraw $4, %xmm0
; X32-SSE-NEXT:    pand %xmm2, %xmm0
; X32-SSE-NEXT:    por %xmm3, %xmm0
; X32-SSE-NEXT:    paddw %xmm1, %xmm1
; X32-SSE-NEXT:    movdqa %xmm1, %xmm2
; X32-SSE-NEXT:    psraw $15, %xmm2
; X32-SSE-NEXT:    movdqa %xmm2, %xmm3
; X32-SSE-NEXT:    pandn %xmm0, %xmm3
; X32-SSE-NEXT:    psraw $2, %xmm0
; X32-SSE-NEXT:    pand %xmm2, %xmm0
; X32-SSE-NEXT:    por %xmm3, %xmm0
; X32-SSE-NEXT:    paddw %xmm1, %xmm1
; X32-SSE-NEXT:    psraw $15, %xmm1
; X32-SSE-NEXT:    movdqa %xmm1, %xmm2
; X32-SSE-NEXT:    pandn %xmm0, %xmm2
; X32-SSE-NEXT:    psraw $1, %xmm0
; X32-SSE-NEXT:    pand %xmm1, %xmm0
; X32-SSE-NEXT:    por %xmm2, %xmm0
; X32-SSE-NEXT:    retl
  %shift = ashr <8 x i16> %a, %b
  ret <8 x i16> %shift
}

define <16 x i8> @var_shift_v16i8(<16 x i8> %a, <16 x i8> %b) {
; SSE2-LABEL: var_shift_v16i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    punpckhbw {{.*#+}} xmm2 = xmm2[8],xmm0[8],xmm2[9],xmm0[9],xmm2[10],xmm0[10],xmm2[11],xmm0[11],xmm2[12],xmm0[12],xmm2[13],xmm0[13],xmm2[14],xmm0[14],xmm2[15],xmm0[15]
; SSE2-NEXT:    psllw $5, %xmm1
; SSE2-NEXT:    punpckhbw {{.*#+}} xmm4 = xmm4[8],xmm1[8],xmm4[9],xmm1[9],xmm4[10],xmm1[10],xmm4[11],xmm1[11],xmm4[12],xmm1[12],xmm4[13],xmm1[13],xmm4[14],xmm1[14],xmm4[15],xmm1[15]
; SSE2-NEXT:    pxor %xmm3, %xmm3
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtw %xmm4, %xmm5
; SSE2-NEXT:    movdqa %xmm5, %xmm6
; SSE2-NEXT:    pandn %xmm2, %xmm6
; SSE2-NEXT:    psraw $4, %xmm2
; SSE2-NEXT:    pand %xmm5, %xmm2
; SSE2-NEXT:    por %xmm6, %xmm2
; SSE2-NEXT:    paddw %xmm4, %xmm4
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtw %xmm4, %xmm5
; SSE2-NEXT:    movdqa %xmm5, %xmm6
; SSE2-NEXT:    pandn %xmm2, %xmm6
; SSE2-NEXT:    psraw $2, %xmm2
; SSE2-NEXT:    pand %xmm5, %xmm2
; SSE2-NEXT:    por %xmm6, %xmm2
; SSE2-NEXT:    paddw %xmm4, %xmm4
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtw %xmm4, %xmm5
; SSE2-NEXT:    movdqa %xmm5, %xmm4
; SSE2-NEXT:    pandn %xmm2, %xmm4
; SSE2-NEXT:    psraw $1, %xmm2
; SSE2-NEXT:    pand %xmm5, %xmm2
; SSE2-NEXT:    por %xmm4, %xmm2
; SSE2-NEXT:    psrlw $8, %xmm2
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    pxor %xmm4, %xmm4
; SSE2-NEXT:    pcmpgtw %xmm1, %xmm4
; SSE2-NEXT:    movdqa %xmm4, %xmm5
; SSE2-NEXT:    pandn %xmm0, %xmm5
; SSE2-NEXT:    psraw $4, %xmm0
; SSE2-NEXT:    pand %xmm4, %xmm0
; SSE2-NEXT:    por %xmm5, %xmm0
; SSE2-NEXT:    paddw %xmm1, %xmm1
; SSE2-NEXT:    pxor %xmm4, %xmm4
; SSE2-NEXT:    pcmpgtw %xmm1, %xmm4
; SSE2-NEXT:    movdqa %xmm4, %xmm5
; SSE2-NEXT:    pandn %xmm0, %xmm5
; SSE2-NEXT:    psraw $2, %xmm0
; SSE2-NEXT:    pand %xmm4, %xmm0
; SSE2-NEXT:    por %xmm5, %xmm0
; SSE2-NEXT:    paddw %xmm1, %xmm1
; SSE2-NEXT:    pcmpgtw %xmm1, %xmm3
; SSE2-NEXT:    movdqa %xmm3, %xmm1
; SSE2-NEXT:    pandn %xmm0, %xmm1
; SSE2-NEXT:    psraw $1, %xmm0
; SSE2-NEXT:    pand %xmm3, %xmm0
; SSE2-NEXT:    por %xmm1, %xmm0
; SSE2-NEXT:    psrlw $8, %xmm0
; SSE2-NEXT:    packuswb %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; SSE41-LABEL: var_shift_v16i8:
; SSE41:       # BB#0:
; SSE41-NEXT:    movdqa %xmm0, %xmm2
; SSE41-NEXT:    psllw $5, %xmm1
; SSE41-NEXT:    punpckhbw {{.*#+}} xmm0 = xmm0[8],xmm1[8],xmm0[9],xmm1[9],xmm0[10],xmm1[10],xmm0[11],xmm1[11],xmm0[12],xmm1[12],xmm0[13],xmm1[13],xmm0[14],xmm1[14],xmm0[15],xmm1[15]
; SSE41-NEXT:    punpckhbw {{.*#+}} xmm3 = xmm3[8],xmm2[8],xmm3[9],xmm2[9],xmm3[10],xmm2[10],xmm3[11],xmm2[11],xmm3[12],xmm2[12],xmm3[13],xmm2[13],xmm3[14],xmm2[14],xmm3[15],xmm2[15]
; SSE41-NEXT:    movdqa %xmm3, %xmm4
; SSE41-NEXT:    psraw $4, %xmm4
; SSE41-NEXT:    pblendvb %xmm4, %xmm3
; SSE41-NEXT:    movdqa %xmm3, %xmm4
; SSE41-NEXT:    psraw $2, %xmm4
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm4, %xmm3
; SSE41-NEXT:    movdqa %xmm3, %xmm4
; SSE41-NEXT:    psraw $1, %xmm4
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm4, %xmm3
; SSE41-NEXT:    psrlw $8, %xmm3
; SSE41-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; SSE41-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3],xmm1[4],xmm2[4],xmm1[5],xmm2[5],xmm1[6],xmm2[6],xmm1[7],xmm2[7]
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psraw $4, %xmm2
; SSE41-NEXT:    pblendvb %xmm2, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psraw $2, %xmm2
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm2, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psraw $1, %xmm2
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm2, %xmm1
; SSE41-NEXT:    psrlw $8, %xmm1
; SSE41-NEXT:    packuswb %xmm3, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: var_shift_v16i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpsllw $5, %xmm1, %xmm1
; AVX-NEXT:    vpunpckhbw {{.*#+}} xmm2 = xmm0[8],xmm1[8],xmm0[9],xmm1[9],xmm0[10],xmm1[10],xmm0[11],xmm1[11],xmm0[12],xmm1[12],xmm0[13],xmm1[13],xmm0[14],xmm1[14],xmm0[15],xmm1[15]
; AVX-NEXT:    vpunpckhbw {{.*#+}} xmm3 = xmm0[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15]
; AVX-NEXT:    vpsraw $4, %xmm3, %xmm4
; AVX-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm3
; AVX-NEXT:    vpsraw $2, %xmm3, %xmm4
; AVX-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm3
; AVX-NEXT:    vpsraw $1, %xmm3, %xmm4
; AVX-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm2
; AVX-NEXT:    vpsrlw $8, %xmm2, %xmm2
; AVX-NEXT:    vpunpcklbw {{.*#+}} xmm1 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; AVX-NEXT:    vpunpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; AVX-NEXT:    vpsraw $4, %xmm0, %xmm3
; AVX-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX-NEXT:    vpsraw $2, %xmm0, %xmm3
; AVX-NEXT:    vpaddw %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX-NEXT:    vpsraw $1, %xmm0, %xmm3
; AVX-NEXT:    vpaddw %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX-NEXT:    vpsrlw $8, %xmm0, %xmm0
; AVX-NEXT:    vpackuswb %xmm2, %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE-LABEL: var_shift_v16i8:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    punpckhbw {{.*#+}} xmm2 = xmm2[8],xmm0[8],xmm2[9],xmm0[9],xmm2[10],xmm0[10],xmm2[11],xmm0[11],xmm2[12],xmm0[12],xmm2[13],xmm0[13],xmm2[14],xmm0[14],xmm2[15],xmm0[15]
; X32-SSE-NEXT:    psllw $5, %xmm1
; X32-SSE-NEXT:    punpckhbw {{.*#+}} xmm4 = xmm4[8],xmm1[8],xmm4[9],xmm1[9],xmm4[10],xmm1[10],xmm4[11],xmm1[11],xmm4[12],xmm1[12],xmm4[13],xmm1[13],xmm4[14],xmm1[14],xmm4[15],xmm1[15]
; X32-SSE-NEXT:    pxor %xmm3, %xmm3
; X32-SSE-NEXT:    pxor %xmm5, %xmm5
; X32-SSE-NEXT:    pcmpgtw %xmm4, %xmm5
; X32-SSE-NEXT:    movdqa %xmm5, %xmm6
; X32-SSE-NEXT:    pandn %xmm2, %xmm6
; X32-SSE-NEXT:    psraw $4, %xmm2
; X32-SSE-NEXT:    pand %xmm5, %xmm2
; X32-SSE-NEXT:    por %xmm6, %xmm2
; X32-SSE-NEXT:    paddw %xmm4, %xmm4
; X32-SSE-NEXT:    pxor %xmm5, %xmm5
; X32-SSE-NEXT:    pcmpgtw %xmm4, %xmm5
; X32-SSE-NEXT:    movdqa %xmm5, %xmm6
; X32-SSE-NEXT:    pandn %xmm2, %xmm6
; X32-SSE-NEXT:    psraw $2, %xmm2
; X32-SSE-NEXT:    pand %xmm5, %xmm2
; X32-SSE-NEXT:    por %xmm6, %xmm2
; X32-SSE-NEXT:    paddw %xmm4, %xmm4
; X32-SSE-NEXT:    pxor %xmm5, %xmm5
; X32-SSE-NEXT:    pcmpgtw %xmm4, %xmm5
; X32-SSE-NEXT:    movdqa %xmm5, %xmm4
; X32-SSE-NEXT:    pandn %xmm2, %xmm4
; X32-SSE-NEXT:    psraw $1, %xmm2
; X32-SSE-NEXT:    pand %xmm5, %xmm2
; X32-SSE-NEXT:    por %xmm4, %xmm2
; X32-SSE-NEXT:    psrlw $8, %xmm2
; X32-SSE-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; X32-SSE-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; X32-SSE-NEXT:    pxor %xmm4, %xmm4
; X32-SSE-NEXT:    pcmpgtw %xmm1, %xmm4
; X32-SSE-NEXT:    movdqa %xmm4, %xmm5
; X32-SSE-NEXT:    pandn %xmm0, %xmm5
; X32-SSE-NEXT:    psraw $4, %xmm0
; X32-SSE-NEXT:    pand %xmm4, %xmm0
; X32-SSE-NEXT:    por %xmm5, %xmm0
; X32-SSE-NEXT:    paddw %xmm1, %xmm1
; X32-SSE-NEXT:    pxor %xmm4, %xmm4
; X32-SSE-NEXT:    pcmpgtw %xmm1, %xmm4
; X32-SSE-NEXT:    movdqa %xmm4, %xmm5
; X32-SSE-NEXT:    pandn %xmm0, %xmm5
; X32-SSE-NEXT:    psraw $2, %xmm0
; X32-SSE-NEXT:    pand %xmm4, %xmm0
; X32-SSE-NEXT:    por %xmm5, %xmm0
; X32-SSE-NEXT:    paddw %xmm1, %xmm1
; X32-SSE-NEXT:    pcmpgtw %xmm1, %xmm3
; X32-SSE-NEXT:    movdqa %xmm3, %xmm1
; X32-SSE-NEXT:    pandn %xmm0, %xmm1
; X32-SSE-NEXT:    psraw $1, %xmm0
; X32-SSE-NEXT:    pand %xmm3, %xmm0
; X32-SSE-NEXT:    por %xmm1, %xmm0
; X32-SSE-NEXT:    psrlw $8, %xmm0
; X32-SSE-NEXT:    packuswb %xmm2, %xmm0
; X32-SSE-NEXT:    retl
  %shift = ashr <16 x i8> %a, %b
  ret <16 x i8> %shift
}

;
; Uniform Variable Shifts
;

define <2 x i64> @splatvar_shift_v2i64(<2 x i64> %a, <2 x i64> %b) {
; SSE2-LABEL: splatvar_shift_v2i64:
; SSE2:       # BB#0:
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm1[0,1,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    movd %xmm2, %rcx
; SSE2-NEXT:    sarq %cl, %rax
; SSE2-NEXT:    movd %rax, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm2[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rcx
; SSE2-NEXT:    sarq %cl, %rax
; SSE2-NEXT:    movd %rax, %xmm0
; SSE2-NEXT:    punpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm0[0]
; SSE2-NEXT:    movdqa %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSE41-LABEL: splatvar_shift_v2i64:
; SSE41:       # BB#0:
; SSE41-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[0,1,0,1]
; SSE41-NEXT:    pextrq $1, %xmm0, %rax
; SSE41-NEXT:    pextrq $1, %xmm1, %rcx
; SSE41-NEXT:    sarq %cl, %rax
; SSE41-NEXT:    movd %rax, %xmm2
; SSE41-NEXT:    movd %xmm0, %rax
; SSE41-NEXT:    movd %xmm1, %rcx
; SSE41-NEXT:    sarq %cl, %rax
; SSE41-NEXT:    movd %rax, %xmm0
; SSE41-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; SSE41-NEXT:    retq
;
; AVX1-LABEL: splatvar_shift_v2i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpshufd {{.*#+}} xmm1 = xmm1[0,1,0,1]
; AVX1-NEXT:    vpextrq $1, %xmm0, %rax
; AVX1-NEXT:    vpextrq $1, %xmm1, %rcx
; AVX1-NEXT:    sarq %cl, %rax
; AVX1-NEXT:    vmovq %rax, %xmm2
; AVX1-NEXT:    vmovq %xmm0, %rax
; AVX1-NEXT:    vmovq %xmm1, %rcx
; AVX1-NEXT:    sarq %cl, %rax
; AVX1-NEXT:    vmovq %rax, %xmm0
; AVX1-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatvar_shift_v2i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpbroadcastq %xmm1, %xmm1
; AVX2-NEXT:    vpextrq $1, %xmm0, %rax
; AVX2-NEXT:    vpextrq $1, %xmm1, %rcx
; AVX2-NEXT:    sarq %cl, %rax
; AVX2-NEXT:    vmovq %rax, %xmm2
; AVX2-NEXT:    vmovq %xmm0, %rax
; AVX2-NEXT:    vmovq %xmm1, %rcx
; AVX2-NEXT:    sarq %cl, %rax
; AVX2-NEXT:    vmovq %rax, %xmm0
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm2[0]
; AVX2-NEXT:    retq
;
; X32-SSE-LABEL: splatvar_shift_v2i64:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    pushl %ebp
; X32-SSE-NEXT:  .Ltmp8:
; X32-SSE-NEXT:    .cfi_def_cfa_offset 8
; X32-SSE-NEXT:    pushl %ebx
; X32-SSE-NEXT:  .Ltmp9:
; X32-SSE-NEXT:    .cfi_def_cfa_offset 12
; X32-SSE-NEXT:    pushl %edi
; X32-SSE-NEXT:  .Ltmp10:
; X32-SSE-NEXT:    .cfi_def_cfa_offset 16
; X32-SSE-NEXT:    pushl %esi
; X32-SSE-NEXT:  .Ltmp11:
; X32-SSE-NEXT:    .cfi_def_cfa_offset 20
; X32-SSE-NEXT:  .Ltmp12:
; X32-SSE-NEXT:    .cfi_offset %esi, -20
; X32-SSE-NEXT:  .Ltmp13:
; X32-SSE-NEXT:    .cfi_offset %edi, -16
; X32-SSE-NEXT:  .Ltmp14:
; X32-SSE-NEXT:    .cfi_offset %ebx, -12
; X32-SSE-NEXT:  .Ltmp15:
; X32-SSE-NEXT:    .cfi_offset %ebp, -8
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[0,1,0,1]
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[3,1,2,3]
; X32-SSE-NEXT:    movd %xmm2, %edx
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm2 = xmm0[2,3,0,1]
; X32-SSE-NEXT:    movd %xmm2, %esi
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm2 = xmm1[2,3,0,1]
; X32-SSE-NEXT:    movd %xmm2, %eax
; X32-SSE-NEXT:    movb %al, %cl
; X32-SSE-NEXT:    shrdl %cl, %edx, %esi
; X32-SSE-NEXT:    movd %xmm0, %edi
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; X32-SSE-NEXT:    movd %xmm0, %ebx
; X32-SSE-NEXT:    movd %xmm1, %ecx
; X32-SSE-NEXT:    shrdl %cl, %ebx, %edi
; X32-SSE-NEXT:    movl %ebx, %ebp
; X32-SSE-NEXT:    sarl %cl, %ebp
; X32-SSE-NEXT:    sarl $31, %ebx
; X32-SSE-NEXT:    testb $32, %cl
; X32-SSE-NEXT:    cmovnel %ebp, %edi
; X32-SSE-NEXT:    movd %edi, %xmm0
; X32-SSE-NEXT:    cmovel %ebp, %ebx
; X32-SSE-NEXT:    movl %edx, %edi
; X32-SSE-NEXT:    movb %al, %cl
; X32-SSE-NEXT:    sarl %cl, %edi
; X32-SSE-NEXT:    sarl $31, %edx
; X32-SSE-NEXT:    testb $32, %al
; X32-SSE-NEXT:    cmovnel %edi, %esi
; X32-SSE-NEXT:    movd %esi, %xmm1
; X32-SSE-NEXT:    movd %ebx, %xmm2
; X32-SSE-NEXT:    cmovel %edi, %edx
; X32-SSE-NEXT:    movd %edx, %xmm3
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm3[0],xmm2[1],xmm3[1]
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; X32-SSE-NEXT:    popl %esi
; X32-SSE-NEXT:    popl %edi
; X32-SSE-NEXT:    popl %ebx
; X32-SSE-NEXT:    popl %ebp
; X32-SSE-NEXT:    retl
  %splat = shufflevector <2 x i64> %b, <2 x i64> undef, <2 x i32> zeroinitializer
  %shift = ashr <2 x i64> %a, %splat
  ret <2 x i64> %shift
}

define <4 x i32> @splatvar_shift_v4i32(<4 x i32> %a, <4 x i32> %b) {
; SSE2-LABEL: splatvar_shift_v4i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    xorps %xmm2, %xmm2
; SSE2-NEXT:    movss {{.*#+}} xmm2 = xmm1[0],xmm2[1,2,3]
; SSE2-NEXT:    psrad %xmm2, %xmm0
; SSE2-NEXT:    retq
;
; SSE41-LABEL: splatvar_shift_v4i32:
; SSE41:       # BB#0:
; SSE41-NEXT:    pxor %xmm2, %xmm2
; SSE41-NEXT:    pblendw {{.*#+}} xmm2 = xmm1[0,1],xmm2[2,3,4,5,6,7]
; SSE41-NEXT:    psrad %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: splatvar_shift_v4i32:
; AVX:       # BB#0:
; AVX-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; AVX-NEXT:    vpblendw {{.*#+}} xmm1 = xmm1[0,1],xmm2[2,3,4,5,6,7]
; AVX-NEXT:    vpsrad %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE-LABEL: splatvar_shift_v4i32:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    xorps %xmm2, %xmm2
; X32-SSE-NEXT:    movss {{.*#+}} xmm2 = xmm1[0],xmm2[1,2,3]
; X32-SSE-NEXT:    psrad %xmm2, %xmm0
; X32-SSE-NEXT:    retl
  %splat = shufflevector <4 x i32> %b, <4 x i32> undef, <4 x i32> zeroinitializer
  %shift = ashr <4 x i32> %a, %splat
  ret <4 x i32> %shift
}

define <8 x i16> @splatvar_shift_v8i16(<8 x i16> %a, <8 x i16> %b) {
; SSE2-LABEL: splatvar_shift_v8i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    movd %xmm1, %eax
; SSE2-NEXT:    movzwl %ax, %eax
; SSE2-NEXT:    movd %eax, %xmm1
; SSE2-NEXT:    psraw %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSE41-LABEL: splatvar_shift_v8i16:
; SSE41:       # BB#0:
; SSE41-NEXT:    pxor %xmm2, %xmm2
; SSE41-NEXT:    pblendw {{.*#+}} xmm2 = xmm1[0],xmm2[1,2,3,4,5,6,7]
; SSE41-NEXT:    psraw %xmm2, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: splatvar_shift_v8i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; AVX-NEXT:    vpblendw {{.*#+}} xmm1 = xmm1[0],xmm2[1,2,3,4,5,6,7]
; AVX-NEXT:    vpsraw %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE-LABEL: splatvar_shift_v8i16:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    movd %xmm1, %eax
; X32-SSE-NEXT:    movzwl %ax, %eax
; X32-SSE-NEXT:    movd %eax, %xmm1
; X32-SSE-NEXT:    psraw %xmm1, %xmm0
; X32-SSE-NEXT:    retl
  %splat = shufflevector <8 x i16> %b, <8 x i16> undef, <8 x i32> zeroinitializer
  %shift = ashr <8 x i16> %a, %splat
  ret <8 x i16> %shift
}

define <16 x i8> @splatvar_shift_v16i8(<16 x i8> %a, <16 x i8> %b) {
; SSE2-LABEL: splatvar_shift_v16i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[0,1,0,3]
; SSE2-NEXT:    pshuflw {{.*#+}} xmm1 = xmm1[0,0,0,0,4,5,6,7]
; SSE2-NEXT:    pshufhw {{.*#+}} xmm3 = xmm1[0,1,2,3,4,4,4,4]
; SSE2-NEXT:    punpckhbw {{.*#+}} xmm1 = xmm1[8],xmm0[8],xmm1[9],xmm0[9],xmm1[10],xmm0[10],xmm1[11],xmm0[11],xmm1[12],xmm0[12],xmm1[13],xmm0[13],xmm1[14],xmm0[14],xmm1[15],xmm0[15]
; SSE2-NEXT:    psllw $5, %xmm3
; SSE2-NEXT:    punpckhbw {{.*#+}} xmm4 = xmm4[8],xmm3[8],xmm4[9],xmm3[9],xmm4[10],xmm3[10],xmm4[11],xmm3[11],xmm4[12],xmm3[12],xmm4[13],xmm3[13],xmm4[14],xmm3[14],xmm4[15],xmm3[15]
; SSE2-NEXT:    pxor %xmm2, %xmm2
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtw %xmm4, %xmm5
; SSE2-NEXT:    movdqa %xmm5, %xmm6
; SSE2-NEXT:    pandn %xmm1, %xmm6
; SSE2-NEXT:    psraw $4, %xmm1
; SSE2-NEXT:    pand %xmm5, %xmm1
; SSE2-NEXT:    por %xmm6, %xmm1
; SSE2-NEXT:    paddw %xmm4, %xmm4
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtw %xmm4, %xmm5
; SSE2-NEXT:    movdqa %xmm5, %xmm6
; SSE2-NEXT:    pandn %xmm1, %xmm6
; SSE2-NEXT:    psraw $2, %xmm1
; SSE2-NEXT:    pand %xmm5, %xmm1
; SSE2-NEXT:    por %xmm6, %xmm1
; SSE2-NEXT:    paddw %xmm4, %xmm4
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtw %xmm4, %xmm5
; SSE2-NEXT:    movdqa %xmm5, %xmm4
; SSE2-NEXT:    pandn %xmm1, %xmm4
; SSE2-NEXT:    psraw $1, %xmm1
; SSE2-NEXT:    pand %xmm5, %xmm1
; SSE2-NEXT:    por %xmm4, %xmm1
; SSE2-NEXT:    psrlw $8, %xmm1
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm3 = xmm3[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    pxor %xmm4, %xmm4
; SSE2-NEXT:    pcmpgtw %xmm3, %xmm4
; SSE2-NEXT:    movdqa %xmm4, %xmm5
; SSE2-NEXT:    pandn %xmm0, %xmm5
; SSE2-NEXT:    psraw $4, %xmm0
; SSE2-NEXT:    pand %xmm4, %xmm0
; SSE2-NEXT:    por %xmm5, %xmm0
; SSE2-NEXT:    paddw %xmm3, %xmm3
; SSE2-NEXT:    pxor %xmm4, %xmm4
; SSE2-NEXT:    pcmpgtw %xmm3, %xmm4
; SSE2-NEXT:    movdqa %xmm4, %xmm5
; SSE2-NEXT:    pandn %xmm0, %xmm5
; SSE2-NEXT:    psraw $2, %xmm0
; SSE2-NEXT:    pand %xmm4, %xmm0
; SSE2-NEXT:    por %xmm5, %xmm0
; SSE2-NEXT:    paddw %xmm3, %xmm3
; SSE2-NEXT:    pcmpgtw %xmm3, %xmm2
; SSE2-NEXT:    movdqa %xmm2, %xmm3
; SSE2-NEXT:    pandn %xmm0, %xmm3
; SSE2-NEXT:    psraw $1, %xmm0
; SSE2-NEXT:    pand %xmm2, %xmm0
; SSE2-NEXT:    por %xmm3, %xmm0
; SSE2-NEXT:    psrlw $8, %xmm0
; SSE2-NEXT:    packuswb %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSE41-LABEL: splatvar_shift_v16i8:
; SSE41:       # BB#0:
; SSE41-NEXT:    movdqa %xmm0, %xmm2
; SSE41-NEXT:    pxor %xmm0, %xmm0
; SSE41-NEXT:    pshufb %xmm0, %xmm1
; SSE41-NEXT:    psllw $5, %xmm1
; SSE41-NEXT:    punpckhbw {{.*#+}} xmm0 = xmm0[8],xmm1[8],xmm0[9],xmm1[9],xmm0[10],xmm1[10],xmm0[11],xmm1[11],xmm0[12],xmm1[12],xmm0[13],xmm1[13],xmm0[14],xmm1[14],xmm0[15],xmm1[15]
; SSE41-NEXT:    punpckhbw {{.*#+}} xmm3 = xmm3[8],xmm2[8],xmm3[9],xmm2[9],xmm3[10],xmm2[10],xmm3[11],xmm2[11],xmm3[12],xmm2[12],xmm3[13],xmm2[13],xmm3[14],xmm2[14],xmm3[15],xmm2[15]
; SSE41-NEXT:    movdqa %xmm3, %xmm4
; SSE41-NEXT:    psraw $4, %xmm4
; SSE41-NEXT:    pblendvb %xmm4, %xmm3
; SSE41-NEXT:    movdqa %xmm3, %xmm4
; SSE41-NEXT:    psraw $2, %xmm4
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm4, %xmm3
; SSE41-NEXT:    movdqa %xmm3, %xmm4
; SSE41-NEXT:    psraw $1, %xmm4
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm4, %xmm3
; SSE41-NEXT:    psrlw $8, %xmm3
; SSE41-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; SSE41-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0],xmm2[0],xmm1[1],xmm2[1],xmm1[2],xmm2[2],xmm1[3],xmm2[3],xmm1[4],xmm2[4],xmm1[5],xmm2[5],xmm1[6],xmm2[6],xmm1[7],xmm2[7]
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psraw $4, %xmm2
; SSE41-NEXT:    pblendvb %xmm2, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psraw $2, %xmm2
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm2, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psraw $1, %xmm2
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm2, %xmm1
; SSE41-NEXT:    psrlw $8, %xmm1
; SSE41-NEXT:    packuswb %xmm3, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: splatvar_shift_v16i8:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpshufb %xmm2, %xmm1, %xmm1
; AVX1-NEXT:    vpsllw $5, %xmm1, %xmm1
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm2 = xmm0[8],xmm1[8],xmm0[9],xmm1[9],xmm0[10],xmm1[10],xmm0[11],xmm1[11],xmm0[12],xmm1[12],xmm0[13],xmm1[13],xmm0[14],xmm1[14],xmm0[15],xmm1[15]
; AVX1-NEXT:    vpunpckhbw {{.*#+}} xmm3 = xmm0[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15]
; AVX1-NEXT:    vpsraw $4, %xmm3, %xmm4
; AVX1-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm3
; AVX1-NEXT:    vpsraw $2, %xmm3, %xmm4
; AVX1-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm3
; AVX1-NEXT:    vpsraw $1, %xmm3, %xmm4
; AVX1-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX1-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm2
; AVX1-NEXT:    vpsrlw $8, %xmm2, %xmm2
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm1 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; AVX1-NEXT:    vpunpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; AVX1-NEXT:    vpsraw $4, %xmm0, %xmm3
; AVX1-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $2, %xmm0, %xmm3
; AVX1-NEXT:    vpaddw %xmm1, %xmm1, %xmm1
; AVX1-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $1, %xmm0, %xmm3
; AVX1-NEXT:    vpaddw %xmm1, %xmm1, %xmm1
; AVX1-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX1-NEXT:    vpsrlw $8, %xmm0, %xmm0
; AVX1-NEXT:    vpackuswb %xmm2, %xmm0, %xmm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatvar_shift_v16i8:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpbroadcastb %xmm1, %xmm1
; AVX2-NEXT:    vpsllw $5, %xmm1, %xmm1
; AVX2-NEXT:    vpunpckhbw {{.*#+}} xmm2 = xmm0[8],xmm1[8],xmm0[9],xmm1[9],xmm0[10],xmm1[10],xmm0[11],xmm1[11],xmm0[12],xmm1[12],xmm0[13],xmm1[13],xmm0[14],xmm1[14],xmm0[15],xmm1[15]
; AVX2-NEXT:    vpunpckhbw {{.*#+}} xmm3 = xmm0[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15]
; AVX2-NEXT:    vpsraw $4, %xmm3, %xmm4
; AVX2-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm3
; AVX2-NEXT:    vpsraw $2, %xmm3, %xmm4
; AVX2-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX2-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm3
; AVX2-NEXT:    vpsraw $1, %xmm3, %xmm4
; AVX2-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX2-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm2
; AVX2-NEXT:    vpsrlw $8, %xmm2, %xmm2
; AVX2-NEXT:    vpunpcklbw {{.*#+}} xmm1 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; AVX2-NEXT:    vpunpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; AVX2-NEXT:    vpsraw $4, %xmm0, %xmm3
; AVX2-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX2-NEXT:    vpsraw $2, %xmm0, %xmm3
; AVX2-NEXT:    vpaddw %xmm1, %xmm1, %xmm1
; AVX2-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX2-NEXT:    vpsraw $1, %xmm0, %xmm3
; AVX2-NEXT:    vpaddw %xmm1, %xmm1, %xmm1
; AVX2-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX2-NEXT:    vpsrlw $8, %xmm0, %xmm0
; AVX2-NEXT:    vpackuswb %xmm2, %xmm0, %xmm0
; AVX2-NEXT:    retq
;
; X32-SSE-LABEL: splatvar_shift_v16i8:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[0,1,0,3]
; X32-SSE-NEXT:    pshuflw {{.*#+}} xmm1 = xmm1[0,0,0,0,4,5,6,7]
; X32-SSE-NEXT:    pshufhw {{.*#+}} xmm3 = xmm1[0,1,2,3,4,4,4,4]
; X32-SSE-NEXT:    punpckhbw {{.*#+}} xmm1 = xmm1[8],xmm0[8],xmm1[9],xmm0[9],xmm1[10],xmm0[10],xmm1[11],xmm0[11],xmm1[12],xmm0[12],xmm1[13],xmm0[13],xmm1[14],xmm0[14],xmm1[15],xmm0[15]
; X32-SSE-NEXT:    psllw $5, %xmm3
; X32-SSE-NEXT:    punpckhbw {{.*#+}} xmm4 = xmm4[8],xmm3[8],xmm4[9],xmm3[9],xmm4[10],xmm3[10],xmm4[11],xmm3[11],xmm4[12],xmm3[12],xmm4[13],xmm3[13],xmm4[14],xmm3[14],xmm4[15],xmm3[15]
; X32-SSE-NEXT:    pxor %xmm2, %xmm2
; X32-SSE-NEXT:    pxor %xmm5, %xmm5
; X32-SSE-NEXT:    pcmpgtw %xmm4, %xmm5
; X32-SSE-NEXT:    movdqa %xmm5, %xmm6
; X32-SSE-NEXT:    pandn %xmm1, %xmm6
; X32-SSE-NEXT:    psraw $4, %xmm1
; X32-SSE-NEXT:    pand %xmm5, %xmm1
; X32-SSE-NEXT:    por %xmm6, %xmm1
; X32-SSE-NEXT:    paddw %xmm4, %xmm4
; X32-SSE-NEXT:    pxor %xmm5, %xmm5
; X32-SSE-NEXT:    pcmpgtw %xmm4, %xmm5
; X32-SSE-NEXT:    movdqa %xmm5, %xmm6
; X32-SSE-NEXT:    pandn %xmm1, %xmm6
; X32-SSE-NEXT:    psraw $2, %xmm1
; X32-SSE-NEXT:    pand %xmm5, %xmm1
; X32-SSE-NEXT:    por %xmm6, %xmm1
; X32-SSE-NEXT:    paddw %xmm4, %xmm4
; X32-SSE-NEXT:    pxor %xmm5, %xmm5
; X32-SSE-NEXT:    pcmpgtw %xmm4, %xmm5
; X32-SSE-NEXT:    movdqa %xmm5, %xmm4
; X32-SSE-NEXT:    pandn %xmm1, %xmm4
; X32-SSE-NEXT:    psraw $1, %xmm1
; X32-SSE-NEXT:    pand %xmm5, %xmm1
; X32-SSE-NEXT:    por %xmm4, %xmm1
; X32-SSE-NEXT:    psrlw $8, %xmm1
; X32-SSE-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; X32-SSE-NEXT:    punpcklbw {{.*#+}} xmm3 = xmm3[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; X32-SSE-NEXT:    pxor %xmm4, %xmm4
; X32-SSE-NEXT:    pcmpgtw %xmm3, %xmm4
; X32-SSE-NEXT:    movdqa %xmm4, %xmm5
; X32-SSE-NEXT:    pandn %xmm0, %xmm5
; X32-SSE-NEXT:    psraw $4, %xmm0
; X32-SSE-NEXT:    pand %xmm4, %xmm0
; X32-SSE-NEXT:    por %xmm5, %xmm0
; X32-SSE-NEXT:    paddw %xmm3, %xmm3
; X32-SSE-NEXT:    pxor %xmm4, %xmm4
; X32-SSE-NEXT:    pcmpgtw %xmm3, %xmm4
; X32-SSE-NEXT:    movdqa %xmm4, %xmm5
; X32-SSE-NEXT:    pandn %xmm0, %xmm5
; X32-SSE-NEXT:    psraw $2, %xmm0
; X32-SSE-NEXT:    pand %xmm4, %xmm0
; X32-SSE-NEXT:    por %xmm5, %xmm0
; X32-SSE-NEXT:    paddw %xmm3, %xmm3
; X32-SSE-NEXT:    pcmpgtw %xmm3, %xmm2
; X32-SSE-NEXT:    movdqa %xmm2, %xmm3
; X32-SSE-NEXT:    pandn %xmm0, %xmm3
; X32-SSE-NEXT:    psraw $1, %xmm0
; X32-SSE-NEXT:    pand %xmm2, %xmm0
; X32-SSE-NEXT:    por %xmm3, %xmm0
; X32-SSE-NEXT:    psrlw $8, %xmm0
; X32-SSE-NEXT:    packuswb %xmm1, %xmm0
; X32-SSE-NEXT:    retl
  %splat = shufflevector <16 x i8> %b, <16 x i8> undef, <16 x i32> zeroinitializer
  %shift = ashr <16 x i8> %a, %splat
  ret <16 x i8> %shift
}

;
; Constant Shifts
;

define <2 x i64> @constant_shift_v2i64(<2 x i64> %a) {
; SSE2-LABEL: constant_shift_v2i64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    sarq %rax
; SSE2-NEXT:    movd %rax, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,3,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    sarq $7, %rax
; SSE2-NEXT:    movd %rax, %xmm0
; SSE2-NEXT:    punpcklqdq {{.*#+}} xmm1 = xmm1[0],xmm0[0]
; SSE2-NEXT:    movdqa %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSE41-LABEL: constant_shift_v2i64:
; SSE41:       # BB#0:
; SSE41-NEXT:    pextrq $1, %xmm0, %rax
; SSE41-NEXT:    sarq $7, %rax
; SSE41-NEXT:    movd %rax, %xmm1
; SSE41-NEXT:    movd %xmm0, %rax
; SSE41-NEXT:    sarq %rax
; SSE41-NEXT:    movd %rax, %xmm0
; SSE41-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSE41-NEXT:    retq
;
; AVX-LABEL: constant_shift_v2i64:
; AVX:       # BB#0:
; AVX-NEXT:    vpextrq $1, %xmm0, %rax
; AVX-NEXT:    sarq $7, %rax
; AVX-NEXT:    vmovq %rax, %xmm1
; AVX-NEXT:    vmovq %xmm0, %rax
; AVX-NEXT:    sarq %rax
; AVX-NEXT:    vmovq %rax, %xmm0
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; AVX-NEXT:    retq
;
; X32-SSE-LABEL: constant_shift_v2i64:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; X32-SSE-NEXT:    movd %xmm1, %eax
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[3,1,2,3]
; X32-SSE-NEXT:    movd %xmm1, %ecx
; X32-SSE-NEXT:    shrdl $7, %ecx, %eax
; X32-SSE-NEXT:    movd %eax, %xmm1
; X32-SSE-NEXT:    movd %xmm0, %eax
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; X32-SSE-NEXT:    movd %xmm0, %edx
; X32-SSE-NEXT:    shrdl $1, %edx, %eax
; X32-SSE-NEXT:    movd %eax, %xmm0
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; X32-SSE-NEXT:    sarl $7, %ecx
; X32-SSE-NEXT:    movd %ecx, %xmm1
; X32-SSE-NEXT:    sarl %edx
; X32-SSE-NEXT:    movd %edx, %xmm2
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm1[0],xmm2[1],xmm1[1]
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm2[0],xmm0[1],xmm2[1]
; X32-SSE-NEXT:    retl
  %shift = ashr <2 x i64> %a, <i64 1, i64 7>
  ret <2 x i64> %shift
}

define <4 x i32> @constant_shift_v4i32(<4 x i32> %a) {
; SSE2-LABEL: constant_shift_v4i32:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $7, %xmm1
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    psrad $5, %xmm2
; SSE2-NEXT:    movsd {{.*#+}} xmm1 = xmm2[0],xmm1[1]
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[1,3,2,3]
; SSE2-NEXT:    movdqa %xmm0, %xmm2
; SSE2-NEXT:    psrad $6, %xmm2
; SSE2-NEXT:    psrad $4, %xmm0
; SSE2-NEXT:    movsd {{.*#+}} xmm2 = xmm0[0],xmm2[1]
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm2[0,2,2,3]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
;
; SSE41-LABEL: constant_shift_v4i32:
; SSE41:       # BB#0:
; SSE41-NEXT:    movdqa %xmm0, %xmm1
; SSE41-NEXT:    psrad $7, %xmm1
; SSE41-NEXT:    movdqa %xmm0, %xmm2
; SSE41-NEXT:    psrad $5, %xmm2
; SSE41-NEXT:    pblendw {{.*#+}} xmm2 = xmm2[0,1,2,3],xmm1[4,5,6,7]
; SSE41-NEXT:    movdqa %xmm0, %xmm1
; SSE41-NEXT:    psrad $6, %xmm1
; SSE41-NEXT:    psrad $4, %xmm0
; SSE41-NEXT:    pblendw {{.*#+}} xmm0 = xmm0[0,1,2,3],xmm1[4,5,6,7]
; SSE41-NEXT:    pblendw {{.*#+}} xmm0 = xmm0[0,1],xmm2[2,3],xmm0[4,5],xmm2[6,7]
; SSE41-NEXT:    retq
;
; AVX1-LABEL: constant_shift_v4i32:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpsrad $7, %xmm0, %xmm1
; AVX1-NEXT:    vpsrad $5, %xmm0, %xmm2
; AVX1-NEXT:    vpblendw {{.*#+}} xmm1 = xmm2[0,1,2,3],xmm1[4,5,6,7]
; AVX1-NEXT:    vpsrad $6, %xmm0, %xmm2
; AVX1-NEXT:    vpsrad $4, %xmm0, %xmm0
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1,2,3],xmm2[4,5,6,7]
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1],xmm1[2,3],xmm0[4,5],xmm1[6,7]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: constant_shift_v4i32:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsravd {{.*}}(%rip), %xmm0, %xmm0
; AVX2-NEXT:    retq
;
; X32-SSE-LABEL: constant_shift_v4i32:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    movdqa %xmm0, %xmm1
; X32-SSE-NEXT:    psrad $7, %xmm1
; X32-SSE-NEXT:    movdqa %xmm0, %xmm2
; X32-SSE-NEXT:    psrad $5, %xmm2
; X32-SSE-NEXT:    movsd {{.*#+}} xmm1 = xmm2[0],xmm1[1]
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[1,3,2,3]
; X32-SSE-NEXT:    movdqa %xmm0, %xmm2
; X32-SSE-NEXT:    psrad $6, %xmm2
; X32-SSE-NEXT:    psrad $4, %xmm0
; X32-SSE-NEXT:    movsd {{.*#+}} xmm2 = xmm0[0],xmm2[1]
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm0 = xmm2[0,2,2,3]
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; X32-SSE-NEXT:    retl
  %shift = ashr <4 x i32> %a, <i32 4, i32 5, i32 6, i32 7>
  ret <4 x i32> %shift
}

define <8 x i16> @constant_shift_v8i16(<8 x i16> %a) {
; SSE2-LABEL: constant_shift_v8i16:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psraw $4, %xmm1
; SSE2-NEXT:    movsd {{.*#+}} xmm1 = xmm0[0],xmm1[1]
; SSE2-NEXT:    pshufd {{.*#+}} xmm2 = xmm1[0,2,2,3]
; SSE2-NEXT:    psraw $2, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm1[1,3,2,3]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1]
; SSE2-NEXT:    movdqa {{.*#+}} xmm0 = [65535,0,65535,0,65535,0,65535,0]
; SSE2-NEXT:    movdqa %xmm2, %xmm1
; SSE2-NEXT:    pand %xmm0, %xmm1
; SSE2-NEXT:    psraw $1, %xmm2
; SSE2-NEXT:    pandn %xmm2, %xmm0
; SSE2-NEXT:    por %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSE41-LABEL: constant_shift_v8i16:
; SSE41:       # BB#0:
; SSE41-NEXT:    movdqa %xmm0, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psraw $8, %xmm2
; SSE41-NEXT:    movaps {{.*#+}} xmm0 = [0,4112,8224,12336,16448,20560,24672,28784]
; SSE41-NEXT:    pblendvb %xmm2, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psraw $4, %xmm2
; SSE41-NEXT:    movaps {{.*#+}} xmm0 = [0,8224,16448,24672,32896,41120,49344,57568]
; SSE41-NEXT:    pblendvb %xmm2, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psraw $2, %xmm2
; SSE41-NEXT:    movaps {{.*#+}} xmm0 = [0,16448,32896,49344,256,16704,33152,49600]
; SSE41-NEXT:    pblendvb %xmm2, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm2
; SSE41-NEXT:    psraw $1, %xmm2
; SSE41-NEXT:    movaps {{.*#+}} xmm0 = [0,32896,256,33152,512,33408,768,33664]
; SSE41-NEXT:    pblendvb %xmm2, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm0
; SSE41-NEXT:    retq
;
; AVX1-LABEL: constant_shift_v8i16:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpsraw $8, %xmm0, %xmm1
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm2 = [0,4112,8224,12336,16448,20560,24672,28784]
; AVX1-NEXT:    vpblendvb %xmm2, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $4, %xmm0, %xmm1
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm2 = [0,8224,16448,24672,32896,41120,49344,57568]
; AVX1-NEXT:    vpblendvb %xmm2, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $2, %xmm0, %xmm1
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm2 = [0,16448,32896,49344,256,16704,33152,49600]
; AVX1-NEXT:    vpblendvb %xmm2, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    vpsraw $1, %xmm0, %xmm1
; AVX1-NEXT:    vmovdqa {{.*#+}} xmm2 = [0,32896,256,33152,512,33408,768,33664]
; AVX1-NEXT:    vpblendvb %xmm2, %xmm1, %xmm0, %xmm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: constant_shift_v8i16:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpmovsxwd %xmm0, %ymm0
; AVX2-NEXT:    vpmovzxwd {{.*#+}} ymm1 = mem[0],zero,mem[1],zero,mem[2],zero,mem[3],zero,mem[4],zero,mem[5],zero,mem[6],zero,mem[7],zero
; AVX2-NEXT:    vpsravd %ymm1, %ymm0, %ymm0
; AVX2-NEXT:    vpshufb {{.*#+}} ymm0 = ymm0[0,1,4,5,8,9,12,13],zero,zero,zero,zero,zero,zero,zero,zero,ymm0[16,17,20,21,24,25,28,29],zero,zero,zero,zero,zero,zero,zero,zero
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,2,2,3]
; AVX2-NEXT:    vzeroupper
; AVX2-NEXT:    retq
;
; X32-SSE-LABEL: constant_shift_v8i16:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    movdqa %xmm0, %xmm1
; X32-SSE-NEXT:    psraw $4, %xmm1
; X32-SSE-NEXT:    movsd {{.*#+}} xmm1 = xmm0[0],xmm1[1]
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm2 = xmm1[0,2,2,3]
; X32-SSE-NEXT:    psraw $2, %xmm1
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm0 = xmm1[1,3,2,3]
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm2 = xmm2[0],xmm0[0],xmm2[1],xmm0[1]
; X32-SSE-NEXT:    movdqa {{.*#+}} xmm0 = [65535,0,65535,0,65535,0,65535,0]
; X32-SSE-NEXT:    movdqa %xmm2, %xmm1
; X32-SSE-NEXT:    pand %xmm0, %xmm1
; X32-SSE-NEXT:    psraw $1, %xmm2
; X32-SSE-NEXT:    pandn %xmm2, %xmm0
; X32-SSE-NEXT:    por %xmm1, %xmm0
; X32-SSE-NEXT:    retl
  %shift = ashr <8 x i16> %a, <i16 0, i16 1, i16 2, i16 3, i16 4, i16 5, i16 6, i16 7>
  ret <8 x i16> %shift
}

define <16 x i8> @constant_shift_v16i8(<16 x i8> %a) {
; SSE2-LABEL: constant_shift_v16i8:
; SSE2:       # BB#0:
; SSE2-NEXT:    punpckhbw {{.*#+}} xmm1 = xmm1[8],xmm0[8],xmm1[9],xmm0[9],xmm1[10],xmm0[10],xmm1[11],xmm0[11],xmm1[12],xmm0[12],xmm1[13],xmm0[13],xmm1[14],xmm0[14],xmm1[15],xmm0[15]
; SSE2-NEXT:    movdqa {{.*#+}} xmm3 = [0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0]
; SSE2-NEXT:    psllw $5, %xmm3
; SSE2-NEXT:    punpckhbw {{.*#+}} xmm4 = xmm4[8],xmm3[8],xmm4[9],xmm3[9],xmm4[10],xmm3[10],xmm4[11],xmm3[11],xmm4[12],xmm3[12],xmm4[13],xmm3[13],xmm4[14],xmm3[14],xmm4[15],xmm3[15]
; SSE2-NEXT:    pxor %xmm2, %xmm2
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtw %xmm4, %xmm5
; SSE2-NEXT:    movdqa %xmm5, %xmm6
; SSE2-NEXT:    pandn %xmm1, %xmm6
; SSE2-NEXT:    psraw $4, %xmm1
; SSE2-NEXT:    pand %xmm5, %xmm1
; SSE2-NEXT:    por %xmm6, %xmm1
; SSE2-NEXT:    paddw %xmm4, %xmm4
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtw %xmm4, %xmm5
; SSE2-NEXT:    movdqa %xmm5, %xmm6
; SSE2-NEXT:    pandn %xmm1, %xmm6
; SSE2-NEXT:    psraw $2, %xmm1
; SSE2-NEXT:    pand %xmm5, %xmm1
; SSE2-NEXT:    por %xmm6, %xmm1
; SSE2-NEXT:    paddw %xmm4, %xmm4
; SSE2-NEXT:    pxor %xmm5, %xmm5
; SSE2-NEXT:    pcmpgtw %xmm4, %xmm5
; SSE2-NEXT:    movdqa %xmm5, %xmm4
; SSE2-NEXT:    pandn %xmm1, %xmm4
; SSE2-NEXT:    psraw $1, %xmm1
; SSE2-NEXT:    pand %xmm5, %xmm1
; SSE2-NEXT:    por %xmm4, %xmm1
; SSE2-NEXT:    psrlw $8, %xmm1
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    punpcklbw {{.*#+}} xmm3 = xmm3[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE2-NEXT:    pxor %xmm4, %xmm4
; SSE2-NEXT:    pcmpgtw %xmm3, %xmm4
; SSE2-NEXT:    movdqa %xmm4, %xmm5
; SSE2-NEXT:    pandn %xmm0, %xmm5
; SSE2-NEXT:    psraw $4, %xmm0
; SSE2-NEXT:    pand %xmm4, %xmm0
; SSE2-NEXT:    por %xmm5, %xmm0
; SSE2-NEXT:    paddw %xmm3, %xmm3
; SSE2-NEXT:    pxor %xmm4, %xmm4
; SSE2-NEXT:    pcmpgtw %xmm3, %xmm4
; SSE2-NEXT:    movdqa %xmm4, %xmm5
; SSE2-NEXT:    pandn %xmm0, %xmm5
; SSE2-NEXT:    psraw $2, %xmm0
; SSE2-NEXT:    pand %xmm4, %xmm0
; SSE2-NEXT:    por %xmm5, %xmm0
; SSE2-NEXT:    paddw %xmm3, %xmm3
; SSE2-NEXT:    pcmpgtw %xmm3, %xmm2
; SSE2-NEXT:    movdqa %xmm2, %xmm3
; SSE2-NEXT:    pandn %xmm0, %xmm3
; SSE2-NEXT:    psraw $1, %xmm0
; SSE2-NEXT:    pand %xmm2, %xmm0
; SSE2-NEXT:    por %xmm3, %xmm0
; SSE2-NEXT:    psrlw $8, %xmm0
; SSE2-NEXT:    packuswb %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSE41-LABEL: constant_shift_v16i8:
; SSE41:       # BB#0:
; SSE41-NEXT:    movdqa %xmm0, %xmm1
; SSE41-NEXT:    movdqa {{.*#+}} xmm3 = [0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0]
; SSE41-NEXT:    psllw $5, %xmm3
; SSE41-NEXT:    punpckhbw {{.*#+}} xmm0 = xmm0[8],xmm3[8],xmm0[9],xmm3[9],xmm0[10],xmm3[10],xmm0[11],xmm3[11],xmm0[12],xmm3[12],xmm0[13],xmm3[13],xmm0[14],xmm3[14],xmm0[15],xmm3[15]
; SSE41-NEXT:    punpckhbw {{.*#+}} xmm2 = xmm2[8],xmm1[8],xmm2[9],xmm1[9],xmm2[10],xmm1[10],xmm2[11],xmm1[11],xmm2[12],xmm1[12],xmm2[13],xmm1[13],xmm2[14],xmm1[14],xmm2[15],xmm1[15]
; SSE41-NEXT:    movdqa %xmm2, %xmm4
; SSE41-NEXT:    psraw $4, %xmm4
; SSE41-NEXT:    pblendvb %xmm4, %xmm2
; SSE41-NEXT:    movdqa %xmm2, %xmm4
; SSE41-NEXT:    psraw $2, %xmm4
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm4, %xmm2
; SSE41-NEXT:    movdqa %xmm2, %xmm4
; SSE41-NEXT:    psraw $1, %xmm4
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm4, %xmm2
; SSE41-NEXT:    psrlw $8, %xmm2
; SSE41-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0],xmm3[0],xmm0[1],xmm3[1],xmm0[2],xmm3[2],xmm0[3],xmm3[3],xmm0[4],xmm3[4],xmm0[5],xmm3[5],xmm0[6],xmm3[6],xmm0[7],xmm3[7]
; SSE41-NEXT:    punpcklbw {{.*#+}} xmm1 = xmm1[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; SSE41-NEXT:    movdqa %xmm1, %xmm3
; SSE41-NEXT:    psraw $4, %xmm3
; SSE41-NEXT:    pblendvb %xmm3, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm3
; SSE41-NEXT:    psraw $2, %xmm3
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm3, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm3
; SSE41-NEXT:    psraw $1, %xmm3
; SSE41-NEXT:    paddw %xmm0, %xmm0
; SSE41-NEXT:    pblendvb %xmm3, %xmm1
; SSE41-NEXT:    psrlw $8, %xmm1
; SSE41-NEXT:    packuswb %xmm2, %xmm1
; SSE41-NEXT:    movdqa %xmm1, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: constant_shift_v16i8:
; AVX:       # BB#0:
; AVX-NEXT:    vmovdqa {{.*#+}} xmm1 = [0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0]
; AVX-NEXT:    vpsllw $5, %xmm1, %xmm1
; AVX-NEXT:    vpunpckhbw {{.*#+}} xmm2 = xmm0[8],xmm1[8],xmm0[9],xmm1[9],xmm0[10],xmm1[10],xmm0[11],xmm1[11],xmm0[12],xmm1[12],xmm0[13],xmm1[13],xmm0[14],xmm1[14],xmm0[15],xmm1[15]
; AVX-NEXT:    vpunpckhbw {{.*#+}} xmm3 = xmm0[8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15]
; AVX-NEXT:    vpsraw $4, %xmm3, %xmm4
; AVX-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm3
; AVX-NEXT:    vpsraw $2, %xmm3, %xmm4
; AVX-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm3
; AVX-NEXT:    vpsraw $1, %xmm3, %xmm4
; AVX-NEXT:    vpaddw %xmm2, %xmm2, %xmm2
; AVX-NEXT:    vpblendvb %xmm2, %xmm4, %xmm3, %xmm2
; AVX-NEXT:    vpsrlw $8, %xmm2, %xmm2
; AVX-NEXT:    vpunpcklbw {{.*#+}} xmm1 = xmm0[0],xmm1[0],xmm0[1],xmm1[1],xmm0[2],xmm1[2],xmm0[3],xmm1[3],xmm0[4],xmm1[4],xmm0[5],xmm1[5],xmm0[6],xmm1[6],xmm0[7],xmm1[7]
; AVX-NEXT:    vpunpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; AVX-NEXT:    vpsraw $4, %xmm0, %xmm3
; AVX-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX-NEXT:    vpsraw $2, %xmm0, %xmm3
; AVX-NEXT:    vpaddw %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX-NEXT:    vpsraw $1, %xmm0, %xmm3
; AVX-NEXT:    vpaddw %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vpblendvb %xmm1, %xmm3, %xmm0, %xmm0
; AVX-NEXT:    vpsrlw $8, %xmm0, %xmm0
; AVX-NEXT:    vpackuswb %xmm2, %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE-LABEL: constant_shift_v16i8:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    punpckhbw {{.*#+}} xmm1 = xmm1[8],xmm0[8],xmm1[9],xmm0[9],xmm1[10],xmm0[10],xmm1[11],xmm0[11],xmm1[12],xmm0[12],xmm1[13],xmm0[13],xmm1[14],xmm0[14],xmm1[15],xmm0[15]
; X32-SSE-NEXT:    movdqa {{.*#+}} xmm3 = [0,1,2,3,4,5,6,7,7,6,5,4,3,2,1,0]
; X32-SSE-NEXT:    psllw $5, %xmm3
; X32-SSE-NEXT:    punpckhbw {{.*#+}} xmm4 = xmm4[8],xmm3[8],xmm4[9],xmm3[9],xmm4[10],xmm3[10],xmm4[11],xmm3[11],xmm4[12],xmm3[12],xmm4[13],xmm3[13],xmm4[14],xmm3[14],xmm4[15],xmm3[15]
; X32-SSE-NEXT:    pxor %xmm2, %xmm2
; X32-SSE-NEXT:    pxor %xmm5, %xmm5
; X32-SSE-NEXT:    pcmpgtw %xmm4, %xmm5
; X32-SSE-NEXT:    movdqa %xmm5, %xmm6
; X32-SSE-NEXT:    pandn %xmm1, %xmm6
; X32-SSE-NEXT:    psraw $4, %xmm1
; X32-SSE-NEXT:    pand %xmm5, %xmm1
; X32-SSE-NEXT:    por %xmm6, %xmm1
; X32-SSE-NEXT:    paddw %xmm4, %xmm4
; X32-SSE-NEXT:    pxor %xmm5, %xmm5
; X32-SSE-NEXT:    pcmpgtw %xmm4, %xmm5
; X32-SSE-NEXT:    movdqa %xmm5, %xmm6
; X32-SSE-NEXT:    pandn %xmm1, %xmm6
; X32-SSE-NEXT:    psraw $2, %xmm1
; X32-SSE-NEXT:    pand %xmm5, %xmm1
; X32-SSE-NEXT:    por %xmm6, %xmm1
; X32-SSE-NEXT:    paddw %xmm4, %xmm4
; X32-SSE-NEXT:    pxor %xmm5, %xmm5
; X32-SSE-NEXT:    pcmpgtw %xmm4, %xmm5
; X32-SSE-NEXT:    movdqa %xmm5, %xmm4
; X32-SSE-NEXT:    pandn %xmm1, %xmm4
; X32-SSE-NEXT:    psraw $1, %xmm1
; X32-SSE-NEXT:    pand %xmm5, %xmm1
; X32-SSE-NEXT:    por %xmm4, %xmm1
; X32-SSE-NEXT:    psrlw $8, %xmm1
; X32-SSE-NEXT:    punpcklbw {{.*#+}} xmm0 = xmm0[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; X32-SSE-NEXT:    punpcklbw {{.*#+}} xmm3 = xmm3[0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7]
; X32-SSE-NEXT:    pxor %xmm4, %xmm4
; X32-SSE-NEXT:    pcmpgtw %xmm3, %xmm4
; X32-SSE-NEXT:    movdqa %xmm4, %xmm5
; X32-SSE-NEXT:    pandn %xmm0, %xmm5
; X32-SSE-NEXT:    psraw $4, %xmm0
; X32-SSE-NEXT:    pand %xmm4, %xmm0
; X32-SSE-NEXT:    por %xmm5, %xmm0
; X32-SSE-NEXT:    paddw %xmm3, %xmm3
; X32-SSE-NEXT:    pxor %xmm4, %xmm4
; X32-SSE-NEXT:    pcmpgtw %xmm3, %xmm4
; X32-SSE-NEXT:    movdqa %xmm4, %xmm5
; X32-SSE-NEXT:    pandn %xmm0, %xmm5
; X32-SSE-NEXT:    psraw $2, %xmm0
; X32-SSE-NEXT:    pand %xmm4, %xmm0
; X32-SSE-NEXT:    por %xmm5, %xmm0
; X32-SSE-NEXT:    paddw %xmm3, %xmm3
; X32-SSE-NEXT:    pcmpgtw %xmm3, %xmm2
; X32-SSE-NEXT:    movdqa %xmm2, %xmm3
; X32-SSE-NEXT:    pandn %xmm0, %xmm3
; X32-SSE-NEXT:    psraw $1, %xmm0
; X32-SSE-NEXT:    pand %xmm2, %xmm0
; X32-SSE-NEXT:    por %xmm3, %xmm0
; X32-SSE-NEXT:    psrlw $8, %xmm0
; X32-SSE-NEXT:    packuswb %xmm1, %xmm0
; X32-SSE-NEXT:    retl
  %shift = ashr <16 x i8> %a, <i8 0, i8 1, i8 2, i8 3, i8 4, i8 5, i8 6, i8 7, i8 7, i8 6, i8 5, i8 4, i8 3, i8 2, i8 1, i8 0>
  ret <16 x i8> %shift
}

;
; Uniform Constant Shifts
;

define <2 x i64> @splatconstant_shift_v2i64(<2 x i64> %a) {
; SSE2-LABEL: splatconstant_shift_v2i64:
; SSE2:       # BB#0:
; SSE2-NEXT:    movdqa %xmm0, %xmm1
; SSE2-NEXT:    psrad $7, %xmm1
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[1,3,2,3]
; SSE2-NEXT:    psrlq $7, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[0,2,2,3]
; SSE2-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; SSE2-NEXT:    retq
;
; SSE41-LABEL: splatconstant_shift_v2i64:
; SSE41:       # BB#0:
; SSE41-NEXT:    movdqa %xmm0, %xmm1
; SSE41-NEXT:    psrad $7, %xmm1
; SSE41-NEXT:    psrlq $7, %xmm0
; SSE41-NEXT:    pblendw {{.*#+}} xmm0 = xmm0[0,1],xmm1[2,3],xmm0[4,5],xmm1[6,7]
; SSE41-NEXT:    retq
;
; AVX1-LABEL: splatconstant_shift_v2i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpsrad $7, %xmm0, %xmm1
; AVX1-NEXT:    vpsrlq $7, %xmm0, %xmm0
; AVX1-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0,1],xmm1[2,3],xmm0[4,5],xmm1[6,7]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splatconstant_shift_v2i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsrad $7, %xmm0, %xmm1
; AVX2-NEXT:    vpsrlq $7, %xmm0, %xmm0
; AVX2-NEXT:    vpblendd {{.*#+}} xmm0 = xmm0[0],xmm1[1],xmm0[2],xmm1[3]
; AVX2-NEXT:    retq
;
; X32-SSE-LABEL: splatconstant_shift_v2i64:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    movdqa %xmm0, %xmm1
; X32-SSE-NEXT:    psrad $7, %xmm1
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm1 = xmm1[1,3,2,3]
; X32-SSE-NEXT:    psrlq $7, %xmm0
; X32-SSE-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[0,2,2,3]
; X32-SSE-NEXT:    punpckldq {{.*#+}} xmm0 = xmm0[0],xmm1[0],xmm0[1],xmm1[1]
; X32-SSE-NEXT:    retl
  %shift = ashr <2 x i64> %a, <i64 7, i64 7>
  ret <2 x i64> %shift
}

define <4 x i32> @splatconstant_shift_v4i32(<4 x i32> %a) {
; SSE-LABEL: splatconstant_shift_v4i32:
; SSE:       # BB#0:
; SSE-NEXT:    psrad $5, %xmm0
; SSE-NEXT:    retq
;
; AVX-LABEL: splatconstant_shift_v4i32:
; AVX:       # BB#0:
; AVX-NEXT:    vpsrad $5, %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE-LABEL: splatconstant_shift_v4i32:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    psrad $5, %xmm0
; X32-SSE-NEXT:    retl
  %shift = ashr <4 x i32> %a, <i32 5, i32 5, i32 5, i32 5>
  ret <4 x i32> %shift
}

define <8 x i16> @splatconstant_shift_v8i16(<8 x i16> %a) {
; SSE-LABEL: splatconstant_shift_v8i16:
; SSE:       # BB#0:
; SSE-NEXT:    psraw $3, %xmm0
; SSE-NEXT:    retq
;
; AVX-LABEL: splatconstant_shift_v8i16:
; AVX:       # BB#0:
; AVX-NEXT:    vpsraw $3, %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE-LABEL: splatconstant_shift_v8i16:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    psraw $3, %xmm0
; X32-SSE-NEXT:    retl
  %shift = ashr <8 x i16> %a, <i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3, i16 3>
  ret <8 x i16> %shift
}

define <16 x i8> @splatconstant_shift_v16i8(<16 x i8> %a) {
; SSE-LABEL: splatconstant_shift_v16i8:
; SSE:       # BB#0:
; SSE-NEXT:    psrlw $3, %xmm0
; SSE-NEXT:    pand {{.*}}(%rip), %xmm0
; SSE-NEXT:    movdqa {{.*#+}} xmm1 = [16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16]
; SSE-NEXT:    pxor %xmm1, %xmm0
; SSE-NEXT:    psubb %xmm1, %xmm0
; SSE-NEXT:    retq
;
; AVX-LABEL: splatconstant_shift_v16i8:
; AVX:       # BB#0:
; AVX-NEXT:    vpsrlw $3, %xmm0, %xmm0
; AVX-NEXT:    vpand {{.*}}(%rip), %xmm0, %xmm0
; AVX-NEXT:    vmovdqa {{.*#+}} xmm1 = [16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16]
; AVX-NEXT:    vpxor %xmm1, %xmm0, %xmm0
; AVX-NEXT:    vpsubb %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
;
; X32-SSE-LABEL: splatconstant_shift_v16i8:
; X32-SSE:       # BB#0:
; X32-SSE-NEXT:    psrlw $3, %xmm0
; X32-SSE-NEXT:    pand .LCPI15_0, %xmm0
; X32-SSE-NEXT:    movdqa {{.*#+}} xmm1 = [16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16]
; X32-SSE-NEXT:    pxor %xmm1, %xmm0
; X32-SSE-NEXT:    psubb %xmm1, %xmm0
; X32-SSE-NEXT:    retl
  %shift = ashr <16 x i8> %a, <i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3, i8 3>
  ret <16 x i8> %shift
}
