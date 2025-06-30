/*
 * @FilePath     : /code/source/loader/start.S
 * @Description  :  系统引导部分，启动时由硬件加载运行，然后完成对二级引导程序loader的加载
 *                  该部分程序存储于磁盘的第1个扇区，在计算机启动时将会由BIOS加载到0x7c00处
 *                  之后，将由BIOS跳转至0x7c00处开始运行
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-14 22:00:44
 * Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
  	# 16位代码，务必加上
  	.code16
 	.text
	.extern loader_entry # 声明loader_entry为外部函数
	.global _start
_start:
	jmp loader_entry
	
	# 32位代码
	.code32
	.text # 代码
	.global protect_mode_entry
	.extern load_kernel

protect_mode_entry:
	# 重新加载所有的数据段描述符
	mov $16, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	# 长跳转进入到32位内核加载模式中
	jmp $8, $load_kernel