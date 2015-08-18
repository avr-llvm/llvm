; RUN: opt -mtriple=x86_x64-pc-windows-msvc -S -winehprepare  < %s | FileCheck %s

declare i32 @__CxxFrameHandler3(...)

declare void @f()

declare i32 @g()

declare void @h(i32)

declare i1 @i()

; CHECK-LABEL: @test1(
define void @test1(i1 %B) personality i32 (...)* @__CxxFrameHandler3 {
entry:
  ; Spill slot should be inserted here
  ; CHECK: [[Slot:%[^ ]+]] = alloca
  ; Can't store for %phi at these defs because the lifetimes overlap
  ; CHECK-NOT: store
  %x = call i32 @g()
  %y = call i32 @g()
  br i1 %B, label %left, label %right
left:
  ; CHECK: left:
  ; CHECK-NEXT: store i32 %x, i32* [[Slot]]
  ; CHECK-NEXT: invoke void @f
  invoke void @f()
          to label %exit unwind label %merge
right:
  ; CHECK: right:
  ; CHECK-NEXT: store i32 %y, i32* [[Slot]]
  ; CHECK-NEXT: invoke void @f
  invoke void @f()
          to label %exit unwind label %merge
merge:
  ; CHECK: merge:
  ; CHECK-NOT: = phi
  %phi = phi i32 [ %x, %left ], [ %y, %right ]
  catchpad void [] to label %catch unwind label %catchend

catch:
  ; CHECK: catch:
  ; CHECK: [[Reload:%[^ ]+]] = load i32, i32* [[Slot]]
  ; CHECK-NEXT: call void @h(i32 [[Reload]])
  call void @h(i32 %phi)
  catchret void to label %exit

catchend:
  catchendpad unwind to caller

exit:
  ret void
}

; CHECK-LABEL: @test2(
define void @test2(i1 %B) personality i32 (...)* @__CxxFrameHandler3 {
entry:
  br i1 %B, label %left, label %right
left:
  ; Need two stores here because %x and %y interfere so they need 2 slots
  ; CHECK: left:
  ; CHECK:   store i32 1, i32* [[Slot1:%[^ ]+]]
  ; CHECK:   store i32 1, i32* [[Slot2:%[^ ]+]]
  ; CHECK-NEXT: invoke void @f
  invoke void @f()
          to label %exit unwind label %merge.inner
right:
  ; Need two stores here because %x and %y interfere so they need 2 slots
  ; CHECK: right:
  ; CHECK-DAG:   store i32 2, i32* [[Slot1]]
  ; CHECK-DAG:   store i32 2, i32* [[Slot2]]
  ; CHECK: invoke void @f
  invoke void @f()
          to label %exit unwind label %merge.inner
merge.inner:
  ; CHECK: merge.inner:
  ; CHECK-NOT: = phi
  ; CHECK: catchpad void
  %x = phi i32 [ 1, %left ], [ 2, %right ]
  catchpad void [] to label %catch.inner unwind label %catchend.inner

catch.inner:
  ; Need just one store here because only %y is affected
  ; CHECK: catch.inner:
  %z = call i32 @g()
  ; CHECK:   store i32 %z
  ; CHECK-NEXT: invoke void @f
  invoke void @f()
          to label %catchret.inner unwind label %merge.outer

catchret.inner:
  catchret void to label %exit
catchend.inner:
  catchendpad unwind label %merge.outer

merge.outer:
  ; CHECK: merge.outer:
  ; CHECK-NOT: = phi
  ; CHECK: catchpad void
  %y = phi i32 [ %x, %catchend.inner ], [ %z, %catch.inner ]
  catchpad void [] to label %catch.outer unwind label %catchend.outer

catchend.outer:
  catchendpad unwind to caller

catch.outer:
  ; Need to load x and y from two different slots since they're both live
  ; and can have different values (if we came from catch.inner)
  ; CHECK: catch.outer:
  ; CHECK-DAG: load i32, i32* [[Slot1]]
  ; CHECK-DAG: load i32, i32* [[Slot2]]
  ; CHECK: catchret void to label
  call void @h(i32 %x)
  call void @h(i32 %y)
  catchret void to label %exit

exit:
  ret void
}

; CHECK-LABEL: @test3(
define void @test3(i1 %B) personality i32 (...)* @__CxxFrameHandler3 {
entry:
  ; need to spill parameter %B and def %x since they're used in a funclet
  ; CHECK: entry:
  ; CHECK-DAG: store i1 %B, i1* [[SlotB:%[^ ]+]]
  ; CHECK-DAG: store i32 %x, i32* [[SlotX:%[^ ]+]]
  ; CHECK: invoke void @f
  %x = call i32 @g()
  invoke void @f()
          to label %exit unwind label %catchpad

catchpad:
  catchpad void [] to label %catch unwind label %catchend

catch:
  ; Need to reload %B here
  ; CHECK: catch:
  ; CHECK: [[ReloadB:%[^ ]+]] = load i1, i1* [[SlotB]]
  ; CHECK: br i1 [[ReloadB]]
  br i1 %B, label %left, label %right
left:
  ; Use of %x is in a phi, so need reload here in pred
  ; CHECK: left:
  ; CHECK: [[ReloadX:%[^ ]+]] = load i32, i32* [[SlotX]]
  ; CHECK: br label %merge
  br label %merge
right:
  br label %merge
merge:
  ; CHECK: merge:
  ; CHECK:   %phi = phi i32 [ [[ReloadX]], %left ]
  %phi = phi i32 [ %x, %left ], [ 42, %right ]
  call void @h(i32 %phi)
  catchret void to label %exit

catchend:
  catchendpad unwind to caller

exit:
  ret void
}

; test4: don't need stores for %phi.inner, as its only use is to feed %phi.outer
;        %phi.outer needs stores in %left, %right, and %join
; CHECK-LABEL: @test4(
define void @test4(i1 %B) personality i32 (...)* @__CxxFrameHandler3 {
entry:
  ; CHECK:      entry:
  ; CHECK:        [[Slot:%[^ ]+]] = alloca
  ; CHECK-NEXT:   br
  br i1 %B, label %left, label %right
left:
  ; CHECK: left:
  ; CHECK-NOT: store
  ; CHECK: store i32 %l, i32* [[Slot]]
  ; CHECK-NEXT: invoke void @f
  %l = call i32 @g()
  invoke void @f()
          to label %join unwind label %catchpad.inner
right:
  ; CHECK: right:
  ; CHECK-NOT: store
  ; CHECK: store i32 %r, i32* [[Slot]]
  ; CHECK-NEXT: invoke void @f
  %r = call i32 @g()
  invoke void @f()
          to label %join unwind label %catchpad.inner
catchpad.inner:
   ; CHECK: catchpad.inner:
   ; CHECK-NEXT: catchpad void
   %phi.inner = phi i32 [ %l, %left ], [ %r, %right ]
   catchpad void [] to label %catch.inner unwind label %catchend.inner
catch.inner:
   catchret void to label %join
catchend.inner:
   catchendpad unwind label  %catchpad.outer
join:
  ; CHECK: join:
  ; CHECK-NOT: store
  ; CHECK: store i32 %j, i32* [[Slot]]
  ; CHECK-NEXT: invoke void @f
   %j = call i32 @g()
   invoke void @f()
           to label %exit unwind label %catchpad.outer
catchpad.outer:
   ; CHECK: catchpad.outer:
   ; CHECK-NEXT: catchpad void
   %phi.outer = phi i32 [ %phi.inner, %catchend.inner ], [ %j, %join ]
   catchpad void [] to label %catch.outer unwind label %catchend.outer
catch.outer:
   ; CHECK: catch.outer:
   ; CHECK:   [[Reload:%[^ ]+]] = load i32, i32* [[Slot]]
   ; CHECK:   call void @h(i32 [[Reload]])
   call void @h(i32 %phi.outer)
   catchret void to label %exit
catchend.outer:
   catchendpad unwind to caller
exit:
   ret void
}

; CHECK-LABEL: @test5(
define void @test5() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  ; need store for %phi.cleanup
  ; CHECK:      entry:
  ; CHECK:        store i32 1, i32* [[CleanupSlot:%[^ ]+]]
  ; CHECK-NEXT:   invoke void @f
  invoke void @f()
          to label %invoke.cont unwind label %cleanup

invoke.cont:
  ; need store for %phi.cleanup
  ; CHECK:      invoke.cont:
  ; CHECK-NEXT:   store i32 2, i32* [[CleanupSlot]]
  ; CHECK-NEXT:   invoke void @f
  invoke void @f()
          to label %invoke.cont2 unwind label %cleanup

cleanup:
  ; cleanup phi can be loaded at cleanup entry
  ; CHECK: cleanup:
  ; CHECK-NEXT: cleanuppad void
  ; CHECK: [[CleanupReload:%[^ ]+]] = load i32, i32* [[CleanupSlot]]
  %phi.cleanup = phi i32 [ 1, %entry ], [ 2, %invoke.cont ]
  cleanuppad void []
  %b = call i1 @i()
  br i1 %b, label %left, label %right

left:
  ; CHECK: left:
  ; CHECK:   call void @h(i32 [[CleanupReload]]
  call void @h(i32 %phi.cleanup)
  br label %merge

right:
  ; CHECK: right:
  ; CHECK:   call void @h(i32 [[CleanupReload]]
  call void @h(i32 %phi.cleanup)
  br label %merge

merge:
  ; need store for %phi.catch
  ; CHECK:      merge:
  ; CHECK-NEXT:   store i32 [[CleanupReload]], i32* [[CatchSlot:%[^ ]+]]
  ; CHECK-NEXT:   cleanupret void
  cleanupret void unwind label %catchpad

invoke.cont2:
  ; need store for %phi.catch
  ; CHECK:      invoke.cont2:
  ; CHECK-NEXT:   store i32 3, i32* [[CatchSlot]]
  ; CHECK-NEXT:   invoke void @f
  invoke void @f()
          to label %exit unwind label %catchpad

catchpad:
  ; CHECK: catchpad:
  ; CHECK-NEXT: catchpad void
  %phi.catch = phi i32 [ %phi.cleanup, %merge ], [ 3, %invoke.cont2 ]
  catchpad void [] to label %catch unwind label %catchend

catch:
  ; CHECK: catch:
  ; CHECK:   [[CatchReload:%[^ ]+]] = load i32, i32* [[CatchSlot]]
  ; CHECK:   call void @h(i32 [[CatchReload]]
  call void @h(i32 %phi.catch)
  catchret void to label %exit

catchend:
  catchendpad unwind to caller

exit:
  ret void
}

; CHECK-LABEL: @test6(
define void @test6() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  ; Since %x needs to be stored but the edge to loop is critical,
  ; it needs to be split
  ; CHECK: entry:
  ; CHECK: invoke i32 @g
  ; CHECK-NEXT: to label %[[SplitBlock:[^ ]+]] unwind label %to_caller
  %x = invoke i32 @g()
          to label %loop unwind label %to_caller
  ; The store should be in the split block
  ; CHECK: [[SplitBlock]]:
  ; CHECK: store i32 %x, i32* [[SpillSlot:%[^ ]+]]
  ; CHECK: br label %loop
to_caller:
  cleanuppad void []
  cleanupret void unwind to caller
loop:
  invoke void @f()
          to label %loop unwind label %cleanup
cleanup:
  ; CHECK: cleanup:
  ; CHECK:   [[Load:%[^ ]+]] = load i32, i32* [[SpillSlot]]
  ; CHECK:   call void @h(i32 [[Load]])
  cleanuppad void []
  call void @h(i32 %x)
  cleanupret void unwind to caller
}

; CHECK-LABEL: @test7(
define void @test7() personality i32 (...)* @__CxxFrameHandler3 {
entry:
  ; %x is an EH pad phi, so gets stored in pred here
  ; CHECK: entry:
  ; CHECK:   store i32 1, i32* [[SlotX:%[^ ]+]]
  ; CHECK:   invoke void @f()
  invoke void @f()
     to label %invoke.cont unwind label %catchpad
invoke.cont:
  ; %x is an EH pad phi, so gets stored in pred here
  ; CHECK: invoke.cont:
  ; CHECK:   store i32 2, i32* [[SlotX]]
  ; CHECK:   invoke void @f()
  invoke void @f()
    to label %exit unwind label %catchpad
catchpad:
  ; %x phi should be eliminated
  ; CHECK: catchpad:
  ; CHECK-NEXT: catchpad void
  %x = phi i32 [ 1, %entry ], [ 2, %invoke.cont ]
  catchpad void [] to label %catch unwind label %catchend
catch:
  %b = call i1 @i()
  br i1 %b, label %left, label %right
left:
  ; Edge from %left to %join needs to be split so that
  ; the load of %x can be inserted *after* the catchret
  ; CHECK: left:
  ; CHECK-NEXT: catchret void to label %[[SplitLeft:[^ ]+]]
  catchret void to label %join
  ; CHECK: [[SplitLeft]]:
  ; CHECK:   [[LoadX:%[^ ]+]] = load i32, i32* [[SlotX]]
  ; CHECK:   br label %join
right:
  ; Edge from %right to %join needs to be split so that
  ; the load of %y can be inserted *after* the catchret
  ; CHECK: right:
  ; CHECK:   store i32 %y, i32* [[SlotY:%[^ ]+]]
  ; CHECK:   catchret void to label %[[SplitRight:[^ ]+]]
  %y = call i32 @g()
  catchret void to label %join
  ; CHECK: [[SplitRight]]:
  ; CHECK:   [[LoadY:%[^ ]+]] = load i32, i32* [[SlotY]]
  ; CHECK:   br label %join
catchend:
  catchendpad unwind to caller
join:
  ; CHECK: join:
  ; CHECK:   %phi = phi i32 [ [[LoadX]], %[[SplitLeft]] ], [ [[LoadY]], %[[SplitRight]] ]
  %phi = phi i32 [ %x, %left ], [ %y, %right ]
  call void @h(i32 %phi)
  br label %exit
exit:
  ret void
}
