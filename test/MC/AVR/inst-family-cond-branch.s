; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s

; TODO: Add BRBS/BRBC instruction to each variant

foo:

  ; BREQ
  breq -18
  breq -12
  
  ; BRNE
  brne 10
  brne 2
  
  ; BRCS
  brcs 8
  brcs 4
  
  ; BRCC
  brcc 66
  brcc -22
  
  ; BRSH
  brsh 32
  brsh 70
  
  ; BRLO
  brlo 12
  brlo 28
  
  ; BRMI
  brmi 66
  brmi 58
  
  ; BRPL
  brpl -12
  brpl 18
  
  ; BRGE
  brge 50
  brge 42
  
  ; BRLT
  brlt 16
  brlt 2
  
  ; BRHS
  brhs -66
  brhs 14
  
  ; BRHC
  brhc 12
  brhc 14
  
  ; BRTS
  brts 18
  brts 22
  
  ; BRTC
  brtc 52
  brtc 50
  
  ; BRVS
  brvs 18
  brvs 32
  
  ; BRVC
  brvc -28
  brvc -62
  
  ; BRIE
  brie 20
  brie 40
  
  ; BRID
  brid 42
  brid 62
  

; BREQ
; CHECK: breq -18                  ; encoding: [0xb1,0xf3]
; CHECK: breq -12                  ; encoding: [0xc1,0xf3]

; BRNE
; CHECK: brne 10                   ; encoding: [0x11,0xf4]
; CHECK: brne 2                    ; encoding: [0xe9,0xf7]

; BRCS
; CHECK: brcs 8                    ; encoding: [0xf8,0xf3]
; CHECK: brcs 4                    ; encoding: [0xe0,0xf3]

; BRCC
; CHECK: brcc 66                   ; encoding: [0xd0,0xf4]
; CHECK: brcc -22                  ; encoding: [0x68,0xf7]

; BRSH
; CHECK: brsh 32                   ; encoding: [0x38,0xf4]
; CHECK: brsh 70                   ; encoding: [0xc8,0xf4]

; BRLO
; CHECK: brlo 12                   ; encoding: [0xd8,0xf3]
; CHECK: brlo 28                   ; encoding: [0x10,0xf0]

; BRMI
; CHECK: brmi 66                   ; encoding: [0xa2,0xf0]
; CHECK: brmi 58                   ; encoding: [0x7a,0xf0]

; BRPL
; CHECK: brpl -12                  ; encoding: [0x5a,0xf7]
; CHECK: brpl 18                   ; encoding: [0xca,0xf7]

; BRGE
; CHECK: brge 50                   ; encoding: [0x44,0xf4]
; CHECK: brge 42                   ; encoding: [0x1c,0xf4]

; BRLT
; CHECK: brlt 16                   ; encoding: [0xac,0xf3]
; CHECK: brlt 2                    ; encoding: [0x6c,0xf3]

; BRHS
; CHECK: brhs -66                  ; encoding: [0x55,0xf2]
; CHECK: brhs 14                   ; encoding: [0x8d,0xf3]

; BRHC
; CHECK: brhc 12                   ; encoding: [0x7d,0xf7]
; CHECK: brhc 14                   ; encoding: [0x7d,0xf7]

; BRTS
; CHECK: brts 18                   ; encoding: [0x86,0xf3]
; CHECK: brts 22                   ; encoding: [0x8e,0xf3]

; BRTC
; CHECK: brtc 52                   ; encoding: [0xfe,0xf7]
; CHECK: brtc 50                   ; encoding: [0xee,0xf7]

; BRVS
; CHECK: brvs 18                   ; encoding: [0x63,0xf3]
; CHECK: brvs 32                   ; encoding: [0x93,0xf3]

; BRVC
; CHECK: brvc -28                  ; encoding: [0x9b,0xf6]
; CHECK: brvc -62                  ; encoding: [0x0b,0xf6]

; BRIE
; CHECK: brie 20                   ; encoding: [0x4f,0xf3]
; CHECK: brie 40                   ; encoding: [0x97,0xf3]

; BRID
; CHECK: brid 42                   ; encoding: [0x97,0xf7]
; CHECK: brid 62                   ; encoding: [0xdf,0xf7]
