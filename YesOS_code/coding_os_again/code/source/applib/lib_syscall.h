/**
 * @FilePath     : /code/source/applib/lib_syscall.h
 * @Description  :
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-01 22:00:08
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H

#include "os_cfg.h"
#include "core/syscall.h"
/**
 * 系统调用传递的参数,用结构体传输
 */
typedef struct _syscall_args_t
{
    int id;   // 根据id确定调用函数
    int arg0; // 参数1
    int arg1; // 参数2
    int arg2; // 参数3
    int arg3; // 参数4
} syscall_args_t;

/**
 * @brief        : 通用系统调用接口
 * @param         {syscall_args_t} *args: 传递给系统调用的参数
 * @return        {int} : 系统调用的返回值
 **/
static inline int sys_call(syscall_args_t *args)
{
    int ret;
    uint32_t addr[] = {0, SELECTOR_SYSCALL | 0}; // RPL<=DPL ,RPL设置为0
    __asm__ __volatile__(
        "push %[arg3]\n\t"
        "push %[arg2]\n\t"
        "push %[arg1]\n\t"
        "push %[arg0]\n\t"
        "push %[id]\n\t"
        "lcalll *(%[a])"
        : "=a"(ret) // 表明从eax中取出其值
        : [arg3] "r"(args->arg3),
          [arg2] "r"(args->arg2),
          [arg1] "r"(args->arg1),
          [arg0] "r"(args->arg0),
          [id] "r"(args->id),
          [a] "r"(addr)); // 跳转到exception_handler_syscall
    /*int 80的实现方式*/
    // __asm__ __volatile__(
    //     "int $0x80"
    //     : "=a"(ret)
    //     : "S"(args->arg3),
    //       "d"(args->arg2),
    //       "c"(args->arg1),
    //       "b"(args->arg0),
    //       "a"(args->id));
    return ret;
}

/**
 * @brief        : 延时系统调用
 * @param         {int} ms: 睡眠的毫秒数
 * @return        {void}
 **/
static inline void msleep(int ms)
{
    if (ms < 0) // 小于0不需要延时
    {
        return;
    }
    syscall_args_t args;
    args.id = SYS_sleep; // 设置调用id
    args.arg0 = ms;      // 设置延时时长
    sys_call(&args);     // 通用系统调用,并传递参数
}
/**
 * @brief        : 获取当前进程的ID号
 * @return        {int} : 进程ID号
 **/
static inline int getpid()
{
    syscall_args_t args;
    args.id = SYS_getpid;
    return sys_call(&args);
}
/**
 * @brief        : 简单打印信息
 * @param         {char} *fmt: 格式化字符串
 * @param         {int} arg: 参数
 * @return        {*}
 **/
static inline void print_msg(const char *fmt, int arg)
{
    syscall_args_t args;
    args.id = SYS_print_msg;
    args.arg0 = (int)fmt;
    args.arg1 = arg;
    sys_call(&args);
}

static inline int fork()
{
    syscall_args_t args;
    args.id = SYS_fork;
    sys_call(&args);
}

#endif