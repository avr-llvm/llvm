; RUN: opt -S -wholeprogramdevirt %s | FileCheck %s

target datalayout = "e-p:64:64"
target triple = "x86_64-unknown-linux-gnu"

@vt = global [1 x i8*] [i8* bitcast (void (i8*)* @vf to i8*)]

define void @vf(i8* %this) {
  ret void
}

; CHECK: define void @call
define void @call(i8* %obj) {
  %vtableptr = bitcast i8* %obj to [1 x i8*]**
  %vtable = load [1 x i8*]*, [1 x i8*]** %vtableptr
  %vtablei8 = bitcast [1 x i8*]* %vtable to i8*
  %p = call i1 @llvm.bitset.test(i8* %vtablei8, metadata !"bitset")
  call void @llvm.assume(i1 %p)
  %fptrptr = getelementptr [1 x i8*], [1 x i8*]* %vtable, i32 0, i32 0
  %fptr = load i8*, i8** %fptrptr
  %fptr_casted = bitcast i8* %fptr to void (i8*)*
  ; CHECK: call void %
  call void %fptr_casted(i8* %obj)
  ret void
}

declare i1 @llvm.bitset.test(i8*, metadata)
declare void @llvm.assume(i1)

!0 = !{!"bitset", [1 x i8*]* @vt, i32 0}
!llvm.bitsets = !{!0}
