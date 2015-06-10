; RUN: llvm-mc -triple avr-none -show-encoding < %s | FileCheck %s


foo:

  rjmp  .+2
  rjmp  .-2
  rjmp  foo
  rjmp  .+8
  rjmp  end
  rjmp  .+0
end:
  rjmp .-4
  rjmp .-6

; CHECK: foo:
; CHECK: rjmp  .+2                   ; encoding: [0x01,0xc0]
; CHECK: rjmp  .-2                   ; encoding: [0xff,0xcf]
; CHECK: rjmp  foo                   ; encoding: [A,0b1100AAAA]
; CHECK: rjmp  .+8                   ; encoding: [0x04,0xc0]
; CHECK: rjmp  end                   ; encoding: [A,0b1100AAAA]
; CHECK: rjmp  .+0                   ; encoding: [0x00,0xc0]
; CHECJ: rjmp  .-4                   ; encoding: [0xfe,0xcf]
; CHECK: rjmp  .-6                   ; encoding: [0xfd,0xcf]
