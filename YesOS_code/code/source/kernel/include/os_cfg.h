#ifndef OS_CFG_H
#define OS_CFG_H

#define GDT_TABLE_SIZE 256

#define KERNEL_SELECTOR_CS (1*8)
#define KERNEL_SELECTOR_DS (2*8)
#define KERNEL_STACK_SIZE (8 * 1024)
#define SELECTOR_SYSCALL (3 * 8) // 调用门的选择子

#define OS_TICK_MS 10

#define IDLE_STACK_SIZE 1024

#define OS_VERSION "0.0.1"

#define TASK_NR 128

#endif