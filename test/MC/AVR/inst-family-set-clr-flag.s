; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s

; BSET
bset 4
bset 2
bset 0
bset 1


; BCLR
bclr 4
bclr 2
bclr 0
bclr 1

; Carry (C flag)
sec
clc

; Negative (N flag)
sen
cln

; Zero (Z flag)
sez
clz

; Interrupt (I flag)
sei
cli

; Signed test (S flag)
ses
cls

; Two's compliment overflow (V flag)
sev
clv

; T (T flag)
set
clt

; Half carry (H flag)
seh
clh

foo:

; BSET
; CHECK: bset 4               ; encoding: [0x48,0x94]  
; CHECK: bset 2               ; encoding: [0x28,0x94]  
; CHECK: bset 0               ; encoding: [0x08,0x94]  
; CHECK: bset 1               ; encoding: [0x18,0x94]  


; BCLR
; CHECK: bclr 4               ; encoding: [0xc8,0x94]  
; CHECK: bclr 2               ; encoding: [0xa8,0x94]  
; CHECK: bclr 0               ; encoding: [0x88,0x94]  
; CHECK: bclr 1               ; encoding: [0x98,0x94]  


; Carry (C flag)
; CHECK: sec                  ; encoding: [0x08,0x94]
; CHECK: clc                  ; encoding: [0x88,0x94]

; Negative (N flag)
; CHECK: sen                  ; encoding: [0x28,0x94]
; CHECK: cln                  ; encoding: [0xa8,0x94]

; Zero (Z flag)
; CHECK: sez                  ; encoding: [0x18,0x94]
; CHECK: clz                  ; encoding: [0x98,0x94]

; Interrupt (I flag)
; CHECK: sei                  ; encoding: [0x78,0x94]
; CHECK: cli                  ; encoding: [0xf8,0x94]

; Signed test (S flag)
; CHECK: ses                  ; encoding: [0x48,0x94]
; CHECK: cls                  ; encoding: [0xc8,0x94]

; Two's compliment overflow (V flag)
; CHECK: sev                  ; encoding: [0x38,0x94]
; CHECK: clv                  ; encoding: [0xb8,0x94]

; T (T flag)
; CHECK: set                  ; encoding: [0x68,0x94]
; CHECK: clt                  ; encoding: [0xe8,0x94]

; Half carry (H flag)
; CHECK: seh                  ; encoding: [0x58,0x94]
; CHECK: clh                  ; encoding: [0xd8,0x94]
