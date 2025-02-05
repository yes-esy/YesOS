/**
 * function：16位与32位的启动混合代码
 * author:yes
 */
	#include "os.h"

	//声明本地以下符号是全局的，在其它源文件中可以访问
	.global _start ,timer_init,syscall_handler

	.extern gdt_table, pg_dir, idt_table, task_sched

	//  指定以下的代码生成16位的机器指令，这样才能在启动时的实模式下运行
  	.code16

	//  以下是代码区
 	.text
_start: 
	//  x86 使用段寄存器中的内容作为内存访问的基址，因此以下预先置好
	mov $0, %ax  // 设置代码段
	mov %ax,%ds  // 设置数据段
	mov %ax,%es  // 设置数据段
	mov %ax,%ss  // 设置栈段
	mov $_start,%esp // 设置栈的起始地址

	// 计算机上电启动后，只读取磁盘的第一个扇区(即512字节)到内存0x7c00处，即相当与自己只有个头在内存，
	// 为了能够读取更多的内容，需要自己读取磁盘的更多内容到内存中，下面将自己的其余部分读取到0x7e00处,即0x7c00后的512字节
read_self_all:
	mov $_start_32, %bx // 读取到的内存地址
	mov $0x2, %cx	// ch:磁头号，cl:扇区号
	mov $0x240, %ax // ah:0x42读磁盘命令，al=0x40 64个扇区,多读一些，32kb
	mov $0x80, %dx // dh:磁盘号，dl:驱动器号0x80(磁盘1)
	int $0x0013		// 调用BIOS中断，读取磁盘扇区
	jc read_self_all	// 读取失败则重试

	// 加载idt表和gdt表，进入保护模式
	cli  // 关中断
	lgdt gdt_desc // 加载gdt表
	lidt idt_desc // 加载idt表
	mov $1, %eax
	lmsw %ax // 设置PE位，进入保护模式
	jmp $KERNEL_CODE_SEG,$_start_32 // 进入32位保护模式，可以运行32位指令

	// 跳到引导标志, 由bios在上电后检查
	.org 0x1fe			// 引导标志
	.byte 0x55, 0xaa

	// 32位保护模式，位于512字节后
	.code32
	.text
_start_32:
	// 重新加载数据段寄存器
	mov $KERNEL_DATA_SEG, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	mov $_start, %esp
	
	//跳转到c语言中运行
	call os_init

	// 打开分页机制
	mov $pg_dir, %eax
	mov %eax, %cr3

	mov %cr4, %eax
	orl $(1 << 4), %eax  // PSE位,支持4MB分页
	mov %eax, %cr4


	mov %cr0, %eax
	orl $(1 << 31), %eax  // PG位,开启分页机制
	mov %eax, %cr0

	mov $TASK0_TSS_SEG, %ax
	ltr %ax // 加载TSS

	mov $TASK0_LDT_SEG, %ax
	lldt %ax

	// 下面模拟中断返回，从而实现从特权级0到特权级3的变换
	// 中断发生时，会自动压入原SS, ESP, EFLAGS, CS, EIP到栈中
	push $TASK_DATA_SEG 
	push $task0_dpl3_stack + 1024 //特权级3的栈
	push $0x202 //0中断暂时关掉
	push $TASK_CODE_SEG  // CPL=3时的选择子
	push $task_0_entry  // EIP
	iret // 从中断返回，切换至任务0

timer_init:
	// 保护现场
	push %ds
	pusha // 保存通用寄存器

	mov $0x20, %al
	outb %al, $0x20  // 发送EOI

	mov $KERNEL_DATA_SEG, %ax
	mov %ax, %ds

	call task_sched // 任务切换

	//恢复现场
	popa
	pop %ds
	iret  // 从中断返回


syscall_handler:
	push %ds
	pusha

	mov $KERNEL_DATA_SEG, %ax
	mov %ax, %ds

	mov %esp, %ebp // 保存栈指针
	push 13 * 4(%ebp)
	push 12 * 4(%ebp)	
	push 11 * 4(%ebp)
	call do_syscall

	add $(3 * 4), %esp 

	popa
	pop %ds
	ret $(3 * 4) // 3个参数，每个4字节

// 创建一个进程，使其工作在特权级三之下
task_0_entry:  // 进程0 
	mov %ss, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %gs
	mov %ax, %fs
	jmp task_0


// gdt描述符，由lgdt加载
gdt_desc: 
	.word (256 * 8) -1
	.long gdt_table

// idt描述符，由lidt加载
idt_desc:
	.word (256 * 8) -1
	.long idt_table