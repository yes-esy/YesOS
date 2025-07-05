/**
 * @FilePath     : /code/source/kernel/fs/fs.c
 * @Description  :  文件系统实现文件
 * @Author       : ys 2900226123@qq.com
 * @Version      : 0.0.1
 * @LastEditors  : ys 2900226123@qq.com
 * @LastEditTime : 2025-07-05 15:08:23
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 **/
#include "fs/fs.h"
#include "tools/klib.h"
#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "tools/log.h"
#include "dev/console.h"
#define TEMP_FILE_ID 100
static uint8_t TEMP_ADDR[100 * 1024]; // 临时地址
static uint8_t *temp_pos;             // 文件读写位置
/*临时使用*/
static void read_disk(uint32_t sector, uint32_t sector_count, uint8_t *buf)
{
    outb(0x1F6, (uint8_t)(0xE0));

    outb(0x1F2, (uint8_t)(sector_count >> 8));
    outb(0x1F3, (uint8_t)(sector >> 24)); // LBA参数的24 ~ 31位
    outb(0x1F4, (uint8_t)(0));            // LBA参数的32~39位
    outb(0x1F5, (uint8_t)(0));            // LBA参数的40~47位

    outb(0x1F2, (uint8_t)(sector_count));
    outb(0x1F3, (uint8_t)(sector));       // LBA参数的0~7位
    outb(0x1F4, (uint8_t)(sector >> 8));  // LBA参数的8~15位
    outb(0x1F5, (uint8_t)(sector >> 16)); // LBA参数的16~23位

    outb(0x1F7, (uint8_t)0x24);

    // 读取数据
    uint16_t *data_buf = (uint16_t *)buf;
    while (sector_count-- > 0)
    {
        // 每次扇区读之前都要检查,等待数据就绪
        while ((inb(0x1F7) & 0x88) != 0x8)
        {
        }

        // 读取数据并将数据写入到缓存中
        for (int i = 0; i < SECTOR_SIZE / 2; i++)
        {
            *data_buf++ = inw(0x1F0);
        }
    }
}

/**
 * @brief        : 打开文件
 * @param         {char} *path: 文件路径
 * @param         {int} flags: 标志
 * @return        {int} : 文件id
 **/
int sys_open(const char *path, int flags, ...)
{
    if (path[0] == '/')
    {
        read_disk(5000, 80, TEMP_ADDR);
        temp_pos = (uint8_t *)TEMP_ADDR;
        return TEMP_FILE_ID;
    }
    return -1;
}
/**
 * @brief        : 读取文件
 * @param         {int} file: 哪一个文件
 * @param         {char} *ptr: 读取到文件的目的地址
 * @param         {int} len: 读取长度
 * @return        {int} 读取成功返回读取长度,失败返回-1
 **/
int sys_read(int file, char *ptr, int len)
{
    if (file == TEMP_FILE_ID)
    {
        kernel_memcpy(ptr, temp_pos, len);
        temp_pos += len;
        return len;
    }
    return -1;
}
/**
 * @brief        : 写文件
 * @param         {int} file: 写入的文件
 * @param         {char} *ptr: 写入的内容的起始地址
 * @param         {int} len: 写入长度
 * @return        {int} : 返回-1
 **/
int sys_write(int file, char *ptr, int len)
{
    if (file == 1)
    { // ptr[len] = '\0';
        // log_printf("%s", ptr);
        console_write(0, ptr, len);
    }
    return -1;
}
/**
 * @brief        : 调整读写指针
 * @param         {int} file: 操作文件
 * @param         {int} ptr:
 * @param         {int} dir:
 * @return        {int} 成功返回 1, 失败返回 -1
 **/
int sys_lseek(int file, int ptr, int dir)
{
    if (file = TEMP_FILE_ID)
    {
        temp_pos = (uint8_t *)(TEMP_ADDR + ptr); // 调整指针
        return 0;
    }
    return -1;
}
int sys_close(int file)
{
    return 0;
}
int sys_isatty(int file)
{
    return 0;
}
int sys_fstat(struct stat *st)
{
    return 0;
}