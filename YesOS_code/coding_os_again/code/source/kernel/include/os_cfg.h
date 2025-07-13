/**
 * @FilePath     : /code/source/kernel/include/os_cfg.h
 * @Description  :  os配置文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-13 20:41:29
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#ifndef OS_CFG_H
#define OS_CFG_H

#define GDT_TABLE_SIZE 256           // GDT表项数量
#define KERNEL_SELECTOR_CS (1 * 8)   // 内核代码段描述符
#define KERNEL_SELECTOR_DS (2 * 8)   // 内核数据段描述符
#define KERNEL_STACK_SIZE (8 * 1024) // 栈大小
#define OS_TICKS_MS (10)             // 每隔10ms
#define OS_VERSION "1.0.0"           // 系统版本号
#define IDLE_TASK_SIZE 1024          // 空闲进程占空间大小
#define SELECTOR_SYSCALL (3 * 8)     // 系统调用
#define TASK_NR 128                  // 系统中最多存在的进程
#define ROOT_DEV DEV_DISK, 0xb1      // 根文件系统主次设备号

#endif
