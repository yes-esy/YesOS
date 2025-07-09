/**
 * @FilePath     : /code/source/kernel/init/first_task.c
 * @Description  : first_task进程代码
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-08 09:48:32
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "applib/lib_syscall.h"
#include "dev/tty.h"
int first_task_main(void)
{
#if 0
    int cnt = 0;
    int pid = getpid();
    print_msg("parent process executing. pid is %d\n", pid);
    pid = fork(); // 创建子进程
    if (pid < 0)
    {
        print_msg("create child process failed. pid is %d\n", pid);
    }
    else if (pid == 0) // 由子进程执行
    {
        print_msg("child process executing.pid is %d\n", pid);
        cnt += 2;
        char *argv[] = {"arg0", "arg1", "arg2", "arg3"};
        execve("/shell.elf", argv, (char **)0);
    }
    else // 由父进程执行
    {
        print_msg("parent process executing, child process pid is %d\n", pid);
        cnt += 1;
    }
#endif  
    for (int i = 0; i < TTY_NR; i++)
    {
        int pid = fork();
        if (pid < 0)
        {
            print_msg("create shell failed. pid is %d\n", pid);
            break;
        }
        else if (pid == 0) // 子进程
        {
            char tty_num[5] = "tty:?";
            tty_num[4] = i + '0';
            char *argv[] = {tty_num, (char *)0};
            execve("/shell.elf", argv, (char **)0);
            print_msg("create shell proc failed\n", 0);
        }
    }
    while (1)
    {
        // log_printf("first task is running . Now cnt is %d",cnt++);
        // msleep(10000);
        // print_msg("cnt = %d\n", cnt++);
        int status;
        wait(&status);
    }

    return 0;
}