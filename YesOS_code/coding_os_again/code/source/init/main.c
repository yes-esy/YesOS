/**
 * @FilePath     : /code/source/init/main.c
 * @Description  :  shell实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-11 18:39:44
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

    // int a = 3/ 0;
    *(char *) 0 = 0x1234;
    return 0;
}
