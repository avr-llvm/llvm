; RUN: llc -mtriple=amdgcn--amdhsa -mcpu=fiji -mattr=+amdgpu-debugger-insert-nops -verify-machineinstrs < %s | FileCheck %s

; CHECK: debugger_insert_nops.cl:2:3
; CHECK-NEXT: s_nop 0
; CHECK: s_nop 0
; CHECK-NEXT: debugger_insert_nops.cl:3:3
; CHECK-NEXT: s_nop 0
; CHECK: s_nop 0
; CHECK-NEXT: debugger_insert_nops.cl:4:3
; CHECK-NEXT: s_nop 0
; CHECK: s_nop 0
; CHECK-NEXT: debugger_insert_nops.cl:5:3
; CHECK-NEXT: s_nop 0
; CHECK: s_nop 0
; CHECK-NEXT: debugger_insert_nops.cl:6:1
; CHECK-NEXT: s_nop 0
; CHECK-NEXT: s_endpgm

; Function Attrs: nounwind
define void @debugger_insert_nops(i32 addrspace(1)* %A) #0 !dbg !12 {
entry:
  %A.addr = alloca i32 addrspace(1)*, align 4
  store i32 addrspace(1)* %A, i32 addrspace(1)** %A.addr, align 4
  call void @llvm.dbg.declare(metadata i32 addrspace(1)** %A.addr, metadata !17, metadata !18), !dbg !19
  %0 = load i32 addrspace(1)*, i32 addrspace(1)** %A.addr, align 4, !dbg !20
  %arrayidx = getelementptr inbounds i32, i32 addrspace(1)* %0, i32 0, !dbg !20
  store i32 1, i32 addrspace(1)* %arrayidx, align 4, !dbg !21
  %1 = load i32 addrspace(1)*, i32 addrspace(1)** %A.addr, align 4, !dbg !22
  %arrayidx1 = getelementptr inbounds i32, i32 addrspace(1)* %1, i32 1, !dbg !22
  store i32 2, i32 addrspace(1)* %arrayidx1, align 4, !dbg !23
  %2 = load i32 addrspace(1)*, i32 addrspace(1)** %A.addr, align 4, !dbg !24
  %arrayidx2 = getelementptr inbounds i32, i32 addrspace(1)* %2, i32 2, !dbg !24
  store i32 3, i32 addrspace(1)* %arrayidx2, align 4, !dbg !25
  %3 = load i32 addrspace(1)*, i32 addrspace(1)** %A.addr, align 4, !dbg !26
  %arrayidx3 = getelementptr inbounds i32, i32 addrspace(1)* %3, i32 4, !dbg !26
  store i32 4, i32 addrspace(1)* %arrayidx3, align 4, !dbg !27
  ret void, !dbg !28
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="fiji" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }

!llvm.dbg.cu = !{!0}
!opencl.kernels = !{!3}
!llvm.module.flags = !{!9, !10}
!llvm.ident = !{!11}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "clang version 3.9.0 (trunk 266620)", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2)
!1 = !DIFile(filename: "debugger_insert_nops.cl", directory: "/home/kzhuravl/Sandbox")
!2 = !{}
!3 = !{void (i32 addrspace(1)*)* @debugger_insert_nops, !4, !5, !6, !7, !8}
!4 = !{!"kernel_arg_addr_space", i32 1}
!5 = !{!"kernel_arg_access_qual", !"none"}
!6 = !{!"kernel_arg_type", !"int*"}
!7 = !{!"kernel_arg_base_type", !"int*"}
!8 = !{!"kernel_arg_type_qual", !""}
!9 = !{i32 2, !"Dwarf Version", i32 4}
!10 = !{i32 2, !"Debug Info Version", i32 3}
!11 = !{!"clang version 3.9.0 (trunk 266620)"}
!12 = distinct !DISubprogram(name: "debugger_insert_nops", scope: !1, file: !1, line: 1, type: !13, isLocal: false, isDefinition: true, scopeLine: 1, flags: DIFlagPrototyped, isOptimized: false, unit: !0, variables: !2)
!13 = !DISubroutineType(types: !14)
!14 = !{null, !15}
!15 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !16, size: 64, align: 32)
!16 = !DIBasicType(name: "int", size: 32, align: 32, encoding: DW_ATE_signed)
!17 = !DILocalVariable(name: "A", arg: 1, scope: !12, file: !1, line: 1, type: !15)
!18 = !DIExpression()
!19 = !DILocation(line: 1, column: 46, scope: !12)
!20 = !DILocation(line: 2, column: 3, scope: !12)
!21 = !DILocation(line: 2, column: 8, scope: !12)
!22 = !DILocation(line: 3, column: 3, scope: !12)
!23 = !DILocation(line: 3, column: 8, scope: !12)
!24 = !DILocation(line: 4, column: 3, scope: !12)
!25 = !DILocation(line: 4, column: 8, scope: !12)
!26 = !DILocation(line: 5, column: 3, scope: !12)
!27 = !DILocation(line: 5, column: 8, scope: !12)
!28 = !DILocation(line: 6, column: 1, scope: !12)
