/**
 * 功能：公共头文件
 *
 *创建时间：2024-12-31
 *作者：yes
 */
#ifndef OS_H
#define OS_H

// 各选择子
#define KERNEL_CODE_SEG (1 * 8)
#define KERNEL_DATA_SEG (2 * 8)
#define APP_CODE_SEG ((3 * 8) | 3) // 特权级3
#define APP_DATA_SEG ((4 * 8) | 3) // 特权级3
#define TASK0_TSS_SEG ((5 * 8))
#define TASK1_TSS_SEG ((6 * 8))
#define SYSCALL_SEG ((7 * 8))

#define TASK0_LDT_SEG ((8 * 8))
#define TASK1_LDT_SEG ((9 * 8))

#define TASK_CODE_SEG ((0 * 8 | 0x4 | 3))
#define TASK_DATA_SEG ((1 * 8 | 0x4 | 3))
#endif // OS_H
