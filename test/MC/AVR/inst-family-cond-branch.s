; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s

; TODO: Add BRBS/BRBC instruction to each variant

foo:

  ; BREQ
  breq .-18
  breq .-12
  
  ; BRNE
  brne .+10
  brne .+2
  
  ; BRCS
  brcs .+8
  brcs .+4
  
  ; BRCC
  brcc .+66
  brcc .-22
  
  ; BRSH
  brsh .+32
  brsh .+70
  
  ; BRLO
  brlo .+12
  brlo .+28
  
  ; BRMI
  brmi .+66
  brmi .+58
  
  ; BRPL
  brpl .-12
  brpl .+18
  
  ; BRGE
  brge .+50
  brge .+42
  
  ; BRLT
  brlt .+16
  brlt .+2
  
  ; BRHS
  brhs .-66
  brhs .+14
  
  ; BRHC
  brhc .+12
  brhc .+14
  
  ; BRTS
  brts .+18
  brts .+22
  
  ; BRTC
  brtc .+52
  brtc .+50
  
  ; BRVS
  brvs .+18
  brvs .+32
  
  ; BRVC
  brvc .-28
  brvc .-62
  
  ; BRIE
  brie .+20
  brie .+40
  
  ; BRID
  brid .+42
  brid .+62
  

; BREQ
; CHECK: breq .-18                  ; encoding: [0xb9,0xf3]
; CHECK: breq .-12                  ; encoding: [0xd1,0xf3]

; BRNE
; CHECK: brne .+10                  ; encoding: [0x29,0xf4]
; CHECK: brne .+2                   ; encoding: [0x09,0xf4]

; BRCS
; CHECK: brbs 0, .+8                ; encoding: [0x20,0xf0]
; CHECK: brbs 0, .+4                ; encoding: [0x10,0xf0]

; BRCC
; CHECK: brbc 0, .+66               ; encoding: [0x08,0xf5]
; CHECK: brbc 0, .-22               ; encoding: [0xa8,0xf7]

; BRSH
; CHECK: brsh .+32                  ; encoding: [0x80,0xf4]
; CHECK: brsh .+70                  ; encoding: [0x18,0xf5]

; BRLO
; CHECK: brlo .+12                  ; encoding: [0x30,0xf0]
; CHECK: brlo .+28                  ; encoding: [0x70,0xf0]

; BRMI
; CHECK: brmi .+66                  ; encoding: [0x0a,0xf1]
; CHECK: brmi .+58                  ; encoding: [0xea,0xf0]

; BRPL
; CHECK: brpl .-12                  ; encoding: [0xd2,0xf7]
; CHECK: brpl .+18                  ; encoding: [0x4a,0xf4]

; BRGE
; CHECK: brge .+50                  ; encoding: [0xcc,0xf4]
; CHECK: brge .+42                  ; encoding: [0xac,0xf4]

; BRLT
; CHECK: brlt .+16                  ; encoding: [0x44,0xf0]
; CHECK: brlt .+2                   ; encoding: [0x0c,0xf0]

; BRHS
; CHECK: brbs 5, .-66               ; encoding: [0xfd,0xf2]
; CHECK: brbs 5, .+14               ; encoding: [0x3d,0xf0]

; BRHC
; CHECK: brbc 5, .+12                  ; encoding: [0x35,0xf4]
; CHECK: brbc 5, .+14                  ; encoding: [0x3d,0xf4]

; BRTS
; CHECK: brbs 6, .+18                  ; encoding: [0x4e,0xf0]
; CHECK: brbs 6, .+22                  ; encoding: [0x5e,0xf0]

; BRTC
; CHECK: brbc 6, .+52                  ; encoding: [0xd6,0xf4]
; CHECK: brbc 6, .+50                  ; encoding: [0xce,0xf4]

; BRVS
; CHECK: brbs 3, .+18                  ; encoding: [0x4b,0xf0]
; CHECK: brbs 3, .+32                  ; encoding: [0x83,0xf0]

; BRVC
; CHECK: brbc 3, .-28                  ; encoding: [0x93,0xf7]
; CHECK: brbc 3, .-62                  ; encoding: [0x0b,0xf7]

; BRIE
; CHECK: brbs 7, .+20                  ; encoding: [0x57,0xf0]
; CHECK: brbs 7, .+40                  ; encoding: [0xa7,0xf0]

; BRID
; CHECK: brbc 7, .+42                  ; encoding: [0xaf,0xf4]
; CHECK: brbc 7, .+62                  ; encoding: [0xff,0xf4]
