/**
 * @FilePath     : /code/source/kernel/init/first_task.c
 * @Description  : first_task进程代码
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-06-28 20:50:23
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
**/
#include "core/task.h"
#include "tools/log.h"
int first_task_main(void)
{
    int cnt = 0;
    while(1)
    {
        log_printf("first task is running . Now cnt is %d",cnt++);
        sys_sleep(10000);
    }
    return 0;
}