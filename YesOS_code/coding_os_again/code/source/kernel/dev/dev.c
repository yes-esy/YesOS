/**
 * @FilePath     : /code/source/kernel/dev/dev.c
 * @Description  :  设备管理层实现
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-06 20:27:05
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "dev/dev.h"
#include "cpu/irq.h"
#include "tools/klib.h"
#define DEV_TABLE_SIZE 128      // 设备数量
extern dev_desc_t dev_tty_desc; // 外部tty设备
/**
 * 静态设备类型表
 */
static dev_desc_t *dev_desc_table[] = {
    &dev_tty_desc,
};
/**
 * 设备实例表
 */
static device_t dev_table[DEV_TABLE_SIZE];
/**
 * @brief        : 设备的打开
 * @param         {int} major: 主设备号,标识设备类型
 * @param         {int} minor: 次设备号,用于标识同类型设备的不同实例
 * @param         {void *} data: 传递给设备的初始化数据
 * @return        {int} : 成功时返回设备在dev_table中的索引(>=0)，失败时返回-1
 **/
int dev_open(int major, int minor, void *data)
{
    irq_state_t irq_state = irq_enter_protection(); // 进入临界区
    device_t *free_dev = (device_t *)0;             // 记录找到的空闲设备位置

    // 第一次扫描：在设备实例表中查找
    for (int i = 0; i < sizeof(dev_table) / sizeof(dev_table[0]); i++)
    {
        device_t *dev = dev_table + i;
        if (dev->open_count == 0) // 找到空闲设备槽位
        {
            free_dev = dev;
        }
        else if ((dev->desc->major == major) && (dev->minor == minor)) // 设备已存在且已打开
        {
            dev->open_count++;               // 增加打开次数
            irq_leave_protection(irq_state); // 离开临界区
            return i;                        // 返回设备索引
        }
    }
    // 没有找到空闲设备
    // 第二次扫描：在设备类型表中查找匹配的设备类型
    dev_desc_t *desc = (dev_desc_t *)0;
    for (int i = 0; i < sizeof(dev_desc_table) / sizeof(dev_desc_table[0]); i++)
    {
        dev_desc_t *dev = dev_desc_table[i];
        if (dev->major == major) // 找到匹配的设备类型
        {
            desc = dev;
            break;
        }
    }

    // 如果找到了设备类型且有空闲槽位，则初始化新设备
    if (desc && free_dev)
    {
        // 初始化设备实例
        free_dev->minor = minor;
        free_dev->data = data;
        free_dev->desc = desc;
        int err = desc->open(free_dev); // 调用设备特定的打开函数
        if (err == 0)                   // 设备打开成功
        {
            free_dev->open_count = 1;
            irq_leave_protection(irq_state); // 离开临界区
            return free_dev - dev_table;     // 返回设备在表中的索引
        }
    }
    irq_leave_protection(irq_state); // 退出临界区
    return -1;                       // 打开失败
}
/**
 * @brief        : 判断设备id是否合法
 * @param         {int} dev_id: 设备索引(id)
 * @return        {int} 若合法返回0,非法返回1
 **/
static int is_dev_id_bad(int dev_id)
{
    if ((dev_id < 0) || (dev_id >= DEV_TABLE_SIZE)) // 不合法
    {
        return 1;
    }
    if (dev_table[dev_id].desc == (dev_desc_t *)0) // 设备描述为空
    {
        return 1;
    }
    return 0; // 合法
}
/**
 * @brief        : 从设备读取数据
 * @param         {int} dev_id: 设备Id
 * @param         {int} addr: 从哪个地址开始读取
 * @param         {char} *buf: 数据存放位置
 * @param         {int} size: 读取数据量
 * @return        {int} : 若读取成功则返回读取的数据量,否则返回-1
 **/
int dev_read(int dev_id, int addr, char *buf, int size)
{
    if (is_dev_id_bad(dev_id)) // 设备id是否合法
    {
        return -1;
    }
    device_t *dev = dev_table + dev_id;           // 具体的设备
    return dev->desc->read(dev, addr, buf, size); // 调用该设备的读取数据接口
}
/**
 * @brief        : 往设备写入数据
 * @param         {int} dev_id: 设备Id
 * @param         {int} addr: 写入的地址
 * @param         {char} *buf: 从哪里取数据写
 * @param         {int} size: 写入的数据量
 * @return        {int} : 写入的数据量
 **/
int dev_write(int dev_id, int addr, char *buf, int size)
{
    if (is_dev_id_bad(dev_id)) // 设备id是否合法
    {
        return -1;
    }
    device_t *dev = dev_table + dev_id;            // 具体的设备
    return dev->desc->write(dev, addr, buf, size); // 调用该设备的写数据接口
}
/**
 * @brief        : 控制设备
 * @param         {int} dev_id: 设备Id
 * @param         {int} cmd: 控制命令
 * @param         {int} arg0: 控制参数1
 * @param         {int} arg1: 控制参数2
 * @return        {int} : 若成功返回0,失败返回-1
 **/
int dev_control(int dev_id, int cmd, int arg0, int arg1)
{
    if (is_dev_id_bad(dev_id)) // 设备id是否合法
    {
        return -1;
    }
    device_t *dev = dev_table + dev_id;              // 具体的设备
    return dev->desc->control(dev, cmd, arg0, arg1); // 调用该设备的控制接口
}
/**
 * @brief        : 关闭设备
 * @param         {int} dev_id: 设备Id
 * @return        {void}
 **/
void dev_close(int dev_id)
{
    if (is_dev_id_bad(dev_id)) // 设备id是否合法
    {
        return ;
    }
    device_t *dev = dev_table + dev_id;         // 具体的设备
    irq_state_t state = irq_enter_protection(); // 进入临界区
    if (--dev->open_count == 0)
    {
        dev->desc->close(dev);                           // 调用该设备的关闭接口
        kernel_memset((void *)dev, 0, sizeof(device_t)); // 清空该设备
    }
    irq_leave_protection(state);
}