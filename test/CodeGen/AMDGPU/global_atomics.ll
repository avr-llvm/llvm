; RUN: llc < %s -march=amdgcn -mcpu=SI -verify-machineinstrs | FileCheck --check-prefix=GCN --check-prefix=SI --check-prefix=FUNC %s
; RUN: llc < %s -march=amdgcn -mcpu=tonga -verify-machineinstrs | FileCheck --check-prefix=GCN --check-prefix=VI --check-prefix=FUNC %s


; FUNC-LABEL: {{^}}atomic_add_i32_offset:
; GCN: buffer_atomic_add v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_add_i32_offset(i32 addrspace(1)* %out, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile add i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_add_i32_ret_offset:
; GCN: buffer_atomic_add [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_add_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile add i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_add_i32_addr64_offset:
; SI: buffer_atomic_add v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
; VI: flat_atomic_add v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}

define void @atomic_add_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile add i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_add_i32_ret_addr64_offset:
; SI: buffer_atomic_add [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_add [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_add_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile add i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_add_i32:
; GCN: buffer_atomic_add v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_add_i32(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0  = atomicrmw volatile add i32 addrspace(1)* %out, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_add_i32_ret:
; GCN: buffer_atomic_add [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_add_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %0  = atomicrmw volatile add i32 addrspace(1)* %out, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_add_i32_addr64:
; SI: buffer_atomic_add v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_add v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_add_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile add i32 addrspace(1)* %ptr, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_add_i32_ret_addr64:
; SI: buffer_atomic_add [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_add [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_add_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile add i32 addrspace(1)* %ptr, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_and_i32_offset:
; GCN: buffer_atomic_and v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_and_i32_offset(i32 addrspace(1)* %out, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile and i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_and_i32_ret_offset:
; GCN: buffer_atomic_and [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_and_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile and i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_and_i32_addr64_offset:
; SI: buffer_atomic_and v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
; VI: flat_atomic_and v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_and_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile and i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_and_i32_ret_addr64_offset:
; SI: buffer_atomic_and [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_and [[RET:v[0-9]]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_and_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile and i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_and_i32:
; GCN: buffer_atomic_and v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_and_i32(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0  = atomicrmw volatile and i32 addrspace(1)* %out, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_and_i32_ret:
; GCN: buffer_atomic_and [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_and_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %0  = atomicrmw volatile and i32 addrspace(1)* %out, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_and_i32_addr64:
; SI: buffer_atomic_and v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_and v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_and_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile and i32 addrspace(1)* %ptr, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_and_i32_ret_addr64:
; SI: buffer_atomic_and [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_and [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_and_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile and i32 addrspace(1)* %ptr, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_sub_i32_offset:
; GCN: buffer_atomic_sub v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_sub_i32_offset(i32 addrspace(1)* %out, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile sub i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_sub_i32_ret_offset:
; GCN: buffer_atomic_sub [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_sub_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile sub i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_sub_i32_addr64_offset:
; SI: buffer_atomic_sub v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
; VI: flat_atomic_sub v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_sub_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile sub i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_sub_i32_ret_addr64_offset:
; SI: buffer_atomic_sub [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_sub [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_sub_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile sub i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_sub_i32:
; GCN: buffer_atomic_sub v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_sub_i32(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0  = atomicrmw volatile sub i32 addrspace(1)* %out, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_sub_i32_ret:
; GCN: buffer_atomic_sub [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_sub_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %0  = atomicrmw volatile sub i32 addrspace(1)* %out, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_sub_i32_addr64:
; SI: buffer_atomic_sub v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_sub v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_sub_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile sub i32 addrspace(1)* %ptr, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_sub_i32_ret_addr64:
; SI: buffer_atomic_sub [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_sub [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_sub_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile sub i32 addrspace(1)* %ptr, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_max_i32_offset:
; GCN: buffer_atomic_smax v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_max_i32_offset(i32 addrspace(1)* %out, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile max i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_max_i32_ret_offset:
; GCN: buffer_atomic_smax [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_max_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile max i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_max_i32_addr64_offset:
; SI: buffer_atomic_smax v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
; VI: flat_atomic_smax v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_max_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile max i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_max_i32_ret_addr64_offset:
; SI: buffer_atomic_smax [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_smax [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_max_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile max i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_max_i32:
; GCN: buffer_atomic_smax v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_max_i32(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0  = atomicrmw volatile max i32 addrspace(1)* %out, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_max_i32_ret:
; GCN: buffer_atomic_smax [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_max_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %0  = atomicrmw volatile max i32 addrspace(1)* %out, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_max_i32_addr64:
; SI: buffer_atomic_smax v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_smax v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_max_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile max i32 addrspace(1)* %ptr, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_max_i32_ret_addr64:
; SI: buffer_atomic_smax [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_smax [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_max_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile max i32 addrspace(1)* %ptr, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_umax_i32_offset:
; GCN: buffer_atomic_umax v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_umax_i32_offset(i32 addrspace(1)* %out, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile umax i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_umax_i32_ret_offset:
; GCN: buffer_atomic_umax [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_umax_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile umax i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_umax_i32_addr64_offset:
; SI: buffer_atomic_umax v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
; VI: flat_atomic_umax v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_umax_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile umax i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_umax_i32_ret_addr64_offset:
; SI: buffer_atomic_umax [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_umax [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_umax_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile umax i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_umax_i32:
; GCN: buffer_atomic_umax v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_umax_i32(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0  = atomicrmw volatile umax i32 addrspace(1)* %out, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_umax_i32_ret:
; GCN: buffer_atomic_umax [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_umax_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %0  = atomicrmw volatile umax i32 addrspace(1)* %out, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_umax_i32_addr64:
; SI: buffer_atomic_umax v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_umax v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_umax_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile umax i32 addrspace(1)* %ptr, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_umax_i32_ret_addr64:
; SI: buffer_atomic_umax [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_umax [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_umax_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile umax i32 addrspace(1)* %ptr, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_min_i32_offset:
; GCN: buffer_atomic_smin v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_min_i32_offset(i32 addrspace(1)* %out, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile min i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_min_i32_ret_offset:
; GCN: buffer_atomic_smin [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_min_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile min i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_min_i32_addr64_offset:
; SI: buffer_atomic_smin v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
; VI: flat_atomic_smin v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_min_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile min i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_min_i32_ret_addr64_offset:
; SI: buffer_atomic_smin [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_smin [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_min_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile min i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_min_i32:
; GCN: buffer_atomic_smin v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_min_i32(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0  = atomicrmw volatile min i32 addrspace(1)* %out, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_min_i32_ret:
; GCN: buffer_atomic_smin [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_min_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %0  = atomicrmw volatile min i32 addrspace(1)* %out, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_min_i32_addr64:
; SI: buffer_atomic_smin v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_smin v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_min_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile min i32 addrspace(1)* %ptr, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_min_i32_ret_addr64:
; SI: buffer_atomic_smin [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_smin [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_min_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile min i32 addrspace(1)* %ptr, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_umin_i32_offset:
; GCN: buffer_atomic_umin v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_umin_i32_offset(i32 addrspace(1)* %out, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile umin i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_umin_i32_ret_offset:
; GCN: buffer_atomic_umin [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_umin_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile umin i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_umin_i32_addr64_offset:
; SI: buffer_atomic_umin v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
; VI: flat_atomic_umin v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_umin_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile umin i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_umin_i32_ret_addr64_offset:
; SI: buffer_atomic_umin [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_umin [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_umin_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile umin i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_umin_i32:
; GCN: buffer_atomic_umin v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_umin_i32(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0  = atomicrmw volatile umin i32 addrspace(1)* %out, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_umin_i32_ret:
; SI: buffer_atomic_umin [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_umin_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %0  = atomicrmw volatile umin i32 addrspace(1)* %out, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_umin_i32_addr64:
; SI: buffer_atomic_umin v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_umin v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_umin_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile umin i32 addrspace(1)* %ptr, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_umin_i32_ret_addr64:
; SI: buffer_atomic_umin [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_umin [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_umin_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile umin i32 addrspace(1)* %ptr, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_or_i32_offset:
; GCN: buffer_atomic_or v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_or_i32_offset(i32 addrspace(1)* %out, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile or i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_or_i32_ret_offset:
; GCN: buffer_atomic_or [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_or_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile or i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_or_i32_addr64_offset:
; SI: buffer_atomic_or v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
; VI: flat_atomic_or v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_or_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile or i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_or_i32_ret_addr64_offset:
; SI: buffer_atomic_or [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_or [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_or_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile or i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_or_i32:
; GCN: buffer_atomic_or v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_or_i32(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0  = atomicrmw volatile or i32 addrspace(1)* %out, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_or_i32_ret:
; GCN: buffer_atomic_or [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_or_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %0  = atomicrmw volatile or i32 addrspace(1)* %out, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_or_i32_addr64:
; SI: buffer_atomic_or v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_or v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_or_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile or i32 addrspace(1)* %ptr, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_or_i32_ret_addr64:
; SI: buffer_atomic_or [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_or [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_or_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile or i32 addrspace(1)* %ptr, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_xchg_i32_offset:
; GCN: buffer_atomic_swap v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_xchg_i32_offset(i32 addrspace(1)* %out, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile xchg i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_xchg_i32_ret_offset:
; GCN: buffer_atomic_swap [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_xchg_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile xchg i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_xchg_i32_addr64_offset:
; SI: buffer_atomic_swap v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
define void @atomic_xchg_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile xchg i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_xchg_i32_ret_addr64_offset:
; SI: buffer_atomic_swap [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_swap [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_xchg_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile xchg i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_xchg_i32:
; GCN: buffer_atomic_swap v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_xchg_i32(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0  = atomicrmw volatile xchg i32 addrspace(1)* %out, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_xchg_i32_ret:
; GCN: buffer_atomic_swap [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_xchg_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %0  = atomicrmw volatile xchg i32 addrspace(1)* %out, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_xchg_i32_addr64:
; SI: buffer_atomic_swap v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_swap v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_xchg_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile xchg i32 addrspace(1)* %ptr, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_xchg_i32_ret_addr64:
; SI: buffer_atomic_swap [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_swap [[RET:v[0-9]+]],  v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_xchg_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile xchg i32 addrspace(1)* %ptr, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; CMP_SWAP

; FUNC-LABEL: {{^}}atomic_cmpxchg_i32_offset:
; GCN: buffer_atomic_cmpswap v[{{[0-9]+}}:{{[0-9]+}}], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_cmpxchg_i32_offset(i32 addrspace(1)* %out, i32 %in, i32 %old) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = cmpxchg volatile i32 addrspace(1)* %gep, i32 %old, i32 %in seq_cst seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_cmpxchg_i32_ret_offset:
; GCN: buffer_atomic_cmpswap v{{\[}}[[RET:[0-9]+]]{{:[0-9]+}}], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword v[[RET]]
define void @atomic_cmpxchg_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i32 %old) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = cmpxchg volatile i32 addrspace(1)* %gep, i32 %old, i32 %in seq_cst seq_cst
  %1  = extractvalue { i32, i1 } %0, 0
  store i32 %1, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_cmpxchg_i32_addr64_offset:
; SI: buffer_atomic_cmpswap v[{{[0-9]+\:[0-9]+}}], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
define void @atomic_cmpxchg_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index, i32 %old) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0   = cmpxchg volatile i32 addrspace(1)* %gep, i32 %old, i32 %in seq_cst seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_cmpxchg_i32_ret_addr64_offset:
; SI: buffer_atomic_cmpswap v{{\[}}[[RET:[0-9]+]]:{{[0-9]+}}], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_cmpswap v[[RET:[0-9]+]], v[{{[0-9]+:[0-9]+}}], v[{{[0-9]+:[0-9]+}}] glc{{$}}
; GCN: buffer_store_dword v[[RET]]
define void @atomic_cmpxchg_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index, i32 %old) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0   = cmpxchg volatile i32 addrspace(1)* %gep, i32 %old, i32 %in seq_cst seq_cst
  %1  = extractvalue { i32, i1 } %0, 0
  store i32 %1, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_cmpxchg_i32:
; GCN: buffer_atomic_cmpswap v[{{[0-9]+:[0-9]+}}], off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_cmpxchg_i32(i32 addrspace(1)* %out, i32 %in, i32 %old) {
entry:
  %0  = cmpxchg volatile i32 addrspace(1)* %out, i32 %old, i32 %in seq_cst seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_cmpxchg_i32_ret:
; GCN: buffer_atomic_cmpswap v{{\[}}[[RET:[0-9]+]]:{{[0-9]+}}], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword v[[RET]]
define void @atomic_cmpxchg_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i32 %old) {
entry:
  %0  = cmpxchg volatile i32 addrspace(1)* %out, i32 %old, i32 %in seq_cst seq_cst
  %1  = extractvalue { i32, i1 } %0, 0
  store i32 %1, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_cmpxchg_i32_addr64:
; SI: buffer_atomic_cmpswap v[{{[0-9]+:[0-9]+}}], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_cmpswap v[{{[0-9]+:[0-9]+}}], v[{{[0-9]+:[0-9]+}}]{{$}}
define void @atomic_cmpxchg_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index, i32 %old) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = cmpxchg volatile i32 addrspace(1)* %ptr, i32 %old, i32 %in seq_cst seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_cmpxchg_i32_ret_addr64:
; SI: buffer_atomic_cmpswap v{{\[}}[[RET:[0-9]+]]:{{[0-9]+}}], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_cmpswap v[[RET:[0-9]+]], v[{{[0-9]+:[0-9]+}}], v[{{[0-9]+:[0-9]+}}] glc{{$}}
; GCN: buffer_store_dword v[[RET]]
define void @atomic_cmpxchg_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index, i32 %old) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = cmpxchg volatile i32 addrspace(1)* %ptr, i32 %old, i32 %in seq_cst seq_cst
  %1  = extractvalue { i32, i1 } %0, 0
  store i32 %1, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_xor_i32_offset:
; GCN: buffer_atomic_xor v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16{{$}}
define void @atomic_xor_i32_offset(i32 addrspace(1)* %out, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile xor i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_xor_i32_ret_offset:
; GCN: buffer_atomic_xor [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_xor_i32_ret_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  %0  = atomicrmw volatile xor i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_xor_i32_addr64_offset:
; SI: buffer_atomic_xor v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16{{$}}
; VI: flat_atomic_xor v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_xor_i32_addr64_offset(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile xor i32 addrspace(1)* %gep, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_xor_i32_ret_addr64_offset:
; SI: buffer_atomic_xor [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_atomic_xor [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_xor_i32_ret_addr64_offset(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = atomicrmw volatile xor i32 addrspace(1)* %gep, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_xor_i32:
; GCN: buffer_atomic_xor v{{[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0{{$}}
define void @atomic_xor_i32(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0  = atomicrmw volatile xor i32 addrspace(1)* %out, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_xor_i32_ret:
; GCN: buffer_atomic_xor [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_xor_i32_ret(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in) {
entry:
  %0  = atomicrmw volatile xor i32 addrspace(1)* %out, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; FUNC-LABEL: {{^}}atomic_xor_i32_addr64:
; SI: buffer_atomic_xor v{{[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64{{$}}
; VI: flat_atomic_xor v[{{[0-9]+:[0-9]+}}], v{{[0-9]+$}}
define void @atomic_xor_i32_addr64(i32 addrspace(1)* %out, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile xor i32 addrspace(1)* %ptr, i32 %in seq_cst
  ret void
}

; FUNC-LABEL: {{^}}atomic_xor_i32_ret_addr64:
; SI: buffer_atomic_xor [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_atomic_xor [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}], v{{[0-9]+}} glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_xor_i32_ret_addr64(i32 addrspace(1)* %out, i32 addrspace(1)* %out2, i32 %in, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %0  = atomicrmw volatile xor i32 addrspace(1)* %ptr, i32 %in seq_cst
  store i32 %0, i32 addrspace(1)* %out2
  ret void
}

; ATOMIC_LOAD
; FUNC-LABEL: {{^}}atomic_load_i32_offset:
; SI: buffer_load_dword [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; VI: flat_load_dword [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}] glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_load_i32_offset(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %in, i32 4
  %0  = load atomic i32, i32 addrspace(1)* %gep  seq_cst, align 4
  store i32 %0, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}atomic_load_i32:
; SI: buffer_load_dword [[RET:v[0-9]+]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; VI: flat_load_dword [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}] glc
; GCN: buffer_store_dword [[RET]]
define void @atomic_load_i32(i32 addrspace(1)* %in, i32 addrspace(1)* %out) {
entry:
  %0  = load atomic i32, i32 addrspace(1)* %in seq_cst, align 4
  store i32 %0, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}atomic_load_i32_addr64_offset:
; SI: buffer_load_dword [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_load_dword [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}] glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_load_i32_addr64_offset(i32 addrspace(1)* %in, i32 addrspace(1)* %out, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %in, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  %0  = load atomic i32, i32 addrspace(1)* %gep seq_cst, align 4
  store i32 %0, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}atomic_load_i32_addr64:
; SI: buffer_load_dword [[RET:v[0-9]+]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_load_dword [[RET:v[0-9]+]], v[{{[0-9]+:[0-9]+}}] glc{{$}}
; GCN: buffer_store_dword [[RET]]
define void @atomic_load_i32_addr64(i32 addrspace(1)* %in, i32 addrspace(1)* %out, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %in, i64 %index
  %0  = load atomic i32, i32 addrspace(1)* %ptr seq_cst, align 4
  store i32 %0, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}atomic_load_i64_offset:
; SI: buffer_load_dwordx2 [[RET:v\[[0-9]+:[0-9]+\]]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:32 glc{{$}}
; VI: flat_load_dwordx2 [[RET:v\[[0-9]+:[0-9]\]]], v[{{[0-9]+}}:{{[0-9]+}}] glc{{$}}
; GCN: buffer_store_dwordx2 [[RET]]
define void @atomic_load_i64_offset(i64 addrspace(1)* %in, i64 addrspace(1)* %out) {
entry:
  %gep = getelementptr i64, i64 addrspace(1)* %in, i64 4
  %0  = load atomic i64, i64 addrspace(1)* %gep  seq_cst, align 8
  store i64 %0, i64 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}atomic_load_i64:
; SI: buffer_load_dwordx2 [[RET:v\[[0-9]+:[0-9]\]]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; VI: flat_load_dwordx2 [[RET:v\[[0-9]+:[0-9]\]]], v[{{[0-9]+}}:{{[0-9]+}}] glc
; GCN: buffer_store_dwordx2 [[RET]]
define void @atomic_load_i64(i64 addrspace(1)* %in, i64 addrspace(1)* %out) {
entry:
  %0  = load atomic i64, i64 addrspace(1)* %in seq_cst, align 8
  store i64 %0, i64 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}atomic_load_i64_addr64_offset:
; SI: buffer_load_dwordx2 [[RET:v\[[0-9]+:[0-9]+\]]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:32 glc{{$}}
; VI: flat_load_dwordx2 [[RET:v\[[0-9]+:[0-9]+\]]], v[{{[0-9]+:[0-9]+}}] glc{{$}}
; GCN: buffer_store_dwordx2 [[RET]]
define void @atomic_load_i64_addr64_offset(i64 addrspace(1)* %in, i64 addrspace(1)* %out, i64 %index) {
entry:
  %ptr = getelementptr i64, i64 addrspace(1)* %in, i64 %index
  %gep = getelementptr i64, i64 addrspace(1)* %ptr, i64 4
  %0  = load atomic i64, i64 addrspace(1)* %gep seq_cst, align 8
  store i64 %0, i64 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: {{^}}atomic_load_i64_addr64:
; SI: buffer_load_dwordx2 [[RET:v\[[0-9]+:[0-9]\]]], v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_load_dwordx2 [[RET:v\[[0-9]+:[0-9]+\]]], v[{{[0-9]+:[0-9]+}}] glc{{$}}
; GCN: buffer_store_dwordx2 [[RET]]
define void @atomic_load_i64_addr64(i64 addrspace(1)* %in, i64 addrspace(1)* %out, i64 %index) {
entry:
  %ptr = getelementptr i64, i64 addrspace(1)* %in, i64 %index
  %0  = load atomic i64, i64 addrspace(1)* %ptr seq_cst, align 8
  store i64 %0, i64 addrspace(1)* %out
  ret void
}

; ATOMIC_STORE
; FUNC-LABEL: {{^}}atomic_store_i32_offset:
; SI: buffer_store_dword {{v[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:16 glc{{$}}
; VI: flat_store_dword v[{{[0-9]+}}:{{[0-9]+}}], {{v[0-9]+}} glc{{$}}
define void @atomic_store_i32_offset(i32 %in, i32 addrspace(1)* %out) {
entry:
  %gep = getelementptr i32, i32 addrspace(1)* %out, i32 4
  store atomic i32 %in, i32 addrspace(1)* %gep  seq_cst, align 4
  ret void
}

; FUNC-LABEL: {{^}}atomic_store_i32:
; SI: buffer_store_dword {{v[0-9]+}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc{{$}}
; VI: flat_store_dword v[{{[0-9]+}}:{{[0-9]+}}], {{v[0-9]+}} glc{{$}}
define void @atomic_store_i32(i32 %in, i32 addrspace(1)* %out) {
entry:
  store atomic i32 %in, i32 addrspace(1)* %out seq_cst, align 4
  ret void
}

; FUNC-LABEL: {{^}}atomic_store_i32_addr64_offset:
; SI: buffer_store_dword {{v[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:16 glc{{$}}
; VI: flat_store_dword v[{{[0-9]+}}:{{[0-9]+}}], {{v[0-9]+}} glc{{$}}
define void @atomic_store_i32_addr64_offset(i32 %in, i32 addrspace(1)* %out, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  %gep = getelementptr i32, i32 addrspace(1)* %ptr, i32 4
  store atomic i32 %in, i32 addrspace(1)* %gep seq_cst, align 4
  ret void
}

; FUNC-LABEL: {{^}}atomic_store_i32_addr64:
; SI: buffer_store_dword {{v[0-9]+}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_store_dword v[{{[0-9]+}}:{{[0-9]+}}], {{v[0-9]+}} glc{{$}}
define void @atomic_store_i32_addr64(i32 %in, i32 addrspace(1)* %out, i64 %index) {
entry:
  %ptr = getelementptr i32, i32 addrspace(1)* %out, i64 %index
  store atomic i32 %in, i32 addrspace(1)* %ptr seq_cst, align 4
  ret void
}

; FUNC-LABEL: {{^}}atomic_store_i64_offset:
; SI: buffer_store_dwordx2 [[RET:v\[[0-9]+:[0-9]+\]]], off, s[{{[0-9]+}}:{{[0-9]+}}], 0 offset:32 glc{{$}}
; VI: flat_store_dwordx2 [[RET:v\[[0-9]+:[0-9]\]]], v[{{[0-9]+}}:{{[0-9]+}}] glc{{$}}
define void @atomic_store_i64_offset(i64 %in, i64 addrspace(1)* %out) {
entry:
  %gep = getelementptr i64, i64 addrspace(1)* %out, i64 4
  store atomic i64 %in, i64 addrspace(1)* %gep  seq_cst, align 8
  ret void
}

; FUNC-LABEL: {{^}}atomic_store_i64:
; SI: buffer_store_dwordx2 {{v\[[0-9]+:[0-9]\]}}, off, s[{{[0-9]+}}:{{[0-9]+}}], 0 glc
; VI: flat_store_dwordx2 {{v\[[0-9]+:[0-9]\]}}, v[{{[0-9]+}}:{{[0-9]+}}] glc
define void @atomic_store_i64(i64 %in, i64 addrspace(1)* %out) {
entry:
  store atomic i64 %in, i64 addrspace(1)* %out seq_cst, align 8
  ret void
}

; FUNC-LABEL: {{^}}atomic_store_i64_addr64_offset:
; SI: buffer_store_dwordx2 {{v\[[0-9]+:[0-9]+\]}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 offset:32 glc{{$}}
; VI: flat_store_dwordx2 {{v\[[0-9]+:[0-9]+\]}}, v[{{[0-9]+:[0-9]+}}] glc{{$}}
define void @atomic_store_i64_addr64_offset(i64 %in, i64 addrspace(1)* %out, i64 %index) {
entry:
  %ptr = getelementptr i64, i64 addrspace(1)* %out, i64 %index
  %gep = getelementptr i64, i64 addrspace(1)* %ptr, i64 4
  store atomic i64 %in, i64 addrspace(1)* %gep seq_cst, align 8
  ret void
}

; FUNC-LABEL: {{^}}atomic_store_i64_addr64:
; SI: buffer_store_dwordx2 {{v\[[0-9]+:[0-9]\]}}, v[{{[0-9]+}}:{{[0-9]+}}], s[{{[0-9]+}}:{{[0-9]+}}], 0 addr64 glc{{$}}
; VI: flat_store_dwordx2 {{v\[[0-9]+:[0-9]+\]}}, v[{{[0-9]+:[0-9]+}}] glc{{$}}
define void @atomic_store_i64_addr64(i64 %in, i64 addrspace(1)* %out, i64 %index) {
entry:
  %ptr = getelementptr i64, i64 addrspace(1)* %out, i64 %index
  store atomic i64 %in, i64 addrspace(1)* %ptr seq_cst, align 8
  ret void
}
