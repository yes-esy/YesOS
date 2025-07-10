/**
 * @FilePath     : /code/source/kernel/include/dev/tty.h
 * @Description  :  tty设备.h头文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-09 15:03:29
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#ifndef TTY_H
#define TTY_H

#include "ipc/sem.h"

#define TTY_NR 1          // tty设备数量
#define TTY_O_BUF_SIZE 512 // 输入队列大小
#define TTY_I_BUF_SIZE 512 // 输出队列大小
#define TTY_OCRLF (1 << 0) // 将回车转换为换行
#define TTY_INCLR (1 << 0) // 
#define TTY_IECHO (1 << 1) // 是否输入回显
/**
 * tty读写队列
 */
typedef struct _tty_fifo_t
{
    char *buf; // 指向输入或输出缓存数组
    int size;  // 大小
    int read;  // 读的位置
    int wirte; // 写的位置
    int count; // 有效数据量
} tty_fifo_t;
/**
 * tty设备的输入输出描述符
 */
typedef struct _tty_t
{
    char o_buf[TTY_O_BUF_SIZE]; // 具体存放输出数据
    tty_fifo_t O_fifo;          // 输出缓存队列
    char i_buf[TTY_I_BUF_SIZE]; // 具体存放数入数据
    tty_fifo_t I_fifo;          // 输入缓存队列
    int console_index;          // 那一块显示区域
    sem_t o_sem;                // 输出信号量
    sem_t i_sem;                // 输入信号量
    int i_flags;                // 输入标志位
    int o_flags;                // 输出标志位
} tty_t;

/**
 * @brief        : 将数据输入到数据输出缓冲队列中
 * @param         {tty_fifo_t *} tty_fifo: 对应的输出缓冲队列
 * @param         {char} c : 输入到输出队列的数据
 * @return        {int}
 **/
int tty_fifo_put(tty_fifo_t *tty_fifo, char c);
/**
 * @brief        : 将数据从输出缓冲队列中取出
 * @param         {tty_fifo_t *} tty_fifo: 对应的输出缓冲队列
 * @param         {char *} c : 取出的数据的指针
 * @return        {int}  若成功返回0,失败返回-1
 **/
int tty_fifo_get(tty_fifo_t *tty_fifo, char *c);
/**
 * @brief        : tty设备读入数据
 * @param         {char} ch: 读入的数据
 * @return        {void}
**/
void tty_in( char ch);
/**
 * @brief        : 选择哪一个tty设备输出数据
 * @param         {int} tty: tty设备的序号
 * @return        {void}
**/
void tty_select(int tty);
#endif
