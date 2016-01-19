; RUN: llc < %s -march=avr | FileCheck %s
; XFAIL:

declare i16 @llvm.bswap.i16(i16)

define i16 @foo(i8, i8, i8) {
entry-block:
  %3 = zext i8 %2 to i16
  %4 = add i16 0, %3
  %sret_slot.sroa.0.0.insert.insert = tail call i16 @llvm.bswap.i16(i16 %4)
  ret i16 %sret_slot.sroa.0.0.insert.insert
}
