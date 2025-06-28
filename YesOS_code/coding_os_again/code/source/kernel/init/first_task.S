/*
 * @FilePath     : /code/source/kernel/init/first_task.S
 * @Description  :  
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-27 17:32:38
 * Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
	.text # 代码
	.global first_task_entry
	.extern first_task_main

first_task_entry:
	# 重新加载所有的数据段描述符
	mov %ss, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	# 长跳转进入first_task_main
	jmp first_task_main