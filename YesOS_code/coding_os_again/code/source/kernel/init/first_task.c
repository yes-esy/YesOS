/**
 * @FilePath     : /code/source/kernel/init/first_task.c
 * @Description  : first_task进程代码
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-03 10:20:54
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "core/task.h"
#include "tools/log.h"
#include "applib/lib_syscall.h"
int first_task_main(void)
{
    int cnt = 0;
    int pid = getpid();
    print_msg("parent process executing. pid is %d", pid);
    pid = fork(); // 创建子进程
    if (pid < 0)
    {
        print_msg("create child process failed. pid is %d", pid);
    }
    else if (pid == 0) // 由子进程执行
    {
        print_msg("child process executing.pid is %d", pid);
        cnt += 2;
        char *argv[] = {"arg0", "arg1", "arg2", "arg3"};
        execve("/shell.elf", argv, (char **)0);
    }
    else // 由父进程执行
    {
        print_msg("parent process executing, child process pid is %d", pid);
        cnt += 1;
    }
    while (1)
    {
        // log_printf("first task is running . Now cnt is %d",cnt++);
        msleep(10000);
        print_msg("cnt = %d", cnt++);
    }
    return 0;
}