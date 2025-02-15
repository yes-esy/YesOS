.text
.global _start
.extern cstart
_start:
    // 需要重新加载各数据段
    mov %ss, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    jmp cstart
   