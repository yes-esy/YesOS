/**
 * @FilePath     : /code/source/kernel/include/fs/fs.h
 * @Description  :  文件系统头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-03 10:52:34
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
**/
#ifndef FS_H
#define FS_H

#include "comm/types.h"

int sys_open(const char * path, int flags,...);
int sys_read(int file,char *ptr , int len);
int sys_write(int file,char *ptr , int len);
int sys_lseek(int file,int ptr , int dir);
int sys_close(int file);

#endif