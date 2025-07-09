/**
 * @FilePath     : /code/source/applib/lib_syscall.c
 * @Description  :
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-09 11:31:58
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
 * @brief        : 进程放弃CPU系统调用接口
 * @return        {int} : 返回0
 **/
int yield()
{
    syscall_args_t args;
    args.id = SYS_yield;
    return sys_call(&args);
}
/**
 * @brief        : 打开文件系统调用接口
 * @param         {char} *name: 文件名
 * @param         {int} flags: 标志(只读,可写)
 * @return        {int} : 返回0
 **/
int open(const char *name, int flags, ...)
{
    syscall_args_t args;
    args.id = SYS_open;
    args.arg0 = (int)name;
    args.arg1 = (int)flags;
    return sys_call(&args);
}
/**
 * @brief        : 读取文件系统调用
 * @param         {int} file: 对应文件
 * @param         {char} *ptr: 读取数据存放区域
 * @param         {int} len: 读取长度
 * @return        {int} : 读取的数据长度
 **/
int read(int file, char *ptr, int len)
{
    syscall_args_t args;
    args.id = SYS_read;
    args.arg0 = file;
    args.arg1 = (int)ptr;
    args.arg2 = (int)len;
    return sys_call(&args);
}
/**
 * @brief        : 写文件系统调用
 * @param         {int} file: 文件
 * @param         {char} *ptr: 写入数据的指针
 * @param         {int} len: 写入文件长度
 * @return        {int} : 成功写入的数据长度
 **/
int write(int file, char *ptr, int len)
{
    syscall_args_t args;
    args.id = SYS_write;
    args.arg0 = file;
    args.arg1 = (int)ptr;
    args.arg2 = (int)len;
    return sys_call(&args);
}
/**
 * @brief        : 关闭文件系统调用
 * @param         {int} file: 对应文件
 * @return        {int} : 返回0
 **/
int close(int file)
{
    syscall_args_t args;
    args.id = SYS_close;
    args.arg0 = file;
    return sys_call(&args);
}
/**
 * @brief        : 调整文件读写指针的位置
 * @param         {int} file: 文件描述符
 * @param         {int} ptr: 指针偏移量（要移动的字节数）
 * @param         {int} dir: 移动方向/基准位置（通常是 SEEK_SET、SEEK_CUR、SEEK_END）
 * @return        {int} 调整后的文件的指针
 **/
int lseek(int file, int ptr, int dir)
{
    syscall_args_t args;
    args.id = SYS_lseek;
    args.arg0 = file;
    args.arg1 = ptr;
    args.arg2 = dir;
    return sys_call(&args);
}
/**
 * @brief        : 判断设备是否为tty设备
 * @param         {int} file: 文件
 * @return        {int} : 若文件为tty设备返回1,否则返回0
 **/
int isatty(int file)
{
    syscall_args_t args;
    args.id = SYS_isatty;
    args.arg0 = file;
    return sys_call(&args);
}
/**
 * @brief        : 用于获取文件状态信息
 * @param         {int} file: 文件描述符（标识要查询的文件）
 * @param         {stat} *st: 指向 struct stat 结构体的指针，用于存储文件状态信息
 * @return        {int} : 成功时返回 0 , 失败时返回 -1（
 **/
int fstat(int file, struct stat *st)
{
    syscall_args_t args;
    args.id = SYS_fstat;
    args.arg0 = file;
    args.arg1 = (int)st;
    return sys_call(&args);
}
/**
 * @brief        : 增加或减少程序的数据段（堆）大小
 * @param         {ptrdiff_t} incr:  增量值
 * @return        {void *} 成功时返回调整前的堆顶指针,失败时返回 (void *)-1
 **/
void *sbrk(ptrdiff_t incr)
{
    syscall_args_t args;
    args.id = SYS_sbrk;
    args.arg0 = (int)incr;
    return (void *)sys_call(&args);
}
/**
 * @brief        : 复制文件描述符
 * @param         {int} file: 要复制的文件描述符
 * @return        {int} 新的文件描述符，失败返回-1
 **/
int dup(int file)
{
    syscall_args_t args;
    args.id = SYS_dup;
    args.arg0 = file;
    return sys_call(&args);
}
/**
 * @brief        : 退出当前进程
 * @param         {int} status: 状态值
 * @return        {void}
 **/
void _exit(int status)
{
    syscall_args_t args;
    args.id = SYS_exit;
    args.arg0 = status;
    sys_call(&args);
    while (1)
        ;
}

/**
 * @brief        : 回收当前进程资源
 * @param         {int *} status: 状态值
 * @return        {void}
 **/
int wait(int *status)
{
    syscall_args_t args;
    args.id = SYS_wait;
    args.arg0 = (int)status;
    return sys_call(&args);
}
