/**
 * @FilePath     : /code/source/kernel/init/first_task.c
 * @Description  : first_task进程代码
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-01 22:00:35
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "core/task.h"
#include "tools/log.h"
#include "applib/lib_syscall.h"
int first_task_main(void)
{
    int pid = getpid();
    pid = fork();
    if(pid < 0)
    {
        print_msg("create child process failed. pid is %d" , pid);
    } else if(pid == 0)
    {
        print_msg("child process , process id is %d" , pid);
    }else
    {
        print_msg("parent process , process id is %d",pid);
    }
    print_msg("taskId = %d", pid);
    int cnt = 0;
    while (1)
    {
        // log_printf("first task is running . Now cnt is %d",cnt++);
        msleep(100000);
        print_msg("cnt = %d", cnt++);
    }
    return 0;
}