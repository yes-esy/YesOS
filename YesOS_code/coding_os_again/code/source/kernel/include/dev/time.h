/**
 * @FilePath     : /code/source/kernel/include/dev/time.h
 * @Description  :  定时器头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-04-16 21:19:56
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef TIME_H
#define TIME_H

#define PIT_OSC_FREQ 1193182
#define PIT_CHANNEL0_DATA_PORT 0x40
#define PIT_COMMAND_MODE_PORT 0x43
#define PIT_CHANNEL0 (0 << 6)
#define PIT_LOAD_LOHI (3 << 4)
#define PIT_MODE3 (3 << 1)
// 定时器初始化
void time_init(void);

// 中断处理函数
void exception_handler_timer(void);

#endif