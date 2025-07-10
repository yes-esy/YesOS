/**
 * @FilePath     : /code/source/kernel/include/fs/devfs/devfs.h
 * @Description  :  设备文件系统.h头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-09 15:34:38
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef DEVFS_H
#define DEVFS_H

/**
 * 设备文件系统类型,tty设备,磁盘磁盘设备
 */
typedef struct _devfs_type_t
{
    const char *name;
    int dev_type;
    int file_type;
} devfs_type_t;

#endif