/**
 * @FilePath     : /code/source/shell/main.c
 * @Description  :  shell实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-05 15:54:12
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "lib_syscall.h"
#include <stdio.h>
int main(int argc, char **argv)
{
    // sbrk(0);
    // sbrk(100);
    // sbrk(100);
    // sbrk(200);
    // sbrk(4096 * 2 + 200);
    // sbrk(4096 * 5 + 200);
    printf("Hello from shell\n");
    for (int i = 0; i < argc; i++)
    {
        printf("arg %s\n", (int)argv[i]);
    }
    fork();
    yield();
    while (1)
    {
        printf("shell pid=%d\n", getpid());
        msleep(10000);
    }

    // int i = 0;
    // for (i = 0; i < 3; i++)
    // {
    //     int fpid = fork();
    //     if (fpid == 0)
    //         print_msg("son. pid is %d",fpid);
    //     else if(fpid > 0 )
    //         print_msg("father.son's pid is %d", fpid);
    //     else
    //         print_msg("fork failed . fpid is %d",fpid);
    // }
    return 0;
}
