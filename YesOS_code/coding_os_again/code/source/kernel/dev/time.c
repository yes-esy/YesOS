/**
 * @FilePath     : /code/source/kernel/dev/time.c
 * @Description  :  定时器c文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-27 21:02:46
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "dev/time.h"
#include "comm/types.h"
#include "comm/cpu_instr.h"
#include "os_cfg.h"
#include "cpu/irq.h"
#include "core/task.h"

static uint32_t sys_tick;

/**
 * @brief        : 定时器中断处理函数
 * @param         {exception_frame_t} *frame: 异常信息
 * @return        {*}
**/
void do_handler_timer(exception_frame_t *frame)
{
    sys_tick++;

    pic_send_eoi(IRQ0_TIMER);

    task_time_ticks();
}

/**
 * @brief        : 初始化
 * @return        {*}
 **/
static void init_pit()
{
    uint32_t reload_count = PIT_OSC_FREQ / (1000.0 * OS_TICKS_MS);

    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNEL0 | PIT_LOAD_LOHI | PIT_MODE3);

    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xFF); // 加载低8位

    outb(PIT_CHANNEL0_DATA_PORT, (reload_count >> 8) & 0xFF); // 加载高8位

    irq_install(IRQ0_TIMER, (irq_handler_t)exception_handler_timer);
    irq_enable(IRQ0_TIMER);
}

/**
 * @brief        : 定时器初始化函数
 * @return        {*}
 **/
void time_init(void)
{
    sys_tick = 0;
    init_pit();
}