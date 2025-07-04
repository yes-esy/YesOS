/**
 * @FilePath     : /code/source/kernel/core/syscall.c
 * @Description  :  系统调用实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-03 15:42:05
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "core/syscall.h"
#include "core/task.h"
#include "tools/log.h"
#include "cpu/irq.h"

typedef int (*sys_handler_t)(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);
/**
 * @brief        : 
 * @param         {char} *fmt:
 * @param         {int} arg:
 * @return        {*}
**/
void sys_print_msg(char *fmt, int arg)
{
    log_printf(fmt, arg);
}
/**
 * 系统调用表
 */
static const sys_handler_t sys_table[] = {
    [SYS_sleep] = (sys_handler_t)sys_sleep,
    [SYS_getpid] = (sys_handler_t)sys_getpid,
    [SYS_print_msg] = (sys_handler_t)sys_print_msg,
    [SYS_fork] = (sys_handler_t)sys_fork,
    [SYS_execve] = (sys_handler_t)sys_execve,
    [SYS_yield] = (sys_handler_t)sys_yield,

};
/**
 * @brief        : 通用系统调用处理函数
 * @param         {syscall_frame_t *} frame: 寄存器信息
 * @return        {void}
 **/
void do_handler_syscall(syscall_frame_t *frame)
{
    if (frame->func_id < sizeof(sys_table) / sizeof(sys_table[0])) // 系统调用号合法
    {
        sys_handler_t handler = sys_table[frame->func_id]; // 系统调用对应的函数

        if (handler)
        {
            int ret = handler(frame->arg0, frame->arg1, frame->arg2, frame->arg3); // 调用该函数
            frame->eax = ret; // 返回值
            return;
        }
    }
    task_t *task = task_current();
    log_printf("task %s, Unknown syscall : %d", task->name, frame->func_id);
    frame->eax = -1; // 出错
}
/* 中断的方式实现系统调用 */
// void do_handler_syscall(exception_frame_t *frame)
// {
//     int func_id = frame->eax;
//     int arg0 = frame->ebx;
//     int arg1 = frame->ecx;
//     int arg2 = frame->edx;
//     int arg3 = frame->esi;

//     if (func_id < sizeof(sys_table) / sizeof(sys_table[0]))
//     {
//         sys_handler_t handler = sys_table[func_id];

//         if (handler)
//         {
//             int ret = handler(arg0, arg1, arg2, arg3);
//             frame->eax = ret;
//             return;
//         }
//     }
//     task_t *task = task_current();
//     log_printf("task %s, Unknown syscall : %d", task->name, func_id);
//     frame->eax = -1;
// }