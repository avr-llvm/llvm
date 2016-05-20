# RUN: llvm-mc %s -triple=x86_64-unknown-unknown | FileCheck %s

movb  $127, %al
movb  $-128, %al

movw  $32767, %ax
movw  $-32768, %ax

movl  $2147483647, %eax
movl  $-2147483648, %eax

movabsq	$9223372036854775807, %rax

# FIXME: This line causes UB failure.
# movabsq	$-9223372036854775808, %rax

# CHECK:  movb  $127, %al
# CHECK:  movb  $-128, %al

# CHECK:  movw  $32767, %ax             # imm = 0x7FFF
# CHECK:  movw  $-32768, %ax            # imm = 0xFFFFFFFFFFFF8000

# CHECK:  movl  $2147483647, %eax       # imm = 0x7FFFFFFF
# CHECK:  movl  $-2147483648, %eax      # imm = 0xFFFFFFFF80000000

# CHECK:  movabsq $9223372036854775807, %rax # imm = 0x7FFFFFFFFFFFFFFF
# FIXME-CHECK:  movabsq $-9223372036854775808, %rax # imm = 0x8000000000000000

