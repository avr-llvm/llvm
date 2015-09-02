; RUN: opt -mtriple=x86_x64-pc-windows-msvc -S -winehprepare  < %s | FileCheck %s

declare i32 @__CxxFrameHandler3(...)

declare void @f()
declare i32 @g()
declare void @h(i32)
declare i1 @b()


define void @test1() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  ; %x def colors: {entry} subset of use colors; must spill
  %x = call i32 @g()
  invoke void @f()
    to label %noreturn unwind label %catch
catch:
  catchpad []
    to label %noreturn unwind label %endcatch
noreturn:
  ; %x use colors: {entry, cleanup}
  call void @h(i32 %x)
  unreachable
endcatch:
  catchendpad unwind to caller
}
; Need two copies of the call to @h, one under entry and one under catch.
; Currently we generate a load for each, though we shouldn't need one
; for the use in entry's copy.
; CHECK-LABEL: @test1(
; CHECK: entry:
; CHECK:   store i32 %x, i32* [[Slot:%[^ ]+]]
; CHECK:   invoke void @f()
; CHECK:     to label %[[EntryCopy:[^ ]+]] unwind label %catch
; CHECK: catch:
; CHECK:   catchpad [] to label %[[CatchCopy:[^ ]+]] unwind
; CHECK: [[CatchCopy]]:
; CHECK:   [[LoadX2:%[^ ]+]] = load i32, i32* [[Slot]]
; CHECK:   call void @h(i32 [[LoadX2]]
; CHECK: [[EntryCopy]]:
; CHECK:   [[LoadX1:%[^ ]+]] = load i32, i32* [[Slot]]
; CHECK:   call void @h(i32 [[LoadX1]]


define void @test2() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  invoke void @f()
    to label %exit unwind label %cleanup
cleanup:
  cleanuppad []
  br label %exit
exit:
  call void @f()
  ret void
}
; Need two copies of %exit's call to @f -- the subsequent ret is only
; valid when coming from %entry, but on the path from %cleanup, this
; might be a valid call to @f which might dynamically not return.
; CHECK-LABEL: @test2(
; CHECK: entry:
; CHECK:   invoke void @f()
; CHECK:     to label %[[exit:[^ ]+]] unwind label %cleanup
; CHECK: cleanup:
; CHECK:   cleanuppad []
; CHECK:   call void @f()
; CHECK-NEXT: unreachable
; CHECK: [[exit]]:
; CHECK:   call void @f()
; CHECK-NEXT: ret void


define void @test3() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  invoke void @f()
    to label %invoke.cont unwind label %catch
invoke.cont:
  invoke void @f()
    to label %exit unwind label %cleanup
catch:
  catchpad [] to label %shared unwind label %endcatch
endcatch:
  catchendpad unwind to caller
cleanup:
  cleanuppad []
  br label %shared
shared:
  call void @f()
  br label %exit
exit:
  ret void
}
; Need two copies of %shared's call to @f (similar to @test2 but
; the two regions here are siblings, not parent-child).
; CHECK-LABEL: @test3(
; CHECK:   invoke void @f()
; CHECK:   invoke void @f()
; CHECK:     to label %[[exit:[^ ]+]] unwind
; CHECK: catch:
; CHECK:   catchpad [] to label %[[shared:[^ ]+]] unwind
; CHECK: cleanup:
; CHECK:   cleanuppad []
; CHECK:   call void @f()
; CHECK-NEXT: unreachable
; CHECK: [[shared]]:
; CHECK:   call void @f()
; CHECK-NEXT: unreachable
; CHECK: [[exit]]:
; CHECK:   ret void


define void @test4() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  invoke void @f()
    to label %shared unwind label %catch
catch:
  catchpad []
    to label %shared unwind label %endcatch
endcatch:
  catchendpad unwind to caller
shared:
  %x = call i32 @g()
  %i = call i32 @g()
  %zero.trip = icmp eq i32 %i, 0
  br i1 %zero.trip, label %exit, label %loop
loop:
  %i.loop = phi i32 [ %i, %shared ], [ %i.dec, %loop.tail ]
  %b = call i1 @b()
  br i1 %b, label %left, label %right
left:
  %y = call i32 @g()
  br label %loop.tail
right:
  call void @h(i32 %x)
  br label %loop.tail
loop.tail:
  %i.dec = sub i32 %i.loop, 1
  %done = icmp eq i32 %i.dec, 0
  br i1 %done, label %exit, label %loop
exit:
  call void @h(i32 %x)
  unreachable
}
; Make sure we can clone regions that have internal control
; flow and SSA values.  Here we need two copies of everything
; from %shared to %exit.
; CHECK-LABEL: @test4(
; CHECK:  entry:
; CHECK:    to label %[[shared_E:[^ ]+]] unwind label %catch
; CHECK:  catch:
; CHECK:    to label %[[shared_C:[^ ]+]] unwind label %endcatch
; CHECK:  [[shared_C]]:
; CHECK:    [[x_C:%[^ ]+]] = call i32 @g()
; CHECK:    [[i_C:%[^ ]+]] = call i32 @g()
; CHECK:    [[zt_C:%[^ ]+]] = icmp eq i32 [[i_C]], 0
; CHECK:    br i1 [[zt_C]], label %[[exit_C:[^ ]+]], label %[[loop_C:[^ ]+]]
; CHECK:  [[shared_E]]:
; CHECK:    [[x_E:%[^ ]+]] = call i32 @g()
; CHECK:    [[i_E:%[^ ]+]] = call i32 @g()
; CHECK:    [[zt_E:%[^ ]+]] = icmp eq i32 [[i_E]], 0
; CHECK:    br i1 [[zt_E]], label %[[exit_E:[^ ]+]], label %[[loop_E:[^ ]+]]
; CHECK:  [[loop_C]]:
; CHECK:    [[iloop_C:%[^ ]+]] = phi i32 [ [[i_C]], %[[shared_C]] ], [ [[idec_C:%[^ ]+]], %[[looptail_C:[^ ]+]] ]
; CHECK:    [[b_C:%[^ ]+]] = call i1 @b()
; CHECK:    br i1 [[b_C]], label %[[left_C:[^ ]+]], label %[[right_C:[^ ]+]]
; CHECK:  [[loop_E]]:
; CHECK:    [[iloop_E:%[^ ]+]] = phi i32 [ [[i_E]], %[[shared_E]] ], [ [[idec_E:%[^ ]+]], %[[looptail_E:[^ ]+]] ]
; CHECK:    [[b_E:%[^ ]+]] = call i1 @b()
; CHECK:    br i1 [[b_E]], label %[[left_E:[^ ]+]], label %[[right_E:[^ ]+]]
; CHECK:  [[left_C]]:
; CHECK:    [[y_C:%[^ ]+]] = call i32 @g()
; CHECK:    br label %[[looptail_C]]
; CHECK:  [[left_E]]:
; CHECK:    [[y_E:%[^ ]+]] = call i32 @g()
; CHECK:    br label %[[looptail_E]]
; CHECK:  [[right_C]]:
; CHECK:    call void @h(i32 [[x_C]])
; CHECK:    br label %[[looptail_C]]
; CHECK:  [[right_E]]:
; CHECK:    call void @h(i32 [[x_E]])
; CHECK:    br label %[[looptail_E]]
; CHECK:  [[looptail_C]]:
; CHECK:    [[idec_C]] = sub i32 [[iloop_C]], 1
; CHECK:    [[done_C:%[^ ]+]] = icmp eq i32 [[idec_C]], 0
; CHECK:    br i1 [[done_C]], label %[[exit_C]], label %[[loop_C]]
; CHECK:  [[looptail_E]]:
; CHECK:    [[idec_E]] = sub i32 [[iloop_E]], 1
; CHECK:    [[done_E:%[^ ]+]] = icmp eq i32 [[idec_E]], 0
; CHECK:    br i1 [[done_E]], label %[[exit_E]], label %[[loop_E]]
; CHECK:  [[exit_C]]:
; CHECK:    call void @h(i32 [[x_C]])
; CHECK:    unreachable
; CHECK:  [[exit_E]]:
; CHECK:    call void @h(i32 [[x_E]])
; CHECK:    unreachable


define void @test5() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  invoke void @f()
    to label %exit unwind label %outer
outer:
  %o = cleanuppad []
  %x = call i32 @g()
  invoke void @f()
    to label %outer.ret unwind label %inner
inner:
  %i = catchpad []
    to label %inner.catch unwind label %inner.endcatch
inner.catch:
  catchret %i to label %outer.post-inner
inner.endcatch:
  catchendpad unwind to caller
outer.post-inner:
  call void @h(i32 %x)
  br label %outer.ret
outer.ret:
  cleanupret %o unwind to caller
exit:
  ret void
}
; Simple nested case (catch-inside-cleanup).  Nothing needs
; to be cloned.  The def and use of %x are both in %outer
; and so don't need to be spilled.
; CHECK-LABEL: @test5(
; CHECK:      outer:
; CHECK:        %x = call i32 @g()
; CHECK-NEXT:   invoke void @f()
; CHECK-NEXT:     to label %outer.ret unwind label %inner
; CHECK:      inner:
; CHECK:          to label %inner.catch unwind label %inner.endcatch
; CHECK:      inner.catch:
; CHECK-NEXT:   catchret %i to label %outer.post-inner
; CHECK:      outer.post-inner:
; CHECK-NEXT:   call void @h(i32 %x)
; CHECK-NEXT:   br label %outer.ret


define void @test6() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  invoke void @f()
    to label %invoke.cont unwind label %left
invoke.cont:
  invoke void @f()
    to label %exit unwind label %right
left:
  cleanuppad []
  br label %shared
right:
  catchpad []
    to label %right.catch unwind label %right.end
right.catch:
  br label %shared
right.end:
  catchendpad unwind to caller
shared:
  %x = call i32 @g()
  invoke void @f()
    to label %shared.cont unwind label %inner
shared.cont:
  unreachable
inner:
  %i = cleanuppad []
  call void @h(i32 %x)
  cleanupret %i unwind label %right.end
exit:
  ret void
}
; %inner is a cleanup which appears both as a child of
; %left and as a child of %right.  Since statically we
; need each funclet to have a single parent, we need to
; clone the entire %inner funclet so we can have one
; copy under each parent.  The cleanupret in %inner
; unwinds to the catchendpad for %right, so the copy
; of %inner under %right should include it; the copy
; of %inner under %left should instead have an
; `unreachable` inserted there, but the copy under
; %left still needs to be created because it's possible
; the dynamic path enters %left, then enters %inner,
; then calls @h, and that the call to @h doesn't return.
; CHECK-LABEL: @test6(
; TODO: CHECKs


define void @test7() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  invoke void @f()
    to label %invoke.cont unwind label %left
invoke.cont:
  invoke void @f()
    to label %unreachable unwind label %right
left:
  cleanuppad []
  invoke void @f() to label %unreachable unwind label %inner
right:
  catchpad []
    to label %right.catch unwind label %right.end
right.catch:
  invoke void @f() to label %unreachable unwind label %inner
right.end:
  catchendpad unwind to caller
inner:
  %i = cleanuppad []
  %x = call i32 @g()
  call void @h(i32 %x)
  cleanupret %i unwind label %right.end
unreachable:
  unreachable
}
; Another case of a two-parent child (like @test6), this time
; with the join at the entry itself instead of following a
; non-pad join.
; CHECK-LABEL: @test7(
; TODO: CHECKs


define void @test8() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  invoke void @f()
    to label %invoke.cont unwind label %left
invoke.cont:
  invoke void @f()
    to label %unreachable unwind label %right
left:
  cleanuppad []
  br label %shared
right:
  catchpad []
    to label %right.catch unwind label %right.end
right.catch:
  br label %shared
right.end:
  catchendpad unwind to caller
shared:
  invoke void @f()
    to label %unreachable unwind label %inner
inner:
  cleanuppad []
  invoke void @f()
    to label %unreachable unwind label %inner.child
inner.child:
  cleanuppad []
  %x = call i32 @g()
  call void @h(i32 %x)
  unreachable
unreachable:
  unreachable
}
; %inner is a two-parent child which itself has a child; need
; to make two copies of both the %inner and %inner.child.
; CHECK-LABEL: @test8(
; TODO: CHECKs


define void @test9() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  invoke void @f()
    to label %invoke.cont unwind label %left
invoke.cont:
  invoke void @f()
    to label %unreachable unwind label %right
left:
  cleanuppad []
  call void @h(i32 1)
  invoke void @f()
    to label %unreachable unwind label %right
right:
  cleanuppad []
  call void @h(i32 2)
  invoke void @f()
    to label %unreachable unwind label %left
unreachable:
  unreachable
}
; This is an irreducible loop with two funclets that enter each other;
; need to make two copies of each funclet (one a child of root, the
; other a child of the opposite funclet), but also make sure not to
; clone self-descendants (if we tried to do that we'd need to make an
; infinite number of them).  Presumably if optimizations ever generated
; such a thing it would mean that one of the two cleanups was originally
; the parent of the other, but that we'd somehow lost track in the CFG
; of which was which along the way; generating each possibility lets
; whichever case was correct execute correctly.
; CHECK-LABEL: @test9(
; TODO: CHECKs
