/*
 * @FilePath     : /code/source/kernel/init/start.S
 * @Description  :  内核工程启动文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-14 22:00:23
 * Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */

#include "os_cfg.h" 
    .text
    .global _start # 将此作为程序入口
    .extern init_main
    .extern kernel_init
    # void start(boot_info_t * boot_info)

_start:
    push %ebp # 保存返回地址
    mov %esp, %ebp # 使ebp指向栈底
    mov 0x8(%ebp),%eax # 将boot_info 传递给 eax
    # mov 4(%esp),%eax

    # kernel_init(boot_info)
    push %eax # boot_info 压栈

    call kernel_init # 调用C函数
    
    # 重新加载GDT
	jmp $KERNEL_SELECTOR_CS, $gdt_reload

gdt_reload:
	mov $KERNEL_SELECTOR_DS, %ax		# 16为数据段选择子
	mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

	# 栈设置
	mov $(stack + KERNEL_STACK_SIZE), %esp

	# 栈和段等沿用之前的设置
	jmp init_main

	.bss
.comm stack, KERNEL_STACK_SIZE    # comm 声明未初始化的通用内存区域，以字节计


    .text
    .extern do_handler_unknown
    .global exception_handler_unknown

exception_handler_unknown:
    
    # 保护现场
    pusha 
    push %ds
    push %es
    push %fs
    push %gs

    call do_handler_unknown

    # 恢复现场
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    iret