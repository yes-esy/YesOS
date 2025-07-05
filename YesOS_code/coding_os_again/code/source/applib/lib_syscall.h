/**
 * @FilePath     : /code/source/applib/lib_syscall.h
 * @Description  :
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-04 20:06:17
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H

#include "os_cfg.h"
#include "core/syscall.h"
#include <sys/stat.h>
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
    uint32_t sys_gate_addr[] = {0, SELECTOR_SYSCALL | 0}; // RPL<=DPL ,RPL设置为0
    __asm__ __volatile__(
        "push %[arg3]\n\t"
        "push %[arg2]\n\t"
        "push %[arg1]\n\t"
        "push %[arg0]\n\t"
        "push %[id]\n\t"
        "lcalll *(%[gate])\n\n"
        : "=a"(ret) // 表明从eax中取出其值
        : [arg3] "r"(args->arg3),
          [arg2] "r"(args->arg2),
          [arg1] "r"(args->arg1),
          [arg0] "r"(args->arg0),
          [id] "r"(args->id),
          [gate] "r"(sys_gate_addr)); // 跳转到exception_handler_syscall
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
 * @brief        : 延时系统调用接口
 * @param         {int} ms: 睡眠的毫秒数
 * @return        {void}
 **/
void msleep(int ms);
/**
 * @brief        : 获取当前进程的ID号接口
 * @return        {int} : 进程ID号
 **/
int getpid();
/**
 * @brief        : 简单打印信息接口
 * @param         {char} *fmt: 格式化字符串
 * @param         {int} arg: 参数
 * @return        {void}
 **/
void print_msg(const char *fmt, int arg);
/**
 * @brief        : 创建子进程的系统调用接口
 * @return        {int} : 子进程的PID
 **/
int fork();
/**
 * @brief        : 创建一个新的进程
 * @param         {char *} name: 应用程序路径
 * @param         {char **} argv: 参数
 * @param         {char **} env: 环境变量
 * @return        {int} : 创建的进程的PID
 **/
int execve(const char *name, char *const *argv, char *const *env);
/**
 * @brief        : 进程放弃CPU
 * @return        {int} : 返回0
 **/
int yield();
int open(const char * name,int flags,...);
int read(int file,char *ptr,int len);
int write(int file,char *ptr,int len);
int close(int file);
int lseek(int file,int ptr,int dir);

int isatty(int file);
int fstat(int file ,struct stat * st);
void * sbrk(ptrdiff_t incr);
#endif