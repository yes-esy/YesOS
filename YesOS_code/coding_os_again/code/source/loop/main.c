/**
 * @FilePath     : /code/source/loop/main.c
 * @Description  :  shell实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-11 17:19:08
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "lib_syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "main.h"
#include <sys/file.h>
#include "fs/file.h"
#include "dev/tty.h"


int main(int argc, char **argv)
{

    if (argc == 1) // 只有echo一个参数
    {
        printf(ESC_COLOR_INFO "Enter message: " ESC_COLOR_DEFAULT);
        fflush(stdout);                         // 强制刷新输出缓冲区
        char msg_buf[128];                      // 等待输入缓冲区
        fgets(msg_buf, sizeof(msg_buf), stdin); // 等待用户输入
        msg_buf[strlen(msg_buf) - 1] = '\0';    // 将'\n'替换
        puts(msg_buf);                          // 输出
        return 0;
    }
    int count = 1; // 默认echo一次;
    int ch;
    while ((ch = getopt(argc, argv, "n:h")) != -1)
    {
        switch (ch)
        {
        case 'h':
            // puts("echo message");
            // puts("echo [-n count] message -- echo some message");
            printf(ESC_COLOR_HELP "Usage: echo message\n");
            printf("       echo [-n count] message -- echo some message\n" ESC_COLOR_DEFAULT);
            optind = 1; // getopt需要多次调用，需要重置
            return 0;
        case 'n':
            count = atoi(optarg);
            break;
        case '?':
            if (optarg)
            {
                fprintf(stderr, ESC_COLOR_ERROR "Unknown option\n" ESC_COLOR_DEFAULT);
            }
            optind = 1; // getopt需要多次调用，需要重置
            return -1;
        default:
            break;
        }
    }
    if (optind > argc - 1)
    {
        fprintf(stderr, ESC_COLOR_ERROR "message is empty\n" ESC_COLOR_DEFAULT);
        optind = 1; // getopt需要多次调用，需要重置
        return -1;
    }
    char *msg = argv[optind];
    for (int i = 0; i < count; i++)
    {
        puts(msg);
    }
    optind = 1; // getopt需要多次调用，需要重置
    return 0;
}
