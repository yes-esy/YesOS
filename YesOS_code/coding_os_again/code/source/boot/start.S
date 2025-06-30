/*
 * @FilePath     : /code/source/boot/start.S
 * @Description  :  系统引导部分，启动时由硬件加载运行，然后完成对二级引导程序loader的加载
 *                  该部分程序存储于磁盘的第1个扇区，在计算机启动时将会由BIOS加载到0x7c00处
 *                  之后，将由BIOS跳转至0x7c00处开始运行
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-30 10:55:28
 * Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
	#include "boot.h"

  	# 16位代码，务必加上
	
  	.code16
 	.text
	.global _start
	.extern boot_entry # 声明boot_entry为外部函数
_start:
	mov $0 , %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	
	mov $_start, %esp 
	# 或者mov $0x7c00,%esp

# 将loader读到0x8000处
:
	mov $0x8000, %bx	# 读取到的内存地址(目的地址)
	mov $0x2, %cx		# ah: 0x2读磁盘命令 
	mov $0x2, %ah		# ch:磁道号，cl起始扇区号
	mov $64, %al		# al: 读取的扇区数量, 必须小于128，暂设置成32KB
	mov $0x0080, %dx	# dh: 磁头号，dl驱动器号0x80(磁盘1)
	int $0x13 
	jc read_loader # 发生错误反复读取

	# 跳转至C部分执行，由C部分处理
	jmp boot_entry

	# 原地read_loader跳转
	jmp .
	
	# 引导结束段
	.section boot_end, "ax"
boot_sig: .byte 0x55, 0xaa