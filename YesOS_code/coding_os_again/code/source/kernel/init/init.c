/**
 * @FilePath     : /code/source/kernel/init/init.c
 * @Description  :  内核初始化文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-14 17:52:45
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "init.h"
#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"

static boot_info_t *init_boot_info; // 启动信息

/**
 * @brief        : 内核初始化
 * @param         {boot_info_t} *boot_info: 启动信息
 * @return        {*}
 **/
void kernel_init(boot_info_t *boot_info)
{
    init_boot_info = boot_info;
    cpu_init();
    irq_init();
}

void init_main()
{
    int a = 3 / 0;
    for (;;)
    {
    }
}