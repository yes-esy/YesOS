/**
 * @FilePath     : /code/source/kernel/dev/tty.c
 * @Description  :  tty设备.c文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-11 15:55:14
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/

#include "dev/dev.h"
#include "dev/tty.h"
#include "tools/log.h"
#include "dev/console.h"
#include "dev/keyboard.h"
#include "cpu/irq.h"
static int curr_tty_index = 0;         // 初始tty设备索引
static tty_t tty_device_table[TTY_NR]; // tty设备
/**
 * @brief        : 选择哪一个tty设备输出数据
 * @param         {int} tty: tty设备的序号
 * @return        {void}
 **/
void tty_select(int tty)
{
    if (tty != curr_tty_index)
    {
        console_select(tty);  // 切换显示数据
        curr_tty_index = tty; // 设置为新的tty设备
    }
}
/**
 * @brief        : 获取对应的tty设备
 * @param         {device_t *} dev: 某一种类型的设备号
 * @return        {tty_t *} : tty设备的指针
 **/
static tty_t *get_tty(device_t *dev)
{
    int tty_id = dev->minor;
    if ((tty_id < 0) || (tty_id >= TTY_NR) || (!dev->open_count))
    {
        log_printf("tty device open failed. tty id is %d\n", tty_id);
        return (tty_t *)0;
    }
    return tty_device_table + tty_id;
}
/**
 * @brief        : 初始化输入输出缓冲队列
 * @param         {tty_fifo_t} *fifo: 输入输出队列结构指针
 * @param         {char} *buf: 管理的数据队列
 * @param         {int} size: 大小
 * @return        {void}
 **/
void tty_fifo_init(tty_fifo_t *fifo, char *buf, int size)
{
    fifo->buf = buf;
    fifo->count = 0;
    fifo->size = size;
    fifo->read = fifo->wirte = 0;
}
/**
 * @brief        : 将一字节数据输入到数据输出缓冲队列中
 * @param         {tty_fifo_t *} tty_fifo: 对应的输出缓冲队列
 * @param         {char} c : 输入到输出队列的数据
 * @return        {int}
 **/
int tty_fifo_put(tty_fifo_t *tty_fifo, char c)
{
    irq_state_t state = irq_enter_protection(); // 开中断保护
    if (tty_fifo->count >= tty_fifo->size)      // 输出队列已满
    {
        irq_leave_protection(state); // 关中断返回
        return -1;
    }
    tty_fifo->buf[tty_fifo->wirte++] = c;
    if (tty_fifo->wirte >= tty_fifo->size) // 写指针到队列尾部
    {
        tty_fifo->wirte = 0; // 回到起点
    }
    tty_fifo->count++;           // 数据量增加
    irq_leave_protection(state); // 关中断返回
    return 0;
}
/**
 * @brief        : 将数一字节据从输出缓冲队列中取出
 * @param         {tty_fifo_t *} tty_fifo: 对应的输出缓冲队列
 * @param         {char *} c : 取出的数据的指针
 * @return        {int}  若成功返回0,失败返回-1
 **/
int tty_fifo_get(tty_fifo_t *tty_fifo, char *c)
{
    irq_state_t state = irq_enter_protection(); // 开中断保护
    if (tty_fifo->count <= 0)                   // 输出队列为空
    {
        irq_leave_protection(state); // 关中断返回
        return -1;
    }
    *c = tty_fifo->buf[tty_fifo->read++];
    if (tty_fifo->read >= tty_fifo->size) // 读指针到队列尾部
    {
        tty_fifo->read = 0; // 回到起点
    }
    tty_fifo->count--;           // 数据减少
    irq_leave_protection(state); // 关中断返回
    return 0;
}
/**
 * @brief        : 打开tty设备,对设备进行初始化
 * @param         {device_t *} dev: 打开的tty设备
 * @return        {int} : 若成功打开返回0，否则返回-1
 **/
int tty_open(device_t *dev)
{
    int index = dev->minor;             // 取当前tty设备索引(ID号)
    if (index < 0 || (index >= TTY_NR)) // 索引(ID号)不合法
    {
        log_printf("open tty device failed. incorrect tty index = %d\n", index);
        return -1;
    }
    tty_t *tty = tty_device_table + index;                   // 取当前tty设备指针
    tty->o_flags = TTY_OCRLF;                                // 打开回车换行功能
    tty->i_flags = TTY_INCLR | TTY_IECHO;                    // 输入设置
    tty_fifo_init(&tty->O_fifo, tty->o_buf, TTY_O_BUF_SIZE); // 初始化输出缓冲区
    sem_init(&tty->o_sem, TTY_O_BUF_SIZE);                   // 初始化输出信号量
    sem_init(&tty->i_sem, 0);                                // 初始化输入信号量
    tty_fifo_init(&tty->I_fifo, tty->i_buf, TTY_I_BUF_SIZE); // 初始化输入缓冲区
    tty->console_index = index;                              // 指定显示器
    console_init(index);                                     // 初始化控制台
    kbd_init();                                              // 初始化键盘
    return 0;
}
/**
 * @brief        : 往设备写入数据
 * @param         {device_t} *dev: 写入数据的设备
 * @param         {int} addr: 从设备的哪里开始写
 * @param         {char} *buf: 写入的数据的起始地址
 * @param         {int} size: 写入的数据量
 * @return        {int} : 成功则返回写入的数据量,否则返回-1
 *
 **/
int tty_write(device_t *dev, int addr, char *buf, int size)
{
    if (size < 0) // 写入大小不正确
    {
        return -1;
    }
    tty_t *tty = get_tty(dev); // 获取对应的tty设备
    if (tty == (tty_t *)0)     // 获取失败
    {
        return -1;
    }
    int len = 0; // 记录写入的数据量
    // 先将所有数据写入缓存中
    while (size) // 往tty设备写入数据
    {
        char c = *buf++; // 取出buf中的数据

        // 如果遇到\n，根据配置决定是否转换成\r\n
        if (c == '\n' && (tty->o_flags & TTY_OCRLF))
        {
            sem_wait(&tty->o_sem);
            int err = tty_fifo_put(&tty->O_fifo, '\r');
            if (err < 0)
            {
                break;
            }
        }

        sem_wait(&tty->o_sem);                   // 等信号量
        int err = tty_fifo_put(&tty->O_fifo, c); // 往输出缓冲队列写数据
        if (err < 0)                             // 写失败
        {
            break;
        }
        len++;
        size--;
        // if(显示器是不是忙)
        // {
        //     continue;
        // }
        // else
        // {
        //     启动硬件发送显示
        // }
        console_write(tty);
    }
    return len;
}
/**
 * @brief        : 读取设备的数据
 * @param         {device_t} *dev: 具体的设备的指针
 * @param         {int} addr: 从设备的何处读取
 * @param         {char *} buf: 读取到哪里
 * @param         {int} size: 读取的数据量
 * @return        {int} : 成功读取的数据量,失败返回-1
 **/
int tty_read(device_t *dev, int addr, char *buf, int size)
{
    if (size < 0)
    {
        return -1;
    }
    tty_t *tty = get_tty(dev); // 当前tty设备
    char *pbuf = buf;          // 记录读入数据缓冲区起始地址
    int len = 0;
    while (len < size)
    {
        sem_wait(&tty->i_sem); // 等信号量

        char ch;
        tty_fifo_get(&tty->I_fifo, &ch); // 取数据
        switch (ch)
        {
        case ASCII_DEL:
            if (len == 0)
            {
                continue;
            }
            len--;
            pbuf--;
            break;
        case '\n':
            if ((tty->i_flags & TTY_INCLR) && (len < size - 1)) // 检查输入设置
            {
                *pbuf++ = '\r'; // 添加换行
                len++;
            }
            *pbuf++ = '\n'; // 添加回车
            len++;
            break;
        default: // 非换行字符
            *pbuf++ = ch;
            len++;
            break;
        }
        if (tty->i_flags & TTY_IECHO) // 回显
        {
            tty_write(dev, 0, &ch, 1);
        }
        if ((ch == '\n') || (ch == '\r')) // 回车即输入结束
        {
            break;
        }
    }
    return len;
}
/**
 * @brief        : 控制设备
 * @param         {device_t *} dev: 具体的设备描述指针
 * @param         {int} cmd: 控制命令
 * @param         {int} arg0: 命令的参数1
 * @param         {int} arg1: 命令的参数2
 * @return        {int} : 返回0
 **/
int tty_control(device_t *dev, int cmd, int arg0, int arg1)
{
    tty_t *tty = get_tty(dev); // 获取tty设备
    switch (cmd)
    {
    case TTY_CMD_ECHO:
        if (arg0)
        {
            tty->i_flags |= TTY_IECHO;
        }
        else
        {
            tty->i_flags &= ~TTY_IECHO;
        }
        break;
    default:
        break;
    }
    return 0;
}
/**
 * @brief        : 关闭设备
 * @param         {device_t *} dev: 具体的设备描述的指针
 * @return        {void}
 **/
void tty_close(device_t *dev)
{
}
/**
 * @brief        : tty设备读入数据存入数据缓冲区
 * @param         {char} ch: 读入的数据
 * @return        {void}
 **/
void tty_in(char ch)
{
    tty_t *tty = tty_device_table + curr_tty_index; // 对应的tty设备
    if (sem_count(&tty->i_sem) >= TTY_I_BUF_SIZE)   // 缓冲区已满
    {
        return; // 直接返回
    }
    // 缓冲区未满
    tty_fifo_put(&tty->I_fifo, ch); // 将读入的数据添加到输入缓冲队列
    sem_signal(&tty->i_sem);        //  已经写入一个数据到缓冲区,发信号
}

/**
 * tty设备描述
 */
dev_desc_t dev_tty_desc = {
    .name = "tty_device",
    .major = DEV_TTY,
    .open = tty_open,
    .read = tty_read,
    .write = tty_write,
    .control = tty_control,
    .close = tty_close,
};