/**
 * @FilePath     : /code/source/kernel/init/init.c
 * @Description  :  内核初始化文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-21 21:11:53
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "init.h"
#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "tools/log.h"
#include "os_cfg.h"
#include "tools/klib.h"

static boot_info_t *init_boot_info; // 启动信息

/**
 * @brief        : 内核初始化
 * @param         {boot_info_t} *boot_info: 启动信息
 * @return        {*}
 **/
void kernel_init(boot_info_t *boot_info)
{
    ASSERT(boot_info->ram_region_count != 0);
    ASSERT(3 < 2);
    init_boot_info = boot_info;
    cpu_init();

    log_init();
    irq_init();
    time_init();
}

void init_main()
{
    int a = 3 / 0;
    // irq_enable_global(); // 测试打开全局中断
    log_printf("Kernel is running . . .");
    log_printf("Version:%s",OS_VERSION);
    log_printf("%s %x","hello",10);
    for (;;)
    {
    }
}