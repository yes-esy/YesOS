/*
 * @FilePath     : /code/source/kernel/init/start.S
 * @Description  :  内核工程启动文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-01 20:46:35
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
# 相当于c语言中的宏其中 num为中断向量号, with_err_code 为错误码
.macro exception_handler name num with_err_code
    .extern do_handler_\name
    .global exception_handler_\name

exception_handler_\name:
    .if \with_err_code == 0
        push $0
    .endif
    # 压入中断号
    push $\num

    # 保护现场
    pusha 
    push %ds
    push %es
    push %fs
    push %gs

    push %esp # 相当于给do_handler_unknown 传递参数
    
    call do_handler_\name # 调用特定c处理函数

    # esp出栈
    add $(1*4) , %esp 
    
    # 恢复现场
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popal
    
    add $(2*4) , %esp

    iret
.endm

exception_handler unknown            , -1    , 0 # 未知异常
exception_handler divider            ,  0    , 0 # 除0异常
exception_handler Debug              ,  0    , 0 # Debug异常
exception_handler NMI                ,  2    , 0 #
exception_handler breakpoint         ,  3    , 0 # 断点异常
exception_handler overflow           ,  4    , 0 # 溢出
exception_handler bound_range        ,  5    , 0 # 越界
exception_handler invalid_opcode     ,  6    , 0 # 无效操作码
exception_handler device_unavailable ,  7    , 0 # 设备不可用
exception_handler double_fault       ,  8    , 1 #
exception_handler invalid_tss        ,  10   , 1 # 无效TSS
exception_handler segment_not_present,  11   , 1 # 段不存在
exception_handler stack_segment_fault,  12   , 1 #
exception_handler general_protection ,  13   , 1 #
exception_handler page_fault         ,  14   , 1 # 页错误
exception_handler fpu_error          ,  16   , 0 # 浮点运算单元
exception_handler alignment_check    ,  17   , 1 # 对齐检查
exception_handler machine_check      ,  18   , 0 #
exception_handler smd_exception      ,  19   , 0 #
exception_handler virtual_exception  ,  20   , 0 #
exception_handler control_exception  ,  21   , 1 #
exception_handler timer              ,  0x20 , 0 #
# exception_handler syscall            ,  0x80 , 0 # 系统调用

    .text
    .global simple_switch
    # simple_switch(&from,&to)
simple_switch:
    mov 4(%esp),%eax  # 取from->stack 
    mov 8(%esp),%edx  # 取to->stack

    # 压栈,保存当前任务的状态
    push %ebp
    push %ebx
    push %esi
    push %edi

    # 切换当前栈
    mov %esp,(%eax)  # from->stack = esp
    mov %edx , %esp  # esp = to->stack
    
    # 加载下一任务的栈
    pop %edi
    pop %esi
    pop %ebx
    pop %ebp

    ret
    

    .global exception_handler_syscall
    .extern do_handler_syscall
exception_handler_syscall:
    pusha  # eax值入栈
    push %ds
    push %es
    push %fs
    push %gs
    pushf # eflags寄存器压栈

    mov %esp , %eax # 将当前栈指针ESP的值复制到EAX
    push %eax # 将EAX(即栈指针)作为参数压入栈
    call do_handler_syscall
    add $4 , %esp # 清理参数(1个指针=4字节)

    popf
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa # 弹出到eax寄存器
    retf $(5 * 4)