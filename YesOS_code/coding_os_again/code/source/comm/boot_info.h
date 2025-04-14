/**
 * @FilePath     : /code/source/comm/boot_info.h
 * @Description  :  boot_info OS启动信息
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-05 14:48:40
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef BOOT_INFO_H
#define BOOT_INFO_H
#include "types.h"

#define BOOT_RAM_REGION_MAX 10 // RAM区最大数量

// 操作系统启动信息
typedef struct _boot_info_t
{
    // RAM区信息
    struct
    {
        uint32_t start; // 起始地址
        uint32_t size;  // 大小
    } ram_region_cfg[BOOT_RAM_REGION_MAX];
    int  ram_region_count; // 多少项目有效
}boot_info_t;

#define SECTOR_SIZE 512                    // 扇区大小 512字节
#define SYS_KERNEL_LOAD_ADDR (1024 * 1024) // loader放置的地址

#endif
