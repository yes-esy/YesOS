#include "cpu/irq.h"
#include "core/task.h"
#include "ipc/sem.h"

void sem_init(sem_t *sem, int count)
{
    sem->count = count;
    list_init(&sem->wait_list);
}

void sem_wait(sem_t *sem)
{
    irq_state_t state = irq_enter_protection();
    if (sem->count > 0)
    {
        sem->count--;
    }
    else
    {
        task_t *current_task = task_current();
        task_set_block(current_task);
        list_insert_last(&sem->wait_list, &current_task->wait_node);

        task_dispatch();
    }
    irq_leave_protection(state);
}

/**
 * 释放信号量
 */
void sem_signal(sem_t *sem)
{
    irq_state_t irq_state = irq_enter_protection();

    if (list_count(&sem->wait_list))
    {
        // 有进程等待，则唤醒加入就绪队列
        list_node_t *node = list_remove_first(&sem->wait_list);
        task_t *task = list_node_parent(node, task_t, wait_node);
        task_set_ready(task);

        task_dispatch();
    }
    else
    {
        sem->count++;
    }

    irq_leave_protection(irq_state);
}

int sem_count(sem_t *sem)
{
    irq_state_t state = irq_enter_protection();
    int count = sem->count;
    irq_leave_protection(state);
    return count;
}