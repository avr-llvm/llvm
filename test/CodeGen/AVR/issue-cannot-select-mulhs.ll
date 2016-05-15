; RUN: llc < %s -march=avr | FileCheck %s
; XFAIL: *

define fastcc void @foo(i32) unnamed_addr {
; CHECK-LABEL: foo:
entry-block:
  br i1 undef, label %return, label %next

next:                                             ; preds = %entry-block
  %1 = trunc i32 %0 to i8
  br label %exit

exit:                                             ; preds = %match_case1, %next
  %result.0282 = phi i8 [ %5, %match_case1 ], [ 0, %next ]
  %2 = icmp ult i32 undef, %0
  br i1 %2, label %match_case0, label %return

match_case0:                                      ; preds = %exit
  %3 = tail call { i8, i1 } @llvm.smul.with.overflow.i8(i8 %result.0282, i8 %1)
  %4 = extractvalue { i8, i1 } %3, 1
  %brmerge = or i1 %4, undef
  br i1 %brmerge, label %return, label %match_case1

match_case1:                                      ; preds = %match_case0
  %5 = extractvalue { i8, i1 } undef, 0
  br label %exit

return:                                           ; preds = %match_case0, %exit, %entry-block
  ret void
}

; Function Attrs: nounwind readnone
declare { i8, i1 } @llvm.smul.with.overflow.i8(i8, i8)
