; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s

foo:

    ldi r24, lo8(0x42)
    ldi r24, lo8(0x2342)

    ldi r24, lo8(0x23)
    ldi r24, hi8(0x2342)

bar:

    ldi r24, lo8(bar)
    ldi r24, hi8(bar)

; CHECK: ldi	r24, lo8(66)            ; encoding: [0x82,0xe4]
; CHECK: ldi	r24, lo8(9026)          ; encoding: [0x82,0xe4]

; CHECK: ldi	r24, lo8(35)            ; encoding: [0x83,0xe2]
; CHECK: ldi	r24, hi8(9026)          ; encoding: [0x83,0xe2]

; CHECK: ldi	r24, lo8(bar)           ; encoding: [0x80'A',0xe0]
                                        ; fixup A - offset: 0, value: lo8(bar), kind: fixup_lo8_ldi
; CHECK: ldi	r24, hi8(bar)           ; encoding: [0x80'A',0xe0]
                                        ; fixup A - offset: 0, value: hi(bar), kind: fixup_hi8_ldi

