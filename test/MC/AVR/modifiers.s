; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s

foo:

    ldi r24, lo8(0x42)
    ldi r24, lo8(0x2342)

    ldi r24, lo8(0x23)
    ldi r24, hi8(0x2342)

; CHECK: ldi  r24, lo8(66)          ; encoding: [0x82,0xe4]
; CHECK: ldi  r24, lo8(9026)        ; encoding: [0x82,0xe4]

; CHECK: ldi  r24, lo8(35)          ; encoding: [0x83,0xe2]
; CHECK: ldi  r24, hi8(9026)        ; encoding: [0x83,0xe2]


bar:

    ldi r24, lo8(bar)
    ldi r24, hi8(bar)

; CHECK: ldi  r24, lo8(bar)         ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: lo8(bar), kind: fixup_lo8_ldi
; CHECK: ldi  r24, hi8(bar)         ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: hi8(bar), kind: fixup_hi8_ldi

lo8:

    ldi r24, lo8(0x0815)
    ldi r24, lo8(foo)
    ldi r24, lo8(bar + 5)

; CHECK: ldi  r24, lo8(2069)        ; encoding: [0x85,0xe1]
; CHECK: ldi  r24, lo8(foo)         ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: lo8(foo), kind: fixup_lo8_ldi
; CHECK: ldi  r24, lo8(bar+5)       ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: lo8(bar+5), kind: fixup_lo8_ldi

hi8:

    ldi r24, hi8(0x0815)
    ldi r24, hi8(foo)
    ldi r24, hi8(bar + 5)

; CHECK: ldi  r24, hi8(2069)        ; encoding: [0x88,0xe0]
; CHECK: ldi  r24, hi8(foo)         ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: hi8(foo), kind: fixup_hi8_ldi
; CHECK: ldi  r24, hi8(bar+5)       ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: hi8(bar+5), kind: fixup_hi8_ldi

hh8:

    ldi r24, hh8(0x0815)
    ldi r24, hh8(foo)
    ldi r24, hh8(bar + 5)

; CHECK: ldi  r24, hh8(2069)        ; encoding: [0x80,0xe0]
; CHECK: ldi  r24, hh8(foo)         ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: hh8(foo), kind: fixup_hh8_ldi
; CHECK: ldi  r24, hh8(bar+5)       ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: hh8(bar+5), kind: fixup_hh8_ldi

hlo8: ; synonym with hh8() above, hence the... odd results

    ldi r24, hlo8(0x0815)
    ldi r24, hlo8(foo)
    ldi r24, hlo8(bar + 5)

; CHECK: ldi  r24, hh8(2069)        ; encoding: [0x80,0xe0]
; CHECK: ldi  r24, hh8(foo)         ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: hh8(foo), kind: fixup_hh8_ldi
; CHECK: ldi  r24, hh8(bar+5)       ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: hh8(bar+5), kind: fixup_hh8_ldi

hhi8:

    ldi r24, hhi8(0x0815)
    ldi r24, hhi8(foo)
    ldi r24, hhi8(bar + 5)

; CHECK: ldi  r24, hhi8(2069)       ; encoding: [0x80,0xe0]
; CHECK: ldi  r24, hhi8(foo)        ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: hhi8(foo), kind: fixup_ms8_ldi
; CHECK: ldi  r24, hhi8(bar+5)      ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: hhi8(bar+5), kind: fixup_ms8_ldi

pm_lo8:
    ldi r24, pm_lo8(0x0815)
    ldi r24, pm_lo8(foo)
    ldi r24, pm_lo8(bar + 5)

; CHECK: ldi  r24, pm_lo8(2069)     ; encoding: [0x8a,0xe0]
; CHECK: ldi  r24, pm_lo8(foo)      ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: pm_lo8(foo), kind: fixup_lo8_ldi_pm
; CHECK: ldi  r24, pm_lo8(bar+5)    ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: pm_lo8(bar+5), kind: fixup_lo8_ldi_pm

pm_hi8:
    ldi r24, pm_hi8(0x0815)
    ldi r24, pm_hi8(foo)
    ldi r24, pm_hi8(bar + 5)

; CHECK: ldi  r24, pm_hi8(2069)     ; encoding: [0x84,0xe0]
; CHECK: ldi  r24, pm_hi8(foo)      ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: pm_hi8(foo), kind: fixup_hi8_ldi_pm
; CHECK: ldi  r24, pm_hi8(bar+5)    ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: pm_hi8(bar+5), kind: fixup_hi8_ldi_pm

pm_hh8:
    ldi r24, pm_hh8(0x0815)
    ldi r24, pm_hh8(foo)
    ldi r24, pm_hh8(bar + 5)

; CHECK: ldi  r24, pm_hh8(2069)     ; encoding: [0x80,0xe0]
; CHECK: ldi  r24, pm_hh8(foo)      ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: pm_hh8(foo), kind: fixup_hh8_ldi_pm
; CHECK: ldi  r24, pm_hh8(bar+5)    ; encoding: [0x80'A',0xe0]
; CHECK:                            ; fixup A - offset: 0, value: pm_hh8(bar+5), kind: fixup_hh8_ldi_pm




