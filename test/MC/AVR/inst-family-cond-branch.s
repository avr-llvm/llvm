; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  ; BREQ
  breq .-18
  breq .-12
  brbs 1, .-18
  brbs 1, baz
  
  ; BRNE
  brne .+10
  brne .+2
  brbc 1, .+10
  brbc 1, bar

bar:
  
  ; BRCS
  brcs .+8
  brcs .+4
  brbs 0, .+8
  brbs 0, end
  
  ; BRCC
  brcc .+66
  brcc .-22
  brbc 0, .+66
  brbc 0, baz
  
  ; BRSH
  brsh .+32
  brsh .+70
  brsh car

baz:

  ; BRLO
  brlo .+12
  brlo .+28
  brlo car
  
  ; BRMI
  brmi .+66
  brmi .+58
  brmi car
  
  ; BRPL
  brpl .-12
  brpl .+18
  brpl car
  
  ; BRGE
  brge .+50
  brge .+42
  brge car

car:
  
  ; BRLT
  brlt .+16
  brlt .+2
  brlt end
  
  ; BRHS
  brhs .-66
  brhs .+14
  brhs just_another_label
  
  ; BRHC
  brhc .+12
  brhc .+14
  brhc just_another_label
  
  ; BRTS
  brts .+18
  brts .+22
  brts just_another_label

just_another_label:
  
  ; BRTC
  brtc .+52
  brtc .+50
  brtc end
  
  ; BRVS
  brvs .+18
  brvs .+32
  brvs end
  
  ; BRVC
  brvc .-28
  brvc .-62
  brvc end
  
  ; BRIE
  brie .+20
  brie .+40
  brie end
  
  ; BRID
  brid .+42
  brid .+62
  brid end

end:
  

; BREQ
; CHECK: breq .-18                  ; encoding: [0xb9,0xf3]
; CHECK: breq .-12                  ; encoding: [0xd1,0xf3]
; CHECK: brbs 1, .-18               ; encoding: [0xb9,0xf3]
; CHECK: brbs 1, baz                ; encoding: [0bAAAAA001,0b111100AA]
; CHECK:                            ;   fixup A - offset: 0, value: baz, kind: fixup_7_pcrel

; BRNE
; CHECK: brne .+10                  ; encoding: [0x29,0xf4]
; CHECK: brne .+2                   ; encoding: [0x09,0xf4]
; CHECK: brbc 1, .+10               ; encoding: [0x29,0xf4]
; CHECK: brbc 1, bar                ; encoding: [0bAAAAA001,0b111101AA]
; CHECK:                            ;   fixup A - offset: 0, value: bar, kind: fixup_7_pcrel

; BRCS
; CHECK: brcs  .+8                  ; encoding: [0x20,0xf0]
; CHECK: brcs  .+4                  ; encoding: [0x10,0xf0]
; CHECK: brcs  .+8                  ; encoding: [0x20,0xf0]
; CHECK: brcs  end                  ; encoding: [0bAAAAA000,0b111100AA]
; CHECK:                            ;   fixup A - offset: 0, value: end, kind: fixup_7_pcrel

; BRCC
; CHECK: brcc .+66                  ; encoding: [0x08,0xf5]
; CHECK: brcc .-22                  ; encoding: [0xa8,0xf7]
; CHECK: brcc .+66                  ; encoding: [0x08,0xf5]
; CHECK: brcc baz                   ; encoding: [0bAAAAA000,0b111101AA]
; CHECK:                            ;   fixup A - offset: 0, value: baz, kind: fixup_7_pcrel

; BRSH
; CHECK: brsh .+32                  ; encoding: [0x80,0xf4]
; CHECK: brsh .+70                  ; encoding: [0x18,0xf5]
; CHECK: brsh car                   ; encoding: [0bAAAAA000,0b111101AA]
; CHECK:                            ;   fixup A - offset: 0, value: car, kind: fixup_7_pcrel

; BRLO
; CHECK: brlo .+12                  ; encoding: [0x30,0xf0]
; CHECK: brlo .+28                  ; encoding: [0x70,0xf0]
; CHECK: brlo car                   ; encoding: [0bAAAAA000,0b111100AA]
; CHECK:                            ;   fixup A - offset: 0, value: car, kind: fixup_7_pcrel

; BRMI
; CHECK: brmi .+66                  ; encoding: [0x0a,0xf1]
; CHECK: brmi .+58                  ; encoding: [0xea,0xf0]
; CHECK: brmi car                   ; encoding: [0bAAAAA010,0b111100AA]
; CHECK:                            ;   fixup A - offset: 0, value: car, kind: fixup_7_pcrel

; BRPL
; CHECK: brpl .-12                  ; encoding: [0xd2,0xf7]
; CHECK: brpl .+18                  ; encoding: [0x4a,0xf4]
; CHECK: brpl car                   ; encoding: [0bAAAAA010,0b111101AA]
; CHECK:                            ;   fixup A - offset: 0, value: car, kind: fixup_7_pcrel

; BRGE
; CHECK: brge .+50                  ; encoding: [0xcc,0xf4]
; CHECK: brge .+42                  ; encoding: [0xac,0xf4]
; CHECK: brge car                   ; encoding: [0bAAAAA100,0b111101AA]
; CHECK:                            ;   fixup A - offset: 0, value: car, kind: fixup_7_pcrel

; BRLT
; CHECK: brlt .+16                  ; encoding: [0x44,0xf0]
; CHECK: brlt .+2                   ; encoding: [0x0c,0xf0]
; CHECK: brlt end                   ; encoding: [0bAAAAA100,0b111100AA]
; CHECK:                            ;   fixup A - offset: 0, value: end, kind: fixup_7_pcrel

; BRHS
; CHECK: brhs .-66                  ; encoding: [0xfd,0xf2]
; CHECK: brhs .+14                  ; encoding: [0x3d,0xf0]
; CHECK: brhs just_another_label    ; encoding: [0bAAAAA101,0b111100AA]
; CHECK:                            ;   fixup A - offset: 0, value: just_another_label, kind: fixup_7_pcrel

; BRHC
; CHECK: brhc .+12                  ; encoding: [0x35,0xf4]
; CHECK: brhc .+14                  ; encoding: [0x3d,0xf4]
; CHECK: brhc just_another_label    ; encoding: [0bAAAAA101,0b111101AA]
; CHECK:                            ;   fixup A - offset: 0, value: just_another_label, kind: fixup_7_pcrel

; BRTS
; CHECK: brts .+18                  ; encoding: [0x4e,0xf0]
; CHECK: brts .+22                  ; encoding: [0x5e,0xf0]
; CHECK: brts just_another_label    ; encoding: [0bAAAAA110,0b111100AA]
; CHECK:                            ;   fixup A - offset: 0, value: just_another_label, kind: fixup_7_pcrel

; BRTC
; CHECK: brtc .+52                  ; encoding: [0xd6,0xf4]
; CHECK: brtc .+50                  ; encoding: [0xce,0xf4]
; CHECK: brtc end                   ; encoding: [0bAAAAA110,0b111101AA]
; CHECK:                            ;   fixup A - offset: 0, value: end, kind: fixup_7_pcrel

; BRVS
; CHECK: brvs .+18                  ; encoding: [0x4b,0xf0]
; CHECK: brvs .+32                  ; encoding: [0x83,0xf0]
; CHECK: brvs end                   ; encoding: [0bAAAAA011,0b111100AA]
; CHECK:                            ;   fixup A - offset: 0, value: end, kind: fixup_7_pcrel

; BRVC
; CHECK: brvc .-28                  ; encoding: [0x93,0xf7]
; CHECK: brvc .-62                  ; encoding: [0x0b,0xf7]
; CHECK: brvc end                   ; encoding: [0bAAAAA011,0b111101AA]
; CHECK:                            ;   fixup A - offset: 0, value: end, kind: fixup_7_pcrel

; BRIE
; CHECK: brie .+20                  ; encoding: [0x57,0xf0]
; CHECK: brie .+40                  ; encoding: [0xa7,0xf0]
; CHECK: brie end                   ; encoding: [0bAAAAA111,0b111100AA]
; CHECK:                            ;   fixup A - offset: 0, value: end, kind: fixup_7_pcrel

; BRID
; CHECK: brid .+42                  ; encoding: [0xaf,0xf4]
; CHECK: brid .+62                  ; encoding: [0xff,0xf4]
; CHECK: brid end                   ; encoding: [0bAAAAA111,0b111101AA]
; CHECK:                            ;   fixup A - offset: 0, value: end, kind: fixup_7_pcrel
