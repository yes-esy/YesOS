/**
 * @FilePath     : /code/source/boot/boot.c
 * @Description  :  系统引导部分，启动时由硬件加载运行，然后完成对二级引导程序loader的加载
 *                  boot扇区容量较小，仅512字节。由于dbr占用了不少字节，导致其没有多少空间放代码，
 *                  所以功能只能最简化,并且要开启最大的优化-os
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-13 20:55:10
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
__asm__(".code16gcc");

#include "boot.h"

#define LOADER_START_ADDR 0x8000 // loader加载的地址

/**
 * @brief        : Boot的C入口函数.只完成一项功能，
 *  即从磁盘找到loader文件然后加载到内容中，并跳转过去
 * @return        {*}
 **/
void boot_entry(void)
{
    ((void(*)(void)) LOADER_START_ADDR)();
}
