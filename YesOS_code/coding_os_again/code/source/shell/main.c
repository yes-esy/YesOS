/**
 * @FilePath     : /code/source/shell/main.c
 * @Description  :  shell实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-06 17:03:12
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "lib_syscall.h"
#include <stdio.h>
int main(int argc, char **argv)
{
    printf("Hello from shell\n");
    printf("\0337Hello,word!\0338123\n"); // ESC 7,8 输出123lo,word!
    printf("\033[31;42mHello,word!\033[39;49m123\n"); // ESC [pn m, Hello,world红色，>其余绿色
    printf("123\033[2DHello,word!\n");                // 光标左移2，1Hello,word!
    printf("123\033[2CHello,word!\n");                // 光标右移2，123 Hello,word!
    printf("\033[31m");                               // ESC [pn m, Hello,world红色，其余绿色
    printf("\033[10;10H test!\n");                    // 定位到10, 10，test!
    printf("\033[20;20H test!\n");                    // 定位到20, 20，test!
    printf("\033[32;25;39m123\n");                    // ESC [pn m, Hello,world红色，其余绿色
    for (int i = 0; i < argc; i++)
    {
        printf("arg %s\n", (int)argv[i]);
    }
    fork();
    yield();
    printf("abef\b\b\b\bcd\n");
    printf("abcd\x7F:fg\n");
    while (1)
    {
        // printf("shell pid=%d\n", getpid());
        msleep(10000);
    }
    return 0;
}
