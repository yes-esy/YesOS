#include "os_cfg.h"
    .text
    .global _start
    .extern kernel_init
    .extern init_main
_start:
    push %ebp
    mov %esp, %ebp
    mov 0x8(%ebp), %eax

    # mov 4(%esp), %eax

    push %eax
    call kernel_init

    jmp $KERNEL_SELECTOR_CS, $gdt_reload

gdt_reload:
    mov $KERNEL_SELECTOR_DS, %ax		// 16为数据段选择子
	mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov $(stack + KERNEL_STACK_SIZE), %esp
    jmp init_main

    .bss
.comm stack,KERNEL_STACK_SIZE