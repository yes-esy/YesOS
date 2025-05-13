/**
 * @FilePath     : /code/source/kernel/include/ipc/sem.h
 * @Description  :  信号量头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-05-07 21:03:42
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef SEM_H
#define SEM_H

#include "tools/list.h"

typedef struct _sem_t
{
    int count;        // 可以使用的临界资源的数量
    list_t wait_list; // 等待队列
} sem_t;

// 初始化函数
void sem_init(sem_t *sem, int init_count);

// 等信号
void sem_wait(sem_t *sem);

// 发信号
void sem_signal(sem_t *sem);

int sem_count(sem_t *sem);
#endif
