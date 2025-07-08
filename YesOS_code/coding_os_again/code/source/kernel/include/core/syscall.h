/**
 * @FilePath     : /code/source/kernel/include/core/syscall.h
 * @Description  :  系统调用头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-03 15:40:18
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef SYSCALL_H
#define SYSCALL_H

#include "comm/types.h"

#define SYS_sleep           0 // 睡眠系统调用编号
#define SYS_getpid          1 // 获取进程PID系统调用编号
#define SYS_fork            2
#define SYS_execve          3
#define SYS_yield           4
#define SYS_open            50
#define SYS_read            51
#define SYS_write           52
#define SYS_close           53
#define SYS_lseek           54
#define SYS_isatty          55
#define SYS_sbrk            56
#define SYS_fstat           57
#define SYS_dup             58
#define SYS_print_msg       100
#define SYSCALL_PARAM_COUNT 5 // 系统调用参数数量

typedef struct _syscall_frame_t
{
    uint32_t eflags;

    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, dummy, ebx, edx, ecx, eax;
    uint32_t eip, cs;
    int func_id, arg0, arg1, arg2, arg3;
    uint32_t esp, ss;
} syscall_frame_t;

void exception_handler_syscall(void); // 系统调用函数声明

#endif