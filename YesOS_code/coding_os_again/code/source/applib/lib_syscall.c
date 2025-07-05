/**
 * @FilePath     : /code/source/applib/lib_syscall.c
 * @Description  :
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-04 21:21:18
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "lib_syscall.h"

/**
 * @brief        : 延时系统调用接口
 * @param         {int} ms: 睡眠的毫秒数
 * @return        {void}
 **/
void msleep(int ms)
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
 * @brief        : 获取当前进程的ID号接口
 * @return        {int} : 进程ID号
 **/
int getpid()
{
    syscall_args_t args;
    args.id = SYS_getpid;
    return sys_call(&args);
}
/**
 * @brief        : 简单打印信息接口
 * @param         {char} *fmt: 格式化字符串
 * @param         {int} arg: 参数
 * @return        {void}
 **/
void print_msg(const char *fmt, int arg)
{
    syscall_args_t args;
    args.id = SYS_print_msg;
    args.arg0 = (int)fmt;
    args.arg1 = arg;
    sys_call(&args);
}
/**
 * @brief        : 创建子进程的系统调用接口
 * @return        {int} : 子进程的PID
 **/
int fork()
{
    syscall_args_t args;
    args.id = SYS_fork;
    return sys_call(&args);
}
/**
 * @brief        : 创建一个新的进程
 * @param         {char *} name: 应用程序路径
 * @param         {char **} argv: 参数
 * @param         {char **} env: 环境变量
 * @return        {int} : 创建的进程的PID
 **/
int execve(const char *name, char *const *argv, char *const *env)
{
    syscall_args_t args;
    args.id = SYS_execve;
    args.arg0 = (int)name;
    args.arg1 = (int)argv;
    args.arg2 = (int)env;
    return sys_call(&args);
}

/**
 * @brief        : 进程放弃CPU
 * @return        {int} : 返回0
 **/
int yield()
{
    syscall_args_t args;
    args.id = SYS_yield;
    return sys_call(&args);
}
int open(const char *name, int flags, ...)
{
    syscall_args_t args;
    args.id = SYS_open;
    args.arg0 = (int)name;
    args.arg1 = (int)flags;
    return sys_call(&args);
}
int read(int file, char *ptr, int len)
{
    syscall_args_t args;
    args.id = SYS_read;
    args.arg0 = file;
    args.arg1 = (int)ptr;
    args.arg2 = (int)len;
    return sys_call(&args);
}
int write(int file, char *ptr, int len)
{
    syscall_args_t args;
    args.id = SYS_write;
    args.arg0 = file;
    args.arg1 = (int)ptr;
    args.arg2 = (int)len;
    return sys_call(&args);
}
int close(int file)
{
    syscall_args_t args;
    args.id = SYS_close;
    args.arg0 = file;
    return sys_call(&args);
}
int lseek(int file, int ptr, int dir)
{
    syscall_args_t args;
    args.id = SYS_lseek;
    args.arg0 = file;
    args.arg1 = ptr;
    args.arg2 = dir;
    return sys_call(&args);
}

int isatty(int file)
{
    syscall_args_t args;
    args.id = SYS_isatty;
    args.arg0 = file;
    return sys_call(&args);
}
int fstat(int file, struct stat *st)
{
    syscall_args_t args;
    args.id = SYS_fstat;
    args.arg0 = file;
    args.arg1 = (int)st;
    return sys_call(&args);
}
void *sbrk(ptrdiff_t incr)
{
    syscall_args_t args;
    args.id = SYS_sbrk;
    args.arg0 = (int)incr;
    return (void *)sys_call(&args);
}