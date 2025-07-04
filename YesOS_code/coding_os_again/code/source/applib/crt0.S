/*
 * @FilePath     : /code/source/applib/crt0.S
 * @Description  :  c运行库文件,设置寄存器
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-03 09:56:31
 * Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
    .text
    .global _start
    .extern cstart
_start:
    mov %ss, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
    jmp cstart