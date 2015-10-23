; RUN: llc < %s -asm-verbose=false | FileCheck %s

; Test the CFG stackifier pass.

target datalayout = "e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-unknown"

declare void @something()

; Test that loops are made contiguous, even in the presence of split backedges.

; CHECK-LABEL: test0
; CHECK: (loop
; CHECK: (add
; CHECK: (brif
; CHECK: (call
; CHECK: (br $BB0_1)
; CHECK: (return)
define void @test0(i32 %n) {
entry:
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %i.next, %back ]
  %i.next = add i32 %i, 1

  %c = icmp slt i32 %i.next, %n
  br i1 %c, label %back, label %exit

exit:
  ret void

back:
  call void @something()
  br label %header
}

; Same as test0, but the branch condition is reversed.

; CHECK-LABEL: test1
; CHECK: (loop
; CHECK: (add
; CHECK: (brif
; CHECK: (call
; CHECK: (br $BB1_1)
; CHECK: (return)
define void @test1(i32 %n) {
entry:
  br label %header

header:
  %i = phi i32 [ 0, %entry ], [ %i.next, %back ]
  %i.next = add i32 %i, 1

  %c = icmp sge i32 %i.next, %n
  br i1 %c, label %exit, label %back

exit:
  ret void

back:
  call void @something()
  br label %header
}

; Test that a simple loop is handled as expected.

; CHECK-LABEL: test2
; CHECK: (block $BB2_2)
; CHECK: (brif $BB2_2 {{.*}})
; CHECK: BB2_1:
; CHECK: (brif $BB2_1 @14)
; CHECK: BB2_2:
; CHECK: (return)
define void @test2(double* nocapture %p, i32 %n) {
entry:
  %cmp.4 = icmp sgt i32 %n, 0
  br i1 %cmp.4, label %for.body.preheader, label %for.end

for.body.preheader:
  br label %for.body

for.body:
  %i.05 = phi i32 [ %inc, %for.body ], [ 0, %for.body.preheader ]
  %arrayidx = getelementptr inbounds double, double* %p, i32 %i.05
  %0 = load double, double* %arrayidx, align 8
  %mul = fmul double %0, 3.200000e+00
  store double %mul, double* %arrayidx, align 8
  %inc = add nuw nsw i32 %i.05, 1
  %exitcond = icmp eq i32 %inc, %n
  br i1 %exitcond, label %for.end.loopexit, label %for.body

for.end.loopexit:
  br label %for.end

for.end:
  ret void
}

; CHECK-LABEL: doublediamond
; CHECK: (block $BB3_5)
; CHECK: (block $BB3_4)
; CHECK: (block $BB3_2)
; CHECK: (brif $BB3_2 @4)
; CHECK: (br $BB3_5)
; CHECK: BB3_2:
; CHECK: (brif $BB3_4 @7)
; CHECK: (br $BB3_5)
; CHECK: BB3_4:
; CHECK: BB3_5:
; CHECK: (return @3)
define i32 @doublediamond(i32 %a, i32 %b, i32* %p) {
entry:
  %c = icmp eq i32 %a, 0
  %d = icmp eq i32 %b, 0
  store volatile i32 0, i32* %p
  br i1 %c, label %true, label %false
true:
  store volatile i32 1, i32* %p
  br label %exit
false:
  store volatile i32 2, i32* %p
  br i1 %d, label %ft, label %ff
ft:
  store volatile i32 3, i32* %p
  br label %exit
ff:
  store volatile i32 4, i32* %p
  br label %exit
exit:
  store volatile i32 5, i32* %p
  ret i32 0
}

; CHECK-LABEL: triangle
; CHECK: (block $BB4_2)
; CHECK: (brif $BB4_2 @3)
; CHECK: BB4_2:
; CHECK: (return @2)
define i32 @triangle(i32* %p, i32 %a) {
entry:
  %c = icmp eq i32 %a, 0
  store volatile i32 0, i32* %p
  br i1 %c, label %true, label %exit
true:
  store volatile i32 1, i32* %p
  br label %exit
exit:
  store volatile i32 2, i32* %p
  ret i32 0
}

; CHECK-LABEL: diamond
; CHECK: (block $BB5_3)
; CHECK: (block $BB5_2)
; CHECK: (brif $BB5_2 @3)
; CHECK: (br $BB5_3)
; CHECK: BB5_2:
; CHECK: BB5_3:
; CHECK: (return @2)
define i32 @diamond(i32* %p, i32 %a) {
entry:
  %c = icmp eq i32 %a, 0
  store volatile i32 0, i32* %p
  br i1 %c, label %true, label %false
true:
  store volatile i32 1, i32* %p
  br label %exit
false:
  store volatile i32 2, i32* %p
  br label %exit
exit:
  store volatile i32 3, i32* %p
  ret i32 0
}

; CHECK-LABEL: single_block
; CHECK-NOT: br
; CHECK: (return @1)
define i32 @single_block(i32* %p) {
entry:
  store volatile i32 0, i32* %p
  ret i32 0
}

; CHECK-LABEL: minimal_loop
; CHECK-NOT: br
; CHECK: BB7_1:
; CHECK: (store_i32 @0 @2)
; CHECK: (br $BB7_1)
define i32 @minimal_loop(i32* %p) {
entry:
  store volatile i32 0, i32* %p
  br label %loop
loop:
  store volatile i32 1, i32* %p
  br label %loop
}

; CHECK-LABEL: simple_loop
; CHECK-NOT: br
; CHECK: BB8_1:
; CHECK: (loop $BB8_1)
; CHECK: (brif $BB8_1 @4)
; CHECK: (return @2)
define i32 @simple_loop(i32* %p, i32 %a) {
entry:
  %c = icmp eq i32 %a, 0
  store volatile i32 0, i32* %p
  br label %loop
loop:
  store volatile i32 1, i32* %p
  br i1 %c, label %loop, label %exit
exit:
  store volatile i32 2, i32* %p
  ret i32 0
}

; CHECK-LABEL: doubletriangle
; CHECK: (block $BB9_4)
; CHECK: (block $BB9_3)
; CHECK: (brif $BB9_4 @4)
; CHECK: (brif $BB9_3 @7)
; CHECK: BB9_3:
; CHECK: BB9_4:
; CHECK: (return @3)
define i32 @doubletriangle(i32 %a, i32 %b, i32* %p) {
entry:
  %c = icmp eq i32 %a, 0
  %d = icmp eq i32 %b, 0
  store volatile i32 0, i32* %p
  br i1 %c, label %true, label %exit
true:
  store volatile i32 2, i32* %p
  br i1 %d, label %tt, label %tf
tt:
  store volatile i32 3, i32* %p
  br label %tf
tf:
  store volatile i32 4, i32* %p
  br label %exit
exit:
  store volatile i32 5, i32* %p
  ret i32 0
}

; CHECK-LABEL: ifelse_earlyexits
; CHECK: (block $BB10_4)
; CHECK: (block $BB10_2)
; CHECK: (brif $BB10_2 @4)
; CHECK: (br $BB10_4)
; CHECK: BB10_2:
; CHECK: (brif $BB10_4 @7)
; CHECK: BB10_4:
; CHECK: (return @3)
define i32 @ifelse_earlyexits(i32 %a, i32 %b, i32* %p) {
entry:
  %c = icmp eq i32 %a, 0
  %d = icmp eq i32 %b, 0
  store volatile i32 0, i32* %p
  br i1 %c, label %true, label %false
true:
  store volatile i32 1, i32* %p
  br label %exit
false:
  store volatile i32 2, i32* %p
  br i1 %d, label %ft, label %exit
ft:
  store volatile i32 3, i32* %p
  br label %exit
exit:
  store volatile i32 4, i32* %p
  ret i32 0
}

; Test that nested loops are handled.

declare void @bar()

define void @test3(i32 %w)  {
entry:
  br i1 undef, label %outer.ph, label %exit

outer.ph:
  br label %outer

outer:
  %tobool = icmp eq i32 undef, 0
  br i1 %tobool, label %inner, label %unreachable

unreachable:
  unreachable

inner:
  %c = icmp eq i32 undef, %w
  br i1 %c, label %if.end, label %inner

exit:
  ret void

if.end:
  call void @bar()
  br label %outer
}
